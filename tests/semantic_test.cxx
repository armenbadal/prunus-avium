#include <catch2/catch_test_macros.hpp>

#include "semantic.hxx"
#include "test_ast.hxx"

#include <optional>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

using namespace avium;
using test::NodeList;

namespace {

Subroutine::Ptr subroutine(std::string_view name,
    NodeList<Parameter> parameters = {},
    std::optional<TypeName> returnType = std::nullopt, Position line = 1)
{
    std::vector<Statement::Ptr> statements;
    if( returnType.has_value() && *returnType != TypeName::Unknown ) {
        Expression::Ptr value;
        switch( *returnType ) {
            case TypeName::Bool:
                value = node<Boolean>(false, line);
                break;
            case TypeName::Real:
                value = node<Number>(0.0, line);
                break;
            case TypeName::Text:
                value = node<Text>("", line);
                break;
            case TypeName::Unknown:
                break;
        }
        statements.push_back(node<Return>(std::move(value), line));
    }
    return node<Subroutine>(name, std::move(parameters), returnType,
        node<Sequence>(std::move(statements), line), line);
}

Subroutine::Ptr subroutineWithBody(std::string_view name,
    NodeList<Statement> statements,
    NodeList<Parameter> parameters = {},
    std::optional<TypeName> returnType = std::nullopt, Position line = 1)
{
    return node<Subroutine>(name, std::move(parameters), returnType,
        node<Sequence>(std::move(statements), line), line);
}

struct AnalysisResult {
    bool valid;
    std::vector<Error> errors;
};

AnalysisResult analyze(NodeList<Subroutine> subroutines)
{
    auto program = node<Program>(std::move(subroutines), 1);
    SymbolTable symbols;
    SemanticModel model;
    Diagnostics diagnostics;
    SemanticAnalyzer analyzer{symbols, model, diagnostics};
    const auto valid = analyzer.analyze(*program);
    return {valid, diagnostics.errors()};
}

AnalysisResult analyzeExpression(Expression::Ptr expression, TypeName resultType)
{
    auto declaration = node<Dim>("result", nullptr, resultType, false, 1);
    auto assignment = node<Let>(node<Variable>("result", expression->line), nullptr,
        std::move(expression), 1);
    return analyze({subroutineWithBody(
        "Main", {std::move(declaration), std::move(assignment)})});
}

} // namespace

TEST_CASE("Semantic analyzer accepts one parameterless procedure Main", "[semantic]")
{
    const auto result = analyze({subroutine("Main")});

    CHECK(result.valid);
    CHECK(result.errors.empty());
}

TEST_CASE("Semantic analyzer requires Main", "[semantic]")
{
    const auto result = analyze({subroutine("Helper")});

    CHECK_FALSE(result.valid);
    CHECK(result.errors.size() == 1);
}

TEST_CASE("Semantic analyzer rejects duplicate Main subroutines", "[semantic]")
{
    const auto result = analyze({subroutine("Main"), subroutine("Main", {}, std::nullopt, 4)});

    CHECK_FALSE(result.valid);
    CHECK(result.errors.size() == 1);
    CHECK(std::get<0>(result.errors.front()) == 4);
}

TEST_CASE("Semantic analyzer rejects Main parameters and return type", "[semantic]")
{
    NodeList<Parameter> parameters{
        node<Parameter>("value", nullptr, TypeName::Real, false, 1)};
    const auto result = analyze({subroutine("Main", std::move(parameters), TypeName::Real)});

    CHECK_FALSE(result.valid);
    CHECK(result.errors.size() == 2);
}

TEST_CASE("Semantic analyzer declares subroutine signatures", "[semantic]")
{
    auto main = subroutine("Main");
    NodeList<Parameter> parameters{
        node<Parameter>("items", nullptr, TypeName::Text, true, 2)};
    auto printItems = subroutine("PrintItems", std::move(parameters), std::nullopt, 2);
    const auto printItemsId = printItems->id();
    auto program = node<Program>(
        NodeList<Subroutine>{std::move(main), std::move(printItems)}, 1);
    SymbolTable symbols;
    SemanticModel model;
    Diagnostics diagnostics;
    SemanticAnalyzer analyzer{symbols, model, diagnostics};

    REQUIRE(analyzer.analyze(*program));
    const auto id = model.symbol(printItemsId);
    REQUIRE(id.has_value());
    const auto& symbol = symbols.symbol(*id);
    REQUIRE(symbol.subroutine.has_value());
    REQUIRE(symbol.subroutine->parameters.size() == 1);
    CHECK(symbol.subroutine->parameters[0].type == TypeName::Text);
    CHECK(symbol.subroutine->parameters[0].isArray);
}

TEST_CASE("Semantic analyzer rejects duplicate subroutine names", "[semantic]")
{
    auto main = subroutine("Main");
    auto first = subroutine("Work");
    auto duplicate = subroutine("Work", {}, std::nullopt, 7);
    const auto result = analyze({std::move(main), std::move(first), std::move(duplicate)});

    CHECK_FALSE(result.valid);
    REQUIRE(result.errors.size() == 1);
    CHECK(std::get<0>(result.errors.front()) == 7);
}

TEST_CASE("Semantic analyzer reserves builtin subroutine names", "[semantic]")
{
    for( const auto name : {"Print", "Input", "NUM"} ) {
        DYNAMIC_SECTION(name)
        {
            auto main = subroutine("Main");
            auto replacement = subroutine(name, {}, std::nullopt, 5);
            const auto result = analyze({std::move(main), std::move(replacement)});
            CHECK_FALSE(result.valid);
            REQUIRE(result.errors.size() == 1);
            CHECK(std::get<0>(result.errors.front()) == 5);
        }
    }
}

TEST_CASE("Semantic analyzer rejects unknown signature types", "[semantic]")
{
    NodeList<Parameter> parameters{
        node<Parameter>("value", nullptr, TypeName::Unknown, false, 3)};
    const auto result = analyze({subroutine("Main"),
        subroutine("Broken", std::move(parameters), TypeName::Unknown, 3)});

    CHECK_FALSE(result.valid);
    CHECK(result.errors.size() == 2);
}

TEST_CASE("Semantic analyzer binds local declarations and uses", "[semantic]")
{
    auto declaration = node<Dim>("value", nullptr, TypeName::Real, false, 2);
    const auto declarationId = declaration->id();
    auto target = node<Variable>("value", 3);
    const auto targetId = target->id();
    auto source = node<Variable>("value", 3);
    const auto sourceId = source->id();
    auto assignment = node<Let>(std::move(target), nullptr, std::move(source), 3);
    auto main = subroutineWithBody(
        "Main", {std::move(declaration), std::move(assignment)});
    auto program = node<Program>(NodeList<Subroutine>{std::move(main)}, 1);
    SymbolTable symbols;
    SemanticModel model;
    Diagnostics diagnostics;
    SemanticAnalyzer analyzer{symbols, model, diagnostics};

    REQUIRE(analyzer.analyze(*program));
    const auto id = model.symbol(declarationId);
    REQUIRE(id.has_value());
    CHECK(model.symbol(targetId) == id);
    CHECK(model.symbol(sourceId) == id);
}

TEST_CASE("Semantic analyzer rejects an undefined variable", "[semantic]")
{
    auto assignment = node<Let>(node<Variable>("missing", 2), nullptr,
        node<Number>(1.0, 2), 2);
    const auto result = analyze({subroutineWithBody("Main", {std::move(assignment)})});

    CHECK_FALSE(result.valid);
    CHECK(result.errors.size() == 1);
}

TEST_CASE("Semantic analyzer rejects duplicate names across nested blocks", "[semantic]")
{
    auto outer = node<Dim>("value", nullptr, TypeName::Real, false, 2);
    auto inner = node<Dim>("value", nullptr, TypeName::Text, false, 4);
    auto branch = node<IfBranch>(node<Boolean>(true, 3),
        node<Sequence>(NodeList<Statement>{std::move(inner)}, 3), 3);
    auto conditional = node<If>(NodeList<IfBranch>{std::move(branch)}, nullptr, 3);
    const auto result = analyze({subroutineWithBody(
        "Main", {std::move(outer), std::move(conditional)})});

    CHECK_FALSE(result.valid);
    REQUIRE(result.errors.size() == 1);
    CHECK(std::get<0>(result.errors.front()) == 4);
}

TEST_CASE("A parameter may have the function name", "[semantic]")
{
    NodeList<Parameter> parameters{
        node<Parameter>("Value", nullptr, TypeName::Real, false, 2)};
    auto returnStatement = node<Return>(node<Variable>("Value", 3), 3);
    const auto result = analyze({subroutine("Main"),
        subroutineWithBody("Value", {std::move(returnStatement)},
            std::move(parameters), TypeName::Real, 2)});

    CHECK(result.valid);
    CHECK(result.errors.empty());
}

TEST_CASE("A function name is not an implicit return variable", "[semantic]")
{
    auto assignment = node<Let>(node<Variable>("Value", 3), nullptr,
        node<Number>(1.0, 3), 3);
    const auto result = analyze({subroutine("Main"),
        subroutineWithBody("Value", {std::move(assignment)}, {},
            TypeName::Real, 2)});

    CHECK_FALSE(result.valid);
    CHECK(result.errors.size() == 2);
}

TEST_CASE("Implicit FOR variable is visible in the whole subroutine", "[semantic]")
{
    auto declaration = node<Dim>("result", nullptr, TypeName::Real, false, 2);
    auto use = node<Variable>("index", 3);
    const auto useId = use->id();
    auto assignment = node<Let>(
        node<Variable>("result", 3), nullptr, std::move(use), 3);
    auto parameter = node<Variable>("index", 4);
    const auto parameterId = parameter->id();
    auto loop = node<For>(std::move(parameter), node<Number>(1.0, 4),
        node<Number>(3.0, 4), node<Number>(1.0, 4),
        node<Sequence>(NodeList<Statement>{}, 4), 4);
    auto main = subroutineWithBody("Main",
        {std::move(declaration), std::move(assignment), std::move(loop)});
    auto program = node<Program>(NodeList<Subroutine>{std::move(main)}, 1);
    SymbolTable symbols;
    SemanticModel model;
    Diagnostics diagnostics;
    SemanticAnalyzer analyzer{symbols, model, diagnostics};

    REQUIRE(analyzer.analyze(*program));
    const auto id = model.symbol(parameterId);
    REQUIRE(id.has_value());
    CHECK(model.symbol(useId) == id);
    CHECK(symbols.symbol(*id).storage == VariableStorage::ForVariable);
}

TEST_CASE("FOR reuses only a scalar REAL variable", "[semantic]")
{
    auto declaration = node<Dim>("index", nullptr, TypeName::Text, false, 2);
    auto loop = node<For>(node<Variable>("index", 3), node<Number>(1.0, 3),
        node<Number>(3.0, 3), node<Number>(1.0, 3),
        node<Sequence>(NodeList<Statement>{}, 3), 3);
    const auto result = analyze({subroutineWithBody(
        "Main", {std::move(declaration), std::move(loop)})});

    CHECK_FALSE(result.valid);
    CHECK(result.errors.size() == 1);
}

TEST_CASE("Semantic analyzer accepts a compatible scalar assignment", "[semantic]")
{
    auto declaration = node<Dim>("value", nullptr, TypeName::Real, false, 2);
    auto number = node<Number>(42.0, 3);
    const auto numberId = number->id();
    auto assignment = node<Let>(
        node<Variable>("value", 3), nullptr, std::move(number), 3);
    auto main = subroutineWithBody(
        "Main", {std::move(declaration), std::move(assignment)});
    auto program = node<Program>(NodeList<Subroutine>{std::move(main)}, 1);
    SymbolTable symbols;
    SemanticModel model;
    Diagnostics diagnostics;
    SemanticAnalyzer analyzer{symbols, model, diagnostics};

    REQUIRE(analyzer.analyze(*program));
    CHECK(model.type(numberId) == TypeName::Real);
}

TEST_CASE("Semantic analyzer rejects an assignment type mismatch", "[semantic]")
{
    auto declaration = node<Dim>("value", nullptr, TypeName::Real, false, 2);
    auto assignment = node<Let>(node<Variable>("value", 3), nullptr,
        node<Text>("wrong", 3), 3);
    const auto result = analyze({subroutineWithBody(
        "Main", {std::move(declaration), std::move(assignment)})});

    CHECK_FALSE(result.valid);
    REQUIRE(result.errors.size() == 1);
    CHECK(std::get<1>(result.errors.front()) == "'value' փոփոխականին պետք է վերագրվի REAL, բայց ստացվել է TEXT։");
}

TEST_CASE("Semantic analyzer rejects indexing a scalar assignment target", "[semantic]")
{
    auto declaration = node<Dim>("value", nullptr, TypeName::Real, false, 2);
    auto assignment = node<Let>(node<Variable>("value", 3), node<Number>(0.0, 3),
        node<Number>(1.0, 3), 3);
    const auto result = analyze({subroutineWithBody(
        "Main", {std::move(declaration), std::move(assignment)})});

    CHECK_FALSE(result.valid);
    CHECK(result.errors.size() == 1);
}

TEST_CASE("Semantic analyzer rejects assigning a whole array", "[semantic]")
{
    auto array = node<Dim>("items", node<Number>(3.0, 2), TypeName::Real, true, 2);
    auto assignment = node<Let>(node<Variable>("items", 3), nullptr,
        node<Number>(1.0, 3), 3);
    const auto result = analyze({subroutineWithBody(
        "Main", {std::move(array), std::move(assignment)})});

    CHECK_FALSE(result.valid);
    CHECK(result.errors.size() == 1);
}

TEST_CASE("Semantic analyzer rejects an array as a scalar value", "[semantic]")
{
    auto array = node<Dim>("items", node<Number>(3.0, 2), TypeName::Real, true, 2);
    auto scalar = node<Dim>("value", nullptr, TypeName::Real, false, 3);
    auto assignment = node<Let>(node<Variable>("value", 4), nullptr,
        node<Variable>("items", 4), 4);
    const auto result = analyze({subroutineWithBody("Main",
        {std::move(array), std::move(scalar), std::move(assignment)})});

    CHECK_FALSE(result.valid);
    CHECK(result.errors.size() == 1);
}

TEST_CASE("RETURN uses the declared function return type", "[semantic]")
{
    auto validReturn = node<Return>(node<Number>(1.0, 3), 3);
    auto invalidReturn = node<Return>(node<Text>("wrong", 6), 6);
    const auto result = analyze({subroutine("Main"),
        subroutineWithBody(
            "Value", {std::move(validReturn)}, {}, TypeName::Real, 2),
        subroutineWithBody(
            "Broken", {std::move(invalidReturn)}, {}, TypeName::Real, 5)});

    CHECK_FALSE(result.valid);
    REQUIRE(result.errors.size() == 1);
    CHECK(std::get<1>(result.errors.front()) == "Ֆունկցիայից պետք է վերադարձվի REAL, բայց ստացվել է TEXT։");
}

TEST_CASE("RETURN is accepted only in a function", "[semantic]")
{
    auto returnStatement = node<Return>(node<Number>(1.0, 2), 2);
    const auto result = analyze({subroutineWithBody(
        "Main", {std::move(returnStatement)})});

    CHECK_FALSE(result.valid);
    REQUIRE(result.errors.size() == 1);
    CHECK(std::get<1>(result.errors.front()) == "RETURN հրամանը թույլատրելի է միայն ֆունկցիայում։");
}

TEST_CASE("RETURN requires a scalar value", "[semantic]")
{
    NodeList<Parameter> parameters{
        node<Parameter>("items", nullptr, TypeName::Real, true, 2)};
    auto returnStatement = node<Return>(node<Variable>("items", 3), 3);
    const auto result = analyze({subroutine("Main"),
        subroutineWithBody("First", {std::move(returnStatement)},
            std::move(parameters), TypeName::Real, 2)});

    CHECK_FALSE(result.valid);
    CHECK(result.errors.size() == 1);
}

TEST_CASE("A function must return on every execution path", "[semantic]")
{
    SECTION("missing RETURN")
    {
        const auto result = analyze({subroutine("Main"),
            subroutineWithBody("Value", {}, {}, TypeName::Real, 2)});

        CHECK_FALSE(result.valid);
        REQUIRE(result.errors.size() == 1);
        CHECK(std::get<1>(result.errors.front()) == "'Value' ֆունկցիայի ոչ բոլոր կատարման ուղիներն են արժեք վերադարձնում։");
    }

    SECTION("complete IF")
    {
        auto firstReturn = node<Return>(node<Number>(1.0, 4), 4);
        auto branch = node<IfBranch>(node<Boolean>(true, 3),
            node<Sequence>(NodeList<Statement>{std::move(firstReturn)}, 3), 3);
        auto secondReturn = node<Return>(node<Number>(2.0, 6), 6);
        auto alternative = node<Sequence>(
            NodeList<Statement>{std::move(secondReturn)}, 5);
        auto conditional = node<If>(NodeList<IfBranch>{std::move(branch)},
            std::move(alternative), 3);
        const auto result = analyze({subroutine("Main"),
            subroutineWithBody("Value", {std::move(conditional)}, {},
                TypeName::Real, 2)});

        CHECK(result.valid);
        CHECK(result.errors.empty());
    }

    SECTION("IF without ELSE")
    {
        auto returnStatement = node<Return>(node<Number>(1.0, 4), 4);
        auto branch = node<IfBranch>(node<Boolean>(true, 3),
            node<Sequence>(NodeList<Statement>{std::move(returnStatement)}, 3), 3);
        auto conditional = node<If>(
            NodeList<IfBranch>{std::move(branch)}, nullptr, 3);
        const auto result = analyze({subroutine("Main"),
            subroutineWithBody("Value", {std::move(conditional)}, {},
                TypeName::Real, 2)});

        CHECK_FALSE(result.valid);
        CHECK(result.errors.size() == 1);
    }
}

TEST_CASE("Assignment checks an inferred expression result type", "[semantic]")
{
    auto declaration = node<Dim>("value", nullptr, TypeName::Real, false, 2);
    auto expression = node<Binary>(Operation::Conc, node<Text>("a", 3),
        node<Text>("b", 3), 3);
    auto assignment = node<Let>(node<Variable>("value", 3), nullptr,
        std::move(expression), 3);
    const auto result = analyze({subroutineWithBody(
        "Main", {std::move(declaration), std::move(assignment)})});

    CHECK_FALSE(result.valid);
    CHECK(result.errors.size() == 1);
}

TEST_CASE("Semantic analyzer accepts well-typed operators", "[semantic]")
{
    SECTION("boolean operators")
    {
        auto negation = node<Unary>(Operation::Not, node<Boolean>(false, 2), 2);
        auto conjunction = node<Binary>(Operation::And, node<Boolean>(true, 2),
            std::move(negation), 2);
        auto expression = node<Binary>(Operation::Or, std::move(conjunction),
            node<Boolean>(false, 2), 2);
        CHECK(analyzeExpression(std::move(expression), TypeName::Bool).valid);
    }

    SECTION("numeric operators")
    {
        auto product = node<Binary>(Operation::Mul, node<Number>(2.0, 2),
            node<Number>(3.0, 2), 2);
        auto expression = node<Binary>(Operation::Add, node<Number>(1.0, 2),
            std::move(product), 2);
        CHECK(analyzeExpression(std::move(expression), TypeName::Real).valid);
    }

    SECTION("text comparison")
    {
        auto expression = node<Binary>(Operation::Lt, node<Text>("a", 2),
            node<Text>("b", 2), 2);
        CHECK(analyzeExpression(std::move(expression), TypeName::Bool).valid);
    }
}

TEST_CASE("Semantic analyzer rejects invalid unary operands", "[semantic]")
{
    SECTION("NOT requires BOOL")
    {
        auto expression = node<Unary>(Operation::Not, node<Number>(1.0, 2), 2);
        const auto result = analyzeExpression(std::move(expression), TypeName::Bool);
        CHECK_FALSE(result.valid);
        REQUIRE(result.errors.size() == 1);
        CHECK(std::get<1>(result.errors.front()) == "'NOT' գործողության օպերանդը պետք է լինի BOOL, բայց ստացվել է REAL։");
    }

    SECTION("unary minus requires REAL")
    {
        auto expression = node<Unary>(Operation::Sub, node<Text>("text", 2), 2);
        const auto result = analyzeExpression(std::move(expression), TypeName::Real);
        CHECK_FALSE(result.valid);
        REQUIRE(result.errors.size() == 1);
        CHECK(std::get<1>(result.errors.front()) == "Ունար '-' գործողության օպերանդը պետք է լինի REAL, բայց ստացվել է TEXT։");
    }
}

TEST_CASE("Semantic analyzer suppresses a unary type error for an unknown operand", "[semantic]")
{
    auto operand = node<Variable>("missing", 2);
    auto expression = node<Unary>(Operation::Not, std::move(operand), 2);
    const auto result = analyzeExpression(std::move(expression), TypeName::Bool);

    CHECK_FALSE(result.valid);
    REQUIRE(result.errors.size() == 1);
    CHECK(std::get<1>(result.errors.front()) == "'missing' անունով փոփոխական սահմանված չէ։");
}

TEST_CASE("Semantic analyzer rejects invalid binary operands", "[semantic]")
{
    SECTION("arithmetic requires REAL")
    {
        auto expression = node<Binary>(Operation::Add, node<Text>("text", 2),
            node<Number>(1.0, 2), 2);
        const auto result = analyzeExpression(std::move(expression), TypeName::Real);
        CHECK_FALSE(result.valid);
        REQUIRE(result.errors.size() == 1);
        CHECK(std::get<1>(result.errors.front()) == "'+' գործողության ձախ օպերանդը պետք է լինի REAL, բայց ստացվել է TEXT։");
    }

    SECTION("concatenation requires TEXT")
    {
        auto expression = node<Binary>(Operation::Conc, node<Text>("text", 2),
            node<Number>(1.0, 2), 2);
        const auto result = analyzeExpression(std::move(expression), TypeName::Text);
        CHECK_FALSE(result.valid);
        CHECK(result.errors.size() == 1);
    }

    SECTION("logical operators require BOOL")
    {
        auto expression = node<Binary>(Operation::And, node<Boolean>(true, 2),
            node<Number>(1.0, 2), 2);
        const auto result = analyzeExpression(std::move(expression), TypeName::Bool);
        CHECK_FALSE(result.valid);
        CHECK(result.errors.size() == 1);
    }

    SECTION("equality requires matching types")
    {
        auto expression = node<Binary>(Operation::Eq, node<Number>(1.0, 2),
            node<Text>("1", 2), 2);
        const auto result = analyzeExpression(std::move(expression), TypeName::Bool);
        CHECK_FALSE(result.valid);
        CHECK(result.errors.size() == 1);
    }

    SECTION("comparison accepts only matching REAL or TEXT values")
    {
        auto expression = node<Binary>(Operation::Lt, node<Boolean>(false, 2),
            node<Boolean>(true, 2), 2);
        const auto result = analyzeExpression(std::move(expression), TypeName::Bool);
        CHECK_FALSE(result.valid);
        CHECK(result.errors.size() == 1);
    }
}

TEST_CASE("Semantic analyzer rejects arrays in scalar operators", "[semantic]")
{
    auto array = node<Dim>("items", node<Number>(3.0, 1), TypeName::Real, true, 1);
    auto result = node<Dim>("result", nullptr, TypeName::Real, false, 1);
    auto expression = node<Binary>(Operation::Add, node<Variable>("items", 2),
        node<Number>(1.0, 2), 2);
    auto assignment = node<Let>(node<Variable>("result", 2), nullptr,
        std::move(expression), 2);
    const auto analysis = analyze({subroutineWithBody("Main",
        {std::move(array), std::move(result), std::move(assignment)})});

    CHECK_FALSE(analysis.valid);
    CHECK(analysis.errors.size() == 1);
}

TEST_CASE("Semantic analyzer accepts well-typed control structures", "[semantic]")
{
    auto ifBranch = node<IfBranch>(node<Boolean>(true, 2),
        node<Sequence>(NodeList<Statement>{}, 2), 2);
    auto conditional = node<If>(NodeList<IfBranch>{std::move(ifBranch)}, nullptr, 2);
    auto whileLoop = node<While>(node<Boolean>(true, 3),
        node<Sequence>(NodeList<Statement>{}, 3), 3);
    auto forLoop = node<For>(node<Variable>("index", 4), node<Number>(1.0, 4),
        node<Number>(3.0, 4), node<Number>(1.0, 4),
        node<Sequence>(NodeList<Statement>{}, 4), 4);
    const auto result = analyze({subroutineWithBody("Main",
        {std::move(conditional), std::move(whileLoop), std::move(forLoop)})});

    CHECK(result.valid);
    CHECK(result.errors.empty());
}

TEST_CASE("Semantic analyzer requires BOOL branch conditions", "[semantic]")
{
    auto first = node<IfBranch>(node<Number>(1.0, 2),
        node<Sequence>(NodeList<Statement>{}, 2), 2);
    auto second = node<IfBranch>(node<Text>("yes", 3),
        node<Sequence>(NodeList<Statement>{}, 3), 3);
    auto conditional = node<If>(
        NodeList<IfBranch>{std::move(first), std::move(second)}, nullptr, 2);
    const auto result = analyze({subroutineWithBody("Main", {std::move(conditional)})});

    CHECK_FALSE(result.valid);
    REQUIRE(result.errors.size() == 2);
    CHECK(std::get<1>(result.errors[0]) == "Պայմանական ճյուղի պայմանը պետք է լինի BOOL, բայց ստացվել է REAL։");
    CHECK(std::get<1>(result.errors[1]) == "Պայմանական ճյուղի պայմանը պետք է լինի BOOL, բայց ստացվել է TEXT։");
}

TEST_CASE("Semantic analyzer requires a BOOL WHILE condition", "[semantic]")
{
    auto loop = node<While>(node<Text>("yes", 2),
        node<Sequence>(NodeList<Statement>{}, 2), 2);
    const auto result = analyze({subroutineWithBody("Main", {std::move(loop)})});

    CHECK_FALSE(result.valid);
    REQUIRE(result.errors.size() == 1);
    CHECK(std::get<1>(result.errors.front()) == "WHILE-ի պայմանը պետք է լինի BOOL, բայց ստացվել է TEXT։");
}

TEST_CASE("Semantic analyzer checks FOR bounds and step", "[semantic]")
{
    auto loop = node<For>(node<Variable>("index", 2), node<Text>("first", 2),
        node<Boolean>(true, 2), node<Number>(0.0, 2),
        node<Sequence>(NodeList<Statement>{}, 2), 2);
    const auto result = analyze({subroutineWithBody("Main", {std::move(loop)})});

    CHECK_FALSE(result.valid);
    CHECK(result.errors.size() == 3);
}

TEST_CASE("Semantic analyzer rejects an array as a condition", "[semantic]")
{
    auto array = node<Dim>("flags", node<Number>(2.0, 1), TypeName::Bool, true, 1);
    auto branch = node<IfBranch>(node<Variable>("flags", 2),
        node<Sequence>(NodeList<Statement>{}, 2), 2);
    auto conditional = node<If>(NodeList<IfBranch>{std::move(branch)}, nullptr, 2);
    const auto result = analyze({subroutineWithBody(
        "Main", {std::move(array), std::move(conditional)})});

    CHECK_FALSE(result.valid);
    CHECK(result.errors.size() == 1);
}

TEST_CASE("Semantic analyzer accepts valid array sizes", "[semantic]")
{
    SECTION("constant expression")
    {
        auto size = node<Binary>(Operation::Add, node<Number>(2.0, 1),
            node<Number>(3.0, 1), 1);
        auto array = node<Dim>("items", std::move(size), TypeName::Real, true, 1);
        const auto result = analyze({subroutineWithBody("Main", {std::move(array)})});
        CHECK(result.valid);
    }

    SECTION("dynamic expression")
    {
        auto length = node<Dim>("length", nullptr, TypeName::Real, false, 1);
        auto size = node<Variable>("length", 2);
        auto array = node<Dim>("items", std::move(size), TypeName::Real, true, 2);
        const auto result = analyze({subroutineWithBody(
            "Main", {std::move(length), std::move(array)})});
        CHECK(result.valid);
    }
}

TEST_CASE("Semantic analyzer requires a scalar REAL array size", "[semantic]")
{
    SECTION("wrong type")
    {
        auto size = node<Text>("large", 1);
        auto array = node<Dim>("items", std::move(size), TypeName::Real, true, 1);
        const auto result = analyze({subroutineWithBody("Main", {std::move(array)})});
        CHECK_FALSE(result.valid);
        REQUIRE(result.errors.size() == 1);
        CHECK(std::get<1>(result.errors.front()) == "Զանգվածի չափը պետք է լինի REAL, բայց ստացվել է TEXT։");
    }

    SECTION("array value")
    {
        auto sizes = node<Dim>("sizes", node<Number>(2.0, 1),
            TypeName::Real, true, 1);
        auto size = node<Variable>("sizes", 2);
        auto items = node<Dim>("items", std::move(size), TypeName::Real, true, 2);
        const auto result = analyze({subroutineWithBody(
            "Main", {std::move(sizes), std::move(items)})});
        CHECK_FALSE(result.valid);
        CHECK(result.errors.size() == 1);
    }
}

TEST_CASE("Semantic analyzer validates constant array sizes", "[semantic]")
{
    for( const auto kind : {0, 1, 2} ) {
        Expression::Ptr size;
        if( kind == 0 ) {
            size = node<Number>(0.0, 1);
        }
        else if( kind == 1 ) {
            size = node<Number>(2.5, 1);
        }
        else {
            size = node<Unary>(Operation::Sub, node<Number>(1.0, 1), 1);
        }
        auto array = node<Dim>("items", std::move(size), TypeName::Real, true, 1);
        const auto result = analyze({subroutineWithBody("Main", {std::move(array)})});
        CHECK_FALSE(result.valid);
        CHECK(result.errors.size() == 1);
    }
}

TEST_CASE("Semantic analyzer requires a local array size", "[semantic]")
{
    auto array = node<Dim>("items", nullptr, TypeName::Real, true, 1);
    const auto result = analyze({subroutineWithBody("Main", {std::move(array)})});

    CHECK_FALSE(result.valid);
    CHECK(result.errors.size() == 1);
}

TEST_CASE("Array access has the array element type", "[semantic]")
{
    auto array = node<Dim>("items", node<Number>(3.0, 1),
        TypeName::Text, true, 1);
    auto scalar = node<Dim>("value", nullptr, TypeName::Text, false, 2);
    auto access = node<Binary>(Operation::Index, node<Variable>("items", 3),
        node<Number>(0.0, 3), 3);
    const auto accessId = access->id();
    auto assignment = node<Let>(node<Variable>("value", 3), nullptr,
        std::move(access), 3);
    auto main = subroutineWithBody(
        "Main", {std::move(array), std::move(scalar), std::move(assignment)});
    auto program = node<Program>(NodeList<Subroutine>{std::move(main)}, 1);
    SymbolTable symbols;
    SemanticModel model;
    Diagnostics diagnostics;
    SemanticAnalyzer analyzer{symbols, model, diagnostics};

    REQUIRE(analyzer.analyze(*program));
    CHECK(model.type(accessId) == TypeName::Text);
}

TEST_CASE("Semantic analyzer rejects indexing a scalar expression", "[semantic]")
{
    auto scalar = node<Dim>("value", nullptr, TypeName::Real, false, 1);
    auto result = node<Dim>("result", nullptr, TypeName::Real, false, 1);
    auto access = node<Binary>(Operation::Index, node<Variable>("value", 2),
        node<Number>(0.0, 2), 2);
    auto assignment = node<Let>(node<Variable>("result", 2), nullptr,
        std::move(access), 2);
    const auto analysis = analyze({subroutineWithBody("Main",
        {std::move(scalar), std::move(result), std::move(assignment)})});

    CHECK_FALSE(analysis.valid);
    CHECK(analysis.errors.size() == 1);
}

TEST_CASE("Semantic analyzer requires a scalar REAL index", "[semantic]")
{
    auto array = node<Dim>("items", node<Number>(3.0, 1),
        TypeName::Real, true, 1);
    auto index = node<Text>("first", 2);
    auto assignment = node<Let>(node<Variable>("items", 2), std::move(index),
        node<Number>(1.0, 2), 2);
    const auto result = analyze({subroutineWithBody(
        "Main", {std::move(array), std::move(assignment)})});

    CHECK_FALSE(result.valid);
    REQUIRE(result.errors.size() == 1);
    CHECK(std::get<1>(result.errors.front()) == "Զանգվածի ինդեքսը պետք է լինի REAL, բայց ստացվել է TEXT։");
}

TEST_CASE("Semantic analyzer binds valid procedure and function calls", "[semantic]")
{
    NodeList<Parameter> printParameters{
        node<Parameter>("message", nullptr, TypeName::Text, false, 5)};
    auto printMessage = subroutine("PrintMessage", std::move(printParameters),
        std::nullopt, 5);
    const auto printMessageId = printMessage->id();

    NodeList<Parameter> doubleParameters{
        node<Parameter>("value", nullptr, TypeName::Real, false, 8)};
    auto doubleValue = subroutine("Double", std::move(doubleParameters),
        TypeName::Real, 8);
    const auto doubleValueId = doubleValue->id();

    auto call = node<Call>("PrintMessage",
        NodeList<Expression>{node<Text>("hello", 2)}, 2);
    const auto callId = call->id();
    auto result = node<Dim>("result", nullptr, TypeName::Real, false, 3);
    auto apply = node<Apply>("Double",
        NodeList<Expression>{node<Number>(2.0, 4)}, 4);
    const auto applyId = apply->id();
    auto assignment = node<Let>(node<Variable>("result", 4), nullptr,
        std::move(apply), 4);
    auto main = subroutineWithBody(
        "Main", {std::move(call), std::move(result), std::move(assignment)});
    auto program = node<Program>(
        NodeList<Subroutine>{std::move(main), std::move(printMessage),
            std::move(doubleValue)},
        1);
    SymbolTable symbols;
    SemanticModel model;
    Diagnostics diagnostics;
    SemanticAnalyzer analyzer{symbols, model, diagnostics};

    REQUIRE(analyzer.analyze(*program));
    CHECK(model.symbol(callId) == model.symbol(printMessageId));
    CHECK(model.symbol(applyId) == model.symbol(doubleValueId));
    CHECK(model.type(applyId) == TypeName::Real);
}

TEST_CASE("Semantic analyzer requires a declared call target", "[semantic]")
{
    auto call = node<Call>("Missing", NodeList<Expression>{}, 2);
    const auto result = analyze({subroutineWithBody("Main", {std::move(call)})});

    CHECK_FALSE(result.valid);
    CHECK(result.errors.size() == 1);
}

TEST_CASE("CALL accepts only procedures", "[semantic]")
{
    auto call = node<Call>("Value", NodeList<Expression>{}, 2);
    const auto result = analyze({subroutineWithBody("Main", {std::move(call)}),
        subroutine("Value", {}, TypeName::Real, 4)});

    CHECK_FALSE(result.valid);
    CHECK(result.errors.size() == 1);
}

TEST_CASE("Function application accepts only functions", "[semantic]")
{
    auto declaration = node<Dim>("result", nullptr, TypeName::Real, false, 2);
    auto apply = node<Apply>("Work", NodeList<Expression>{}, 3);
    auto assignment = node<Let>(node<Variable>("result", 3), nullptr,
        std::move(apply), 3);
    const auto result = analyze({subroutineWithBody(
                                     "Main",
                                     {std::move(declaration), std::move(assignment)}),
        subroutine("Work", {}, std::nullopt, 5)});

    CHECK_FALSE(result.valid);
    CHECK(result.errors.size() == 1);
}

TEST_CASE("Semantic analyzer checks call argument count", "[semantic]")
{
    NodeList<Parameter> parameters{
        node<Parameter>("first", nullptr, TypeName::Real, false, 4),
        node<Parameter>("second", nullptr, TypeName::Real, false, 4)};
    auto call = node<Call>("Add",
        NodeList<Expression>{node<Number>(1.0, 2)}, 2);
    const auto result = analyze({subroutineWithBody("Main", {std::move(call)}),
        subroutine("Add", std::move(parameters), std::nullopt, 4)});

    CHECK_FALSE(result.valid);
    CHECK(result.errors.size() == 1);
}

TEST_CASE("Semantic analyzer checks scalar argument types", "[semantic]")
{
    NodeList<Parameter> parameters{
        node<Parameter>("value", nullptr, TypeName::Real, false, 4)};
    auto call = node<Call>("UseNumber",
        NodeList<Expression>{node<Text>("wrong", 2)}, 2);
    const auto result = analyze({subroutineWithBody("Main", {std::move(call)}),
        subroutine("UseNumber", std::move(parameters), std::nullopt, 4)});

    CHECK_FALSE(result.valid);
    REQUIRE(result.errors.size() == 1);
    CHECK(std::get<1>(result.errors.front()) == "'UseNumber' ենթածրագրի թիվ 1 արգումենտը պետք է լինի REAL, բայց ստացվել է TEXT։");
}

TEST_CASE("Semantic analyzer checks argument shapes", "[semantic]")
{
    SECTION("array parameter requires an array")
    {
        NodeList<Parameter> parameters{
            node<Parameter>("items", nullptr, TypeName::Real, true, 5)};
        auto value = node<Dim>("value", nullptr, TypeName::Real, false, 2);
        auto call = node<Call>("UseItems",
            NodeList<Expression>{node<Variable>("value", 3)}, 3);
        const auto result = analyze({subroutineWithBody(
                                         "Main", {std::move(value), std::move(call)}),
            subroutine("UseItems", std::move(parameters), std::nullopt, 5)});

        CHECK_FALSE(result.valid);
        CHECK(result.errors.size() == 1);
    }

    SECTION("scalar parameter rejects an array")
    {
        NodeList<Parameter> parameters{
            node<Parameter>("value", nullptr, TypeName::Real, false, 5)};
        auto items = node<Dim>("items", node<Number>(3.0, 2),
            TypeName::Real, true, 2);
        auto call = node<Call>("UseValue",
            NodeList<Expression>{node<Variable>("items", 3)}, 3);
        const auto result = analyze({subroutineWithBody(
                                         "Main", {std::move(items), std::move(call)}),
            subroutine("UseValue", std::move(parameters), std::nullopt, 5)});

        CHECK_FALSE(result.valid);
        CHECK(result.errors.size() == 1);
    }
}

TEST_CASE("Semantic analyzer checks array argument element types", "[semantic]")
{
    NodeList<Parameter> parameters{
        node<Parameter>("items", nullptr, TypeName::Real, true, 5)};
    auto items = node<Dim>("items", node<Number>(3.0, 2),
        TypeName::Text, true, 2);
    auto call = node<Call>("UseItems",
        NodeList<Expression>{node<Variable>("items", 3)}, 3);
    const auto result = analyze({subroutineWithBody(
                                     "Main", {std::move(items), std::move(call)}),
        subroutine("UseItems", std::move(parameters), std::nullopt, 5)});

    CHECK_FALSE(result.valid);
    CHECK(result.errors.size() == 1);
}

TEST_CASE("Semantic analyzer accepts builtin subroutine signatures", "[semantic]")
{
    SECTION("Print accepts every scalar type")
    {
        for( const auto type : {TypeName::Bool, TypeName::Real, TypeName::Text} ) {
            Expression::Ptr argument;
            if( type == TypeName::Bool ) {
                argument = node<Boolean>(true, 2);
            }
            else if( type == TypeName::Real ) {
                argument = node<Number>(1.0, 2);
            }
            else {
                argument = node<Text>("text", 2);
            }
            auto call = node<Call>(
                "Print", NodeList<Expression>{std::move(argument)}, 2);
            const auto result = analyze({subroutineWithBody("Main", {std::move(call)})});
            CHECK(result.valid);
        }
    }

    SECTION("Input and NUM expose their result types")
    {
        auto input = node<Apply>("Input", NodeList<Expression>{}, 2);
        auto number = node<Apply>("NUM",
            NodeList<Expression>{std::move(input)}, 2);
        const auto result = analyzeExpression(std::move(number), TypeName::Real);
        CHECK(result.valid);
    }
}

TEST_CASE("Semantic analyzer rejects calls that violate builtin signatures", "[semantic]")
{
    SECTION("argument count")
    {
        auto call = node<Call>("Print", NodeList<Expression>{}, 2);
        const auto result = analyze({subroutineWithBody("Main", {std::move(call)})});
        CHECK_FALSE(result.valid);
        CHECK(result.errors.size() == 1);
    }

    SECTION("scalar shape")
    {
        auto items = node<Dim>("items", node<Number>(2.0, 2),
            TypeName::Text, true, 2);
        auto call = node<Call>("Print",
            NodeList<Expression>{node<Variable>("items", 3)}, 3);
        const auto result = analyze({subroutineWithBody(
            "Main", {std::move(items), std::move(call)})});
        CHECK_FALSE(result.valid);
        CHECK(result.errors.size() == 1);
    }

    SECTION("argument type")
    {
        auto number = node<Apply>("NUM",
            NodeList<Expression>{node<Number>(1.0, 2)}, 2);
        const auto result = analyzeExpression(std::move(number), TypeName::Real);
        CHECK_FALSE(result.valid);
        CHECK(result.errors.size() == 1);
    }

    SECTION("call kind")
    {
        auto call = node<Call>("Input", NodeList<Expression>{}, 2);
        const auto result = analyze({subroutineWithBody("Main", {std::move(call)})});
        CHECK_FALSE(result.valid);
        CHECK(result.errors.size() == 1);
    }
}

TEST_CASE("Signature checker supports additional builtin subroutines", "[semantic]")
{
    SymbolTable symbols;
    const auto builtin = symbols.declareSubroutine(
        {"IsEmpty", {{TypeName::Text, false}}, TypeName::Bool, true});
    SemanticModel model;
    Diagnostics diagnostics;
    SemanticAnalyzer analyzer{symbols, model, diagnostics};

    auto apply = node<Apply>("IsEmpty",
        NodeList<Expression>{node<Text>("", 2)}, 2);
    const auto applyId = apply->id();
    auto result = node<Dim>("result", nullptr, TypeName::Bool, false, 2);
    auto assignment = node<Let>(node<Variable>("result", 2), nullptr,
        std::move(apply), 2);
    auto main = subroutineWithBody(
        "Main", {std::move(result), std::move(assignment)});
    auto program = node<Program>(NodeList<Subroutine>{std::move(main)}, 1);

    REQUIRE(analyzer.analyze(*program));
    CHECK(model.symbol(applyId) == builtin);
    CHECK(model.type(applyId) == TypeName::Bool);
}
