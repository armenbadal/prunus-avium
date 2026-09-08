#include <catch2/catch_test_macros.hpp>

#include "formatters.hxx"

#include <format>

using namespace avium;

TEST_CASE("TypeName-ը ձևաչափվում է std::format-ով", "[formatters]")
{
    CHECK(std::format("{}", TypeName::Bool) == "BOOL");
    CHECK(std::format("{}", TypeName::Real) == "REAL");
    CHECK(std::format("{}", TypeName::Text) == "TEXT");
    CHECK(std::format("{:>7}", TypeName::Real) == "   REAL");
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
