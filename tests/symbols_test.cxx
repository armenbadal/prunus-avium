#include <catch2/catch_test_macros.hpp>

#include "symbols.hxx"

using namespace avium;

TEST_CASE("SymbolTable declares and resolves variables", "[symbols]")
{
    SymbolTable symbols;
    symbols.openScope();

    const auto id = symbols.declareVariable("value", TypeName::Real);

    REQUIRE(id != UnknownSymbol);
    CHECK(symbols.lookup("value") == id);
    CHECK(symbols.declaredInCurrentScope("value"));
    REQUIRE(symbols.symbol(id).type.has_value());
    CHECK(symbols.symbol(id).type == TypeName::Real);
    CHECK(symbols.symbol(id).kind == SymbolKind::Variable);
}

TEST_CASE("SymbolTable rejects duplicate declarations in one scope", "[symbols]")
{
    SymbolTable symbols;
    symbols.openScope();

    CHECK(symbols.declareVariable("value", TypeName::Real) != UnknownSymbol);
    CHECK(symbols.declareVariable("value", TypeName::Text) == UnknownSymbol);
    CHECK(symbols.size() == 1);
}

TEST_CASE("SymbolTable resolves the innermost declaration", "[symbols]")
{
    SymbolTable symbols;
    symbols.openScope();
    const auto outer = symbols.declareVariable("value", TypeName::Real);
    symbols.openScope();
    const auto inner = symbols.declareVariable("value", TypeName::Text);

    CHECK(symbols.lookup("value") == inner);
    REQUIRE(symbols.closeScope());
    CHECK(symbols.lookup("value") == outer);
    REQUIRE(symbols.closeScope());
    CHECK_FALSE(symbols.closeScope());
}

TEST_CASE("Subroutine lookup ignores a same-named local variable", "[symbols]")
{
    SymbolTable symbols;
    const auto subroutine = symbols.declareSubroutine({"Value", {}, TypeName::Real, false});
    symbols.openScope();
    const auto variable = symbols.declareVariable("Value", TypeName::Real,
        false, VariableStorage::ReturnValue);

    CHECK(symbols.lookup("Value") == variable);
    CHECK(symbols.lookupSubroutine("Value") == subroutine);
    REQUIRE(symbols.symbol(subroutine).subroutine.has_value());
    CHECK(symbols.symbol(subroutine).type == TypeName::Real);
    CHECK(symbols.symbol(subroutine).subroutine->returnType == TypeName::Real);
}

TEST_CASE("Procedure symbols have no value type", "[symbols]")
{
    SymbolTable symbols;
    const auto id = symbols.declareSubroutine({"Work", {}, std::nullopt, false});

    REQUIRE(id != UnknownSymbol);
    CHECK_FALSE(symbols.symbol(id).type.has_value());
}
