#include <catch2/catch_test_macros.hpp>

#include "symbols.hxx"

using namespace avium;

TEST_CASE("SymbolTable declares and resolves variables", "[symbols]")
{
    SymbolTable symbols;
    symbols.openScope();
    ScalarType realType{ScalarType::Name::Real, 1};

    const auto id = symbols.declareVariable("value", realType);

    REQUIRE(id != UnknownSymbol);
    CHECK(symbols.lookup("value") == id);
    CHECK(symbols.declaredInCurrentScope("value"));
    REQUIRE(symbols.variable(id) != nullptr);
    CHECK(symbols.variable(id)->type == &realType);
    CHECK(symbols.subroutine(id) == nullptr);
    CHECK(symbols.symbol(id).name == "value");
}

TEST_CASE("SymbolTable rejects duplicate declarations in one scope", "[symbols]")
{
    SymbolTable symbols;
    symbols.openScope();
    ScalarType realType{ScalarType::Name::Real, 1};
    ScalarType textType{ScalarType::Name::Text, 1};

    CHECK(symbols.declareVariable("value", realType) != UnknownSymbol);
    CHECK(symbols.declareVariable("value", textType) == UnknownSymbol);
    CHECK(symbols.size() == 1);
}

TEST_CASE("SymbolTable resolves the innermost declaration", "[symbols]")
{
    SymbolTable symbols;
    symbols.openScope();
    ScalarType realType{ScalarType::Name::Real, 1};
    ScalarType textType{ScalarType::Name::Text, 1};
    const auto outer = symbols.declareVariable("value", realType);
    symbols.openScope();
    const auto inner = symbols.declareVariable("value", textType);

    CHECK(symbols.lookup("value") == inner);
    REQUIRE(symbols.closeScope());
    CHECK(symbols.lookup("value") == outer);
    REQUIRE(symbols.closeScope());
    CHECK_FALSE(symbols.closeScope());
}

TEST_CASE("Subroutine lookup ignores a same-named local variable", "[symbols]")
{
    SymbolTable symbols;
    ScalarType realType{ScalarType::Name::Real, 1};
    const auto subroutine = symbols.declareSubroutine({"Value", {{}, &realType}});
    symbols.openScope();
    const auto variable = symbols.declareVariable("Value", realType);

    CHECK(symbols.lookup("Value") == variable);
    CHECK(symbols.lookupSubroutine("Value") == subroutine);
    REQUIRE(symbols.subroutine(subroutine) != nullptr);
    CHECK(symbols.subroutine(subroutine)->signature.returnType == &realType);
    CHECK(symbols.variable(subroutine) == nullptr);
}

TEST_CASE("Procedure symbols have no value type", "[symbols]")
{
    SymbolTable symbols;
    const auto id = symbols.declareSubroutine({"Work", {{}, nullptr}});

    REQUIRE(id != UnknownSymbol);
    REQUIRE(symbols.subroutine(id) != nullptr);
    CHECK(symbols.subroutine(id)->signature.returnType == nullptr);
}
