#include <catch2/catch_test_macros.hpp>

#include "formatters.hxx"

#include <format>
#include <memory>

using namespace avium;

TEST_CASE("Lexeme-ը ձևաչափվում է diagnostic ներկայացմամբ", "[formatters]")
{
    CHECK(std::format("{}", Lexeme{Token::NewLine, "", 1}) == "տողի ավարտ");
    CHECK(std::format("{}", Lexeme{Token::Eof, "", 1}) == "ֆայլի ավարտ");
    CHECK(std::format("{}", Lexeme{Token::None, "@", 1}) == "անհայտ նիշ '@'");
    CHECK(std::format("{}", Lexeme{Token::Identifier, "value", 1}) == "'value'");
    CHECK(std::format("{}", Lexeme{Token::RealLit, "3.14", 1}) == "'3.14'");
    CHECK(std::format("{}", Lexeme{Token::Then, "THEN", 1}) == "'THEN'");
}

TEST_CASE("ScalarType::Name-ը ձևաչափվում է std::format-ով", "[formatters]")
{
    CHECK(std::format("{}", ScalarType::Name::Bool) == "BOOL");
    CHECK(std::format("{}", ScalarType::Name::Real) == "REAL");
    CHECK(std::format("{}", ScalarType::Name::Text) == "TEXT");
    CHECK(std::format("{:>7}", ScalarType::Name::Real) == "   REAL");
}

TEST_CASE("Type-ը ձևաչափվում է std::format-ով", "[formatters]")
{
    ScalarType scalar{ScalarType::Name::Bool, 1};
    ArrayType array{std::make_unique<ScalarType>(ScalarType::Name::Text, 1), nullptr, 1};

    CHECK(std::format("{}", static_cast<const Type&>(scalar)) == "BOOL");
    CHECK(std::format("{}", static_cast<const Type&>(array)) == "TEXT[]");
}

TEST_CASE("Operation-ը ձևաչափվում է std::format-ով", "[formatters]")
{
    CHECK(std::format("{}", Operation::None) == "?");
    CHECK(std::format("{}", Operation::Add) == "+");
    CHECK(std::format("{}", Operation::Sub) == "-");
    CHECK(std::format("{}", Operation::Mul) == "*");
    CHECK(std::format("{}", Operation::Div) == "/");
    CHECK(std::format("{}", Operation::Quot) == "\\");
    CHECK(std::format("{}", Operation::Mod) == "MOD");
    CHECK(std::format("{}", Operation::Pow) == "^");
    CHECK(std::format("{}", Operation::Eq) == "=");
    CHECK(std::format("{}", Operation::Ne) == "<>");
    CHECK(std::format("{}", Operation::Gt) == ">");
    CHECK(std::format("{}", Operation::Ge) == ">=");
    CHECK(std::format("{}", Operation::Lt) == "<");
    CHECK(std::format("{}", Operation::Le) == "<=");
    CHECK(std::format("{}", Operation::And) == "AND");
    CHECK(std::format("{}", Operation::Or) == "OR");
    CHECK(std::format("{}", Operation::Not) == "NOT");
    CHECK(std::format("{}", Operation::Conc) == "&");
    CHECK(std::format("{}", Operation::Index) == "[]");
    CHECK(std::format("{:>3}", Operation::Add) == "  +");
}
