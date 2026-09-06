#include <catch2/catch_test_macros.hpp>

#include "scanner.hxx"

#include <cstddef>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace avium;

namespace {

std::vector<Lexeme> scanAll(std::string_view source)
{
    std::istringstream input{std::string{source}};
    Scanner scanner{input};
    std::vector<Lexeme> result;

    for( ;; ) {
        auto lexeme = scanner.scan();
        result.push_back(lexeme);
        if( lexeme.is(Token::Eof) )
            return result;
    }
}

} // namespace

TEST_CASE("Scanner-ը կարդում է թվային լիտերալներ", "[scanner]")
{
    const auto lexemes = scanAll("12 3.14 0.5");

    REQUIRE(lexemes.size() == 4);
    CHECK(lexemes[0].is(Token::RealLit));
    CHECK(lexemes[0].value == "12");
    CHECK(lexemes[1].is(Token::RealLit));
    CHECK(lexemes[1].value == "3.14");
    CHECK(lexemes[2].is(Token::RealLit));
    CHECK(lexemes[2].value == "0.5");
    CHECK(lexemes[3].is(Token::Eof));
}

TEST_CASE("Scanner-ը մերժում է կետով ավարտվող թիվը", "[scanner]")
{
    const auto lexemes = scanAll("1.");

    REQUIRE(lexemes.size() == 2);
    CHECK(lexemes[0].is(Token::None));
    CHECK(lexemes[0].value == "1.");
    CHECK(lexemes[1].is(Token::Eof));
}

TEST_CASE("Scanner-ը կարդում է տեքստային լիտերալներ", "[scanner]")
{
    const auto lexemes = scanAll(R"("hello" "")");

    REQUIRE(lexemes.size() == 3);
    CHECK(lexemes[0].is(Token::TextLit));
    CHECK(lexemes[0].value == "hello");
    CHECK(lexemes[1].is(Token::TextLit));
    CHECK(lexemes[1].value.empty());
    CHECK(lexemes[2].is(Token::Eof));
}

TEST_CASE("Scanner-ը ճիշտ է մշակում տեքստի մեջ newline-ի սխալը", "[scanner]")
{
    const auto lexemes = scanAll("\"hello\nnext");

    REQUIRE(lexemes.size() == 4);
    CHECK(lexemes[0].is(Token::None));
    CHECK(lexemes[0].value == "hello");
    CHECK(lexemes[0].line == 1);
    CHECK(lexemes[1].is(Token::NewLine));
    CHECK(lexemes[1].line == 1);
    CHECK(lexemes[2].is(Token::Identifier));
    CHECK(lexemes[2].value == "next");
    CHECK(lexemes[2].line == 2);
    CHECK(lexemes[3].is(Token::Eof));
}

TEST_CASE("Scanner-ը ճանաչում է identifier-ները", "[scanner]")
{
    const auto lexemes = scanAll("value2 Main Print Input NUM");

    REQUIRE(lexemes.size() == 6);
    for( std::size_t index = 0; index < 5; ++index )
        CHECK(lexemes[index].is(Token::Identifier));
    CHECK(lexemes[0].value == "value2");
    CHECK(lexemes[1].value == "Main");
    CHECK(lexemes[2].value == "Print");
    CHECK(lexemes[3].value == "Input");
    CHECK(lexemes[4].value == "NUM");
    CHECK(lexemes[5].is(Token::Eof));
}

TEST_CASE("Scanner-ը մերժում է underscore-ը identifier-ում", "[scanner]")
{
    const auto lexemes = scanAll("_x value_2");

    REQUIRE(lexemes.size() == 6);
    CHECK(lexemes[0].is(Token::None));
    CHECK(lexemes[0].value == "_");
    CHECK(lexemes[1].is(Token::Identifier));
    CHECK(lexemes[1].value == "x");
    CHECK(lexemes[2].is(Token::Identifier));
    CHECK(lexemes[2].value == "value");
    CHECK(lexemes[3].is(Token::None));
    CHECK(lexemes[3].value == "_");
    CHECK(lexemes[4].is(Token::RealLit));
    CHECK(lexemes[4].value == "2");
    CHECK(lexemes[5].is(Token::Eof));
}

TEST_CASE("Scanner-ը պահպանում է մեծատառ keyword-ների զգայունությունը", "[scanner]")
{
    const auto lexemes = scanAll("SUB sub TRUE FALSE true MOD");

    REQUIRE(lexemes.size() == 7);
    CHECK(lexemes[0].is(Token::Subroutine));
    CHECK(lexemes[1].is(Token::Identifier));
    CHECK(lexemes[2].is(Token::BoolLit));
    CHECK(lexemes[3].is(Token::BoolLit));
    CHECK(lexemes[4].is(Token::Identifier));
    CHECK(lexemes[5].is(Token::Mod));
    CHECK(lexemes[6].is(Token::Eof));
}

TEST_CASE("Scanner-ը ճանաչում է Կեռասի բոլոր ծառայողական բառերը", "[scanner]")
{
    using Keyword = std::pair<std::string_view, Token>;
    const std::vector<Keyword> keywords{
        {"SUB", Token::Subroutine},
        {"LET", Token::Let},
        {"DIM", Token::Dim},
        {"AS", Token::As},
        {"REAL", Token::Real},
        {"TEXT", Token::Text},
        {"BOOL", Token::Bool},
        {"IF", Token::If},
        {"THEN", Token::Then},
        {"ELSEIF", Token::ElseIf},
        {"ELSE", Token::Else},
        {"WHILE", Token::While},
        {"FOR", Token::For},
        {"TO", Token::To},
        {"STEP", Token::Step},
        {"CALL", Token::Call},
        {"END", Token::End},
        {"MOD", Token::Mod},
        {"AND", Token::And},
        {"OR", Token::Or},
        {"NOT", Token::Not},
    };

    std::string source;
    for( const auto& [word, token] : keywords ) {
        source += word;
        source += ' ';
    }

    const auto lexemes = scanAll(source);
    REQUIRE(lexemes.size() == keywords.size() + 1);
    for( std::size_t index = 0; index < keywords.size(); ++index ) {
        CHECK(lexemes[index].kind == keywords[index].second);
        CHECK(lexemes[index].value == keywords[index].first);
    }
    CHECK(lexemes.back().is(Token::Eof));
}

TEST_CASE("Scanner-ը կարդում է single և multi-character օպերատորները", "[scanner]")
{
    const auto lexemes = scanAll("( ) [ ] , + - * / \\ ^ & = < <> <= > >=");
    const std::vector<Token> expected{
        Token::LeftPar,
        Token::RightPar,
        Token::LeftBrack,
        Token::RightBrack,
        Token::Comma,
        Token::Add,
        Token::Sub,
        Token::Mul,
        Token::Div,
        Token::Quot,
        Token::Pow,
        Token::Amp,
        Token::Eq,
        Token::Lt,
        Token::Ne,
        Token::Le,
        Token::Gt,
        Token::Ge,
        Token::Eof,
    };

    REQUIRE(lexemes.size() == expected.size());
    for( std::size_t index = 0; index < expected.size(); ++index )
        CHECK(lexemes[index].is(expected[index]));
}

TEST_CASE("Scanner-ը պահպանում է newline-ները և թարմացնում line-ը", "[scanner]")
{
    const auto lexemes = scanAll("a\n\nb\r\nc");

    REQUIRE(lexemes.size() == 7);
    CHECK(lexemes[0].is(Token::Identifier));
    CHECK(lexemes[0].line == 1);
    CHECK(lexemes[1].is(Token::NewLine));
    CHECK(lexemes[1].line == 1);
    CHECK(lexemes[2].is(Token::NewLine));
    CHECK(lexemes[2].line == 2);
    CHECK(lexemes[3].is(Token::Identifier));
    CHECK(lexemes[3].line == 3);
    CHECK(lexemes[4].is(Token::NewLine));
    CHECK(lexemes[4].line == 3);
    CHECK(lexemes[5].is(Token::Identifier));
    CHECK(lexemes[5].line == 4);
    CHECK(lexemes[6].is(Token::Eof));
    CHECK(lexemes[6].line == 4);
}

TEST_CASE("Scanner-ը բաց է թողնում comment-ը, բայց պահպանում է նրա newline-ը", "[scanner]")
{
    const auto lexemes = scanAll("' comment\nSUB Main");

    REQUIRE(lexemes.size() == 4);
    CHECK(lexemes[0].is(Token::NewLine));
    CHECK(lexemes[0].line == 1);
    CHECK(lexemes[1].is(Token::Subroutine));
    CHECK(lexemes[1].line == 2);
    CHECK(lexemes[2].is(Token::Identifier));
    CHECK(lexemes[2].value == "Main");
    CHECK(lexemes[2].line == 2);
    CHECK(lexemes[3].is(Token::Eof));
}

TEST_CASE("Scanner-ը հասնում է EOF whitespace/comment-ից հետո", "[scanner]")
{
    CHECK(scanAll(" \t\r\n").back().is(Token::Eof));
    CHECK(scanAll("' comment").back().is(Token::Eof));
}

TEST_CASE("Scanner-ը մերժում է EOF-ով չփակված տեքստը", "[scanner]")
{
    const auto lexemes = scanAll("\"unterminated");

    REQUIRE(lexemes.size() == 2);
    CHECK(lexemes[0].is(Token::None));
    CHECK(lexemes[0].value == "unterminated");
    CHECK(lexemes[1].is(Token::Eof));
}

TEST_CASE("Scanner-ը վերադարձնում է None անթույլատրելի նիշի համար", "[scanner]")
{
    const auto lexemes = scanAll("@");

    REQUIRE(lexemes.size() == 2);
    CHECK(lexemes[0].is(Token::None));
    CHECK(lexemes[0].value == "@");
    CHECK(lexemes[1].is(Token::Eof));
}
