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
    CHECK(symbols.symbol(id).type == &realType);
    CHECK(symbols.symbol(id).kind == SymbolKind::Variable);
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
    const auto subroutine = symbols.declareSubroutine({"Value", {}, &realType, false});
    symbols.openScope();
    const auto variable = symbols.declareVariable(
        "Value", realType, VariableStorage::ReturnValue);

    CHECK(symbols.lookup("Value") == variable);
    CHECK(symbols.lookupSubroutine("Value") == subroutine);
    REQUIRE(symbols.symbol(subroutine).subroutine.has_value());
    CHECK(symbols.symbol(subroutine).type == &realType);
    CHECK(symbols.symbol(subroutine).subroutine->returnType == &realType);
}

TEST_CASE("Procedure symbols have no value type", "[symbols]")
{
    SymbolTable symbols;
    const auto id = symbols.declareSubroutine({"Work", {}, nullptr, false});

    REQUIRE(id != UnknownSymbol);
    CHECK(symbols.symbol(id).type == nullptr);
}
