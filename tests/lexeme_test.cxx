#include <catch2/catch_test_macros.hpp>

#include "formatters.hxx"

#include <cstddef>
#include <format>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace avium;

TEST_CASE("Lexeme-ը պահպանում է իր հիմնական տվյալները", "[lexeme]")
{
    Lexeme empty;
    CHECK(empty.kind == Token::None);
    CHECK(empty.value.empty());
    CHECK(empty.line == 0);

    const Lexeme number{Token::RealLit, "42", 3};
    CHECK(number.kind == Token::RealLit);
    CHECK(number.value == "42");
    CHECK(number.line == 3);
}

TEST_CASE("Lexeme::is-ը գտնում է սպասվող token-ները", "[lexeme]")
{
    const Lexeme lexeme{Token::While, "WHILE", 4};

    CHECK(lexeme.is(Token::While));
    CHECK(lexeme.is(Token::For, Token::While, Token::Let));
    CHECK_FALSE(lexeme.is(Token::If));
    CHECK_FALSE(lexeme.is(Token::If, Token::Let));
}

TEST_CASE("Lexeme-ը ստեղծում է diagnostic ներկայացում", "[lexeme]")
{
    CHECK(Lexeme{}.toString() == "<None, , 0>");
    CHECK(Lexeme{Token::Identifier, "Main", 2}.toString() == "<IDENT, Main, 2>");
    CHECK(Lexeme{Token::RealLit, "3.14", 7}.toString() == "<RealLit, 3.14, 7>");
}

TEST_CASE("Կեռասի բոլոր token-ներն ունեն canonical անուն", "[lexeme]")
{
    using TokenName = std::pair<Token, std::string_view>;
    const std::vector<TokenName> names{
        {Token::None, "None"},
        {Token::Identifier, "IDENT"},
        {Token::RealLit, "RealLit"},
        {Token::TextLit, "TextLit"},
        {Token::BoolLit, "BoolLit"},

        {Token::Subroutine, "SUB"},
        {Token::Dim, "DIM"},
        {Token::As, "AS"},
        {Token::Let, "LET"},
        {Token::If, "IF"},
        {Token::Then, "THEN"},
        {Token::ElseIf, "ELSEIF"},
        {Token::Else, "ELSE"},
        {Token::While, "WHILE"},
        {Token::For, "FOR"},
        {Token::To, "TO"},
        {Token::Step, "STEP"},
        {Token::Call, "CALL"},
        {Token::End, "END"},

        {Token::Real, "REAL"},
        {Token::Text, "TEXT"},
        {Token::Bool, "BOOL"},

        {Token::NewLine, "New Line"},
        {Token::Eq, "="},
        {Token::Ne, "<>"},
        {Token::Lt, "<"},
        {Token::Le, "<="},
        {Token::Gt, ">"},
        {Token::Ge, ">="},
        {Token::LeftPar, "("},
        {Token::RightPar, ")"},
        {Token::LeftBrack, "["},
        {Token::RightBrack, "]"},
        {Token::Comma, ","},
        {Token::Add, "+"},
        {Token::Sub, "-"},
        {Token::Amp, "&"},
        {Token::Or, "OR"},
        {Token::Mul, "*"},
        {Token::Div, "/"},
        {Token::Mod, "MOD"},
        {Token::Quot, "\\"},
        {Token::And, "AND"},
        {Token::Pow, "^"},
        {Token::Not, "NOT"},
        {Token::Eof, "Eof"},
    };

    CHECK(names.size() == static_cast<std::size_t>(Token::Eof) + 1);

    for( const auto& [token, name] : names )
        CHECK(std::format("{}", token) == name);
}
