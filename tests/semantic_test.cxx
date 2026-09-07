#include <catch2/catch_test_macros.hpp>

#include "semantic.hxx"

#include <optional>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

using namespace avium;

namespace {

Subroutine::Ptr subroutine(std::string_view name,
    std::vector<Parameter::Ptr> parameters = {},
    std::optional<TypeName> returnType = std::nullopt, Position line = 1)
{
    return node<Subroutine>(name, std::move(parameters), returnType,
        node<Sequence>(std::vector<Statement::Ptr>{}, line), line);
}

Subroutine::Ptr subroutineWithBody(std::string_view name,
    std::vector<Statement::Ptr> statements,
    std::vector<Parameter::Ptr> parameters = {},
    std::optional<TypeName> returnType = std::nullopt, Position line = 1)
{
    return node<Subroutine>(name, std::move(parameters), returnType,
        node<Sequence>(std::move(statements), line), line);
}

struct AnalysisResult {
    bool valid;
    std::vector<Error> errors;
};

AnalysisResult analyze(std::vector<Subroutine::Ptr> subroutines)
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
    return analyze({subroutineWithBody("Main", {declaration, assignment})});
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
    std::vector<Parameter::Ptr> parameters{
        node<Parameter>("value", nullptr, TypeName::Real, false, 1)};
    const auto result = analyze({subroutine("Main", std::move(parameters), TypeName::Real)});

    CHECK_FALSE(result.valid);
    CHECK(result.errors.size() == 2);
}

TEST_CASE("Semantic analyzer declares subroutine signatures", "[semantic]")
{
    auto main = subroutine("Main");
    std::vector<Parameter::Ptr> parameters{
        node<Parameter>("items", nullptr, TypeName::Text, true, 2)};
    auto printItems = subroutine("PrintItems", std::move(parameters), std::nullopt, 2);
    auto program = node<Program>(std::vector<Subroutine::Ptr>{main, printItems}, 1);
    SymbolTable symbols;
    SemanticModel model;
    Diagnostics diagnostics;
    SemanticAnalyzer analyzer{symbols, model, diagnostics};

    REQUIRE(analyzer.analyze(*program));
    const auto id = model.symbol(printItems->id());
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
    const auto result = analyze({main, first, duplicate});

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
            const auto result = analyze({main, replacement});
            CHECK_FALSE(result.valid);
            REQUIRE(result.errors.size() == 1);
            CHECK(std::get<0>(result.errors.front()) == 5);
        }
    }
}

TEST_CASE("Semantic analyzer rejects unknown signature types", "[semantic]")
{
    std::vector<Parameter::Ptr> parameters{
        node<Parameter>("value", nullptr, TypeName::Unknown, false, 3)};
    const auto result = analyze({subroutine("Main"),
        subroutine("Broken", std::move(parameters), TypeName::Unknown, 3)});

    CHECK_FALSE(result.valid);
    CHECK(result.errors.size() == 2);
}

TEST_CASE("Semantic analyzer binds local declarations and uses", "[semantic]")
{
    auto declaration = node<Dim>("value", nullptr, TypeName::Real, false, 2);
    auto target = node<Variable>("value", 3);
    auto source = node<Variable>("value", 3);
    auto assignment = node<Let>(target, nullptr, source, 3);
    auto main = subroutineWithBody("Main", {declaration, assignment});
    auto program = node<Program>(std::vector<Subroutine::Ptr>{main}, 1);
    SymbolTable symbols;
    SemanticModel model;
    Diagnostics diagnostics;
    SemanticAnalyzer analyzer{symbols, model, diagnostics};

    REQUIRE(analyzer.analyze(*program));
    const auto id = model.symbol(declaration->id());
    REQUIRE(id.has_value());
    CHECK(model.symbol(target->id()) == id);
    CHECK(model.symbol(source->id()) == id);
}

TEST_CASE("Semantic analyzer rejects an undefined variable", "[semantic]")
{
    auto assignment = node<Let>(node<Variable>("missing", 2), nullptr,
        node<Number>(1.0, 2), 2);
    const auto result = analyze({subroutineWithBody("Main", {assignment})});

    CHECK_FALSE(result.valid);
    CHECK(result.errors.size() == 1);
}

TEST_CASE("Semantic analyzer rejects duplicate names across nested blocks", "[semantic]")
{
    auto outer = node<Dim>("value", nullptr, TypeName::Real, false, 2);
    auto inner = node<Dim>("value", nullptr, TypeName::Text, false, 4);
    auto branch = node<IfBranch>(node<Boolean>(true, 3),
        node<Sequence>(std::vector<Statement::Ptr>{inner}, 3), 3);
    auto conditional = node<If>(std::vector<IfBranch::Ptr>{branch}, nullptr, 3);
    const auto result = analyze({subroutineWithBody("Main", {outer, conditional})});

    CHECK_FALSE(result.valid);
    REQUIRE(result.errors.size() == 1);
    CHECK(std::get<0>(result.errors.front()) == 4);
}

TEST_CASE("Function return variable cannot duplicate a parameter", "[semantic]")
{
    std::vector<Parameter::Ptr> parameters{
        node<Parameter>("Value", nullptr, TypeName::Real, false, 2)};
    const auto result = analyze({subroutine("Main"),
        subroutine("Value", std::move(parameters), TypeName::Real, 2)});

    CHECK_FALSE(result.valid);
    CHECK(result.errors.size() == 1);
}

TEST_CASE("Implicit FOR variable is visible in the whole subroutine", "[semantic]")
{
    auto declaration = node<Dim>("result", nullptr, TypeName::Real, false, 2);
    auto use = node<Variable>("index", 3);
    auto assignment = node<Let>(node<Variable>("result", 3), nullptr, use, 3);
    auto parameter = node<Variable>("index", 4);
    auto loop = node<For>(parameter, node<Number>(1.0, 4), node<Number>(3.0, 4),
        node<Number>(1.0, 4), node<Sequence>(std::vector<Statement::Ptr>{}, 4), 4);
    auto main = subroutineWithBody("Main", {declaration, assignment, loop});
    auto program = node<Program>(std::vector<Subroutine::Ptr>{main}, 1);
    SymbolTable symbols;
    SemanticModel model;
    Diagnostics diagnostics;
    SemanticAnalyzer analyzer{symbols, model, diagnostics};

    REQUIRE(analyzer.analyze(*program));
    const auto id = model.symbol(parameter->id());
    REQUIRE(id.has_value());
    CHECK(model.symbol(use->id()) == id);
    CHECK(symbols.symbol(*id).storage == VariableStorage::ForVariable);
}

TEST_CASE("FOR reuses only a scalar REAL variable", "[semantic]")
{
    auto declaration = node<Dim>("index", nullptr, TypeName::Text, false, 2);
    auto loop = node<For>(node<Variable>("index", 3), node<Number>(1.0, 3),
        node<Number>(3.0, 3), node<Number>(1.0, 3),
        node<Sequence>(std::vector<Statement::Ptr>{}, 3), 3);
    const auto result = analyze({subroutineWithBody("Main", {declaration, loop})});

    CHECK_FALSE(result.valid);
    CHECK(result.errors.size() == 1);
}

TEST_CASE("Semantic analyzer accepts a compatible scalar assignment", "[semantic]")
{
    auto declaration = node<Dim>("value", nullptr, TypeName::Real, false, 2);
    auto number = node<Number>(42.0, 3);
    auto assignment = node<Let>(node<Variable>("value", 3), nullptr, number, 3);
    auto main = subroutineWithBody("Main", {declaration, assignment});
    auto program = node<Program>(std::vector<Subroutine::Ptr>{main}, 1);
    SymbolTable symbols;
    SemanticModel model;
    Diagnostics diagnostics;
    SemanticAnalyzer analyzer{symbols, model, diagnostics};

    REQUIRE(analyzer.analyze(*program));
    CHECK(model.type(number->id()) == TypeName::Real);
}

TEST_CASE("Semantic analyzer rejects an assignment type mismatch", "[semantic]")
{
    auto declaration = node<Dim>("value", nullptr, TypeName::Real, false, 2);
    auto assignment = node<Let>(node<Variable>("value", 3), nullptr,
        node<Text>("wrong", 3), 3);
    const auto result = analyze({subroutineWithBody("Main", {declaration, assignment})});

    CHECK_FALSE(result.valid);
    REQUIRE(result.errors.size() == 1);
    CHECK(std::get<1>(result.errors.front()) == "'value' փոփոխականին պետք է վերագրվի REAL, բայց ստացվել է TEXT։");
}

TEST_CASE("Semantic analyzer rejects indexing a scalar assignment target", "[semantic]")
{
    auto declaration = node<Dim>("value", nullptr, TypeName::Real, false, 2);
    auto assignment = node<Let>(node<Variable>("value", 3), node<Number>(0.0, 3),
        node<Number>(1.0, 3), 3);
    const auto result = analyze({subroutineWithBody("Main", {declaration, assignment})});

    CHECK_FALSE(result.valid);
    CHECK(result.errors.size() == 1);
}

TEST_CASE("Semantic analyzer rejects assigning a whole array", "[semantic]")
{
    auto array = node<Dim>("items", node<Number>(3.0, 2), TypeName::Real, true, 2);
    auto assignment = node<Let>(node<Variable>("items", 3), nullptr,
        node<Number>(1.0, 3), 3);
    const auto result = analyze({subroutineWithBody("Main", {array, assignment})});

    CHECK_FALSE(result.valid);
    CHECK(result.errors.size() == 1);
}

TEST_CASE("Semantic analyzer rejects an array as a scalar value", "[semantic]")
{
    auto array = node<Dim>("items", node<Number>(3.0, 2), TypeName::Real, true, 2);
    auto scalar = node<Dim>("value", nullptr, TypeName::Real, false, 3);
    auto assignment = node<Let>(node<Variable>("value", 4), nullptr,
        node<Variable>("items", 4), 4);
    const auto result = analyze({subroutineWithBody("Main", {array, scalar, assignment})});

    CHECK_FALSE(result.valid);
    CHECK(result.errors.size() == 1);
}

TEST_CASE("Function return variable uses the declared return type", "[semantic]")
{
    auto validReturn = node<Let>(node<Variable>("Value", 3), nullptr,
        node<Number>(1.0, 3), 3);
    auto invalidReturn = node<Let>(node<Variable>("Broken", 6), nullptr,
        node<Text>("wrong", 6), 6);
    const auto result = analyze({subroutine("Main"),
        subroutineWithBody("Value", {validReturn}, {}, TypeName::Real, 2),
        subroutineWithBody("Broken", {invalidReturn}, {}, TypeName::Real, 5)});

    CHECK_FALSE(result.valid);
    CHECK(result.errors.size() == 1);
}

TEST_CASE("Assignment checks an inferred expression result type", "[semantic]")
{
    auto declaration = node<Dim>("value", nullptr, TypeName::Real, false, 2);
    auto expression = node<Binary>(Operation::Conc, node<Text>("a", 3),
        node<Text>("b", 3), 3);
    auto assignment = node<Let>(node<Variable>("value", 3), nullptr,
        expression, 3);
    const auto result = analyze({subroutineWithBody("Main", {declaration, assignment})});

    CHECK_FALSE(result.valid);
    CHECK(result.errors.size() == 1);
}

TEST_CASE("Semantic analyzer accepts well-typed operators", "[semantic]")
{
    SECTION("boolean operators")
    {
        auto negation = node<Unary>(Operation::Not, node<Boolean>(false, 2), 2);
        auto conjunction = node<Binary>(Operation::And, node<Boolean>(true, 2),
            negation, 2);
        auto expression = node<Binary>(Operation::Or, conjunction,
            node<Boolean>(false, 2), 2);
        CHECK(analyzeExpression(expression, TypeName::Bool).valid);
    }

    SECTION("numeric operators")
    {
        auto product = node<Binary>(Operation::Mul, node<Number>(2.0, 2),
            node<Number>(3.0, 2), 2);
        auto expression = node<Binary>(Operation::Add, node<Number>(1.0, 2),
            product, 2);
        CHECK(analyzeExpression(expression, TypeName::Real).valid);
    }

    SECTION("text comparison")
    {
        auto expression = node<Binary>(Operation::Lt, node<Text>("a", 2),
            node<Text>("b", 2), 2);
        CHECK(analyzeExpression(expression, TypeName::Bool).valid);
    }
}

TEST_CASE("Semantic analyzer rejects invalid unary operands", "[semantic]")
{
    SECTION("NOT requires BOOL")
    {
        auto expression = node<Unary>(Operation::Not, node<Number>(1.0, 2), 2);
        const auto result = analyzeExpression(expression, TypeName::Bool);
        CHECK_FALSE(result.valid);
        REQUIRE(result.errors.size() == 1);
        CHECK(std::get<1>(result.errors.front()) == "'NOT' գործողության օպերանդը պետք է լինի BOOL, բայց ստացվել է REAL։");
    }

    SECTION("unary minus requires REAL")
    {
        auto expression = node<Unary>(Operation::Sub, node<Text>("text", 2), 2);
        const auto result = analyzeExpression(expression, TypeName::Real);
        CHECK_FALSE(result.valid);
        REQUIRE(result.errors.size() == 1);
        CHECK(std::get<1>(result.errors.front()) == "Ունար '-' գործողության օպերանդը պետք է լինի REAL, բայց ստացվել է TEXT։");
    }
}

TEST_CASE("Semantic analyzer suppresses a unary type error for an unknown operand", "[semantic]")
{
    auto operand = node<Variable>("missing", 2);
    auto expression = node<Unary>(Operation::Not, operand, 2);
    const auto result = analyzeExpression(expression, TypeName::Bool);

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
        const auto result = analyzeExpression(expression, TypeName::Real);
        CHECK_FALSE(result.valid);
        REQUIRE(result.errors.size() == 1);
        CHECK(std::get<1>(result.errors.front()) == "'+' գործողության ձախ օպերանդը պետք է լինի REAL, բայց ստացվել է TEXT։");
    }

    SECTION("concatenation requires TEXT")
    {
        auto expression = node<Binary>(Operation::Conc, node<Text>("text", 2),
            node<Number>(1.0, 2), 2);
        const auto result = analyzeExpression(expression, TypeName::Text);
        CHECK_FALSE(result.valid);
        CHECK(result.errors.size() == 1);
    }

    SECTION("logical operators require BOOL")
    {
        auto expression = node<Binary>(Operation::And, node<Boolean>(true, 2),
            node<Number>(1.0, 2), 2);
        const auto result = analyzeExpression(expression, TypeName::Bool);
        CHECK_FALSE(result.valid);
        CHECK(result.errors.size() == 1);
    }

    SECTION("equality requires matching types")
    {
        auto expression = node<Binary>(Operation::Eq, node<Number>(1.0, 2),
            node<Text>("1", 2), 2);
        const auto result = analyzeExpression(expression, TypeName::Bool);
        CHECK_FALSE(result.valid);
        CHECK(result.errors.size() == 1);
    }

    SECTION("comparison accepts only matching REAL or TEXT values")
    {
        auto expression = node<Binary>(Operation::Lt, node<Boolean>(false, 2),
            node<Boolean>(true, 2), 2);
        const auto result = analyzeExpression(expression, TypeName::Bool);
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
        expression, 2);
    const auto analysis = analyze({subroutineWithBody("Main",
        {array, result, assignment})});

    CHECK_FALSE(analysis.valid);
    CHECK(analysis.errors.size() == 1);
}

TEST_CASE("Semantic analyzer accepts well-typed control structures", "[semantic]")
{
    auto ifBranch = node<IfBranch>(node<Boolean>(true, 2),
        node<Sequence>(std::vector<Statement::Ptr>{}, 2), 2);
    auto conditional = node<If>(std::vector<IfBranch::Ptr>{ifBranch}, nullptr, 2);
    auto whileLoop = node<While>(node<Boolean>(true, 3),
        node<Sequence>(std::vector<Statement::Ptr>{}, 3), 3);
    auto forLoop = node<For>(node<Variable>("index", 4), node<Number>(1.0, 4),
        node<Number>(3.0, 4), node<Number>(1.0, 4),
        node<Sequence>(std::vector<Statement::Ptr>{}, 4), 4);
    const auto result = analyze({subroutineWithBody("Main",
        {conditional, whileLoop, forLoop})});

    CHECK(result.valid);
    CHECK(result.errors.empty());
}

TEST_CASE("Semantic analyzer requires BOOL branch conditions", "[semantic]")
{
    auto first = node<IfBranch>(node<Number>(1.0, 2),
        node<Sequence>(std::vector<Statement::Ptr>{}, 2), 2);
    auto second = node<IfBranch>(node<Text>("yes", 3),
        node<Sequence>(std::vector<Statement::Ptr>{}, 3), 3);
    auto conditional = node<If>(std::vector<IfBranch::Ptr>{first, second},
        nullptr, 2);
    const auto result = analyze({subroutineWithBody("Main", {conditional})});

    CHECK_FALSE(result.valid);
    REQUIRE(result.errors.size() == 2);
    CHECK(std::get<1>(result.errors[0]) == "Պայմանական ճյուղի պայմանը պետք է լինի BOOL, բայց ստացվել է REAL։");
    CHECK(std::get<1>(result.errors[1]) == "Պայմանական ճյուղի պայմանը պետք է լինի BOOL, բայց ստացվել է TEXT։");
}

TEST_CASE("Semantic analyzer requires a BOOL WHILE condition", "[semantic]")
{
    auto loop = node<While>(node<Text>("yes", 2),
        node<Sequence>(std::vector<Statement::Ptr>{}, 2), 2);
    const auto result = analyze({subroutineWithBody("Main", {loop})});

    CHECK_FALSE(result.valid);
    REQUIRE(result.errors.size() == 1);
    CHECK(std::get<1>(result.errors.front()) == "WHILE-ի պայմանը պետք է լինի BOOL, բայց ստացվել է TEXT։");
}

TEST_CASE("Semantic analyzer checks FOR bounds and step", "[semantic]")
{
    auto loop = node<For>(node<Variable>("index", 2), node<Text>("first", 2),
        node<Boolean>(true, 2), node<Number>(0.0, 2),
        node<Sequence>(std::vector<Statement::Ptr>{}, 2), 2);
    const auto result = analyze({subroutineWithBody("Main", {loop})});

    CHECK_FALSE(result.valid);
    CHECK(result.errors.size() == 3);
}

TEST_CASE("Semantic analyzer rejects an array as a condition", "[semantic]")
{
    auto array = node<Dim>("flags", node<Number>(2.0, 1), TypeName::Bool, true, 1);
    auto branch = node<IfBranch>(node<Variable>("flags", 2),
        node<Sequence>(std::vector<Statement::Ptr>{}, 2), 2);
    auto conditional = node<If>(std::vector<IfBranch::Ptr>{branch}, nullptr, 2);
    const auto result = analyze({subroutineWithBody("Main", {array, conditional})});

    CHECK_FALSE(result.valid);
    CHECK(result.errors.size() == 1);
}

TEST_CASE("Semantic analyzer accepts valid array sizes", "[semantic]")
{
    SECTION("constant expression")
    {
        auto size = node<Binary>(Operation::Add, node<Number>(2.0, 1),
            node<Number>(3.0, 1), 1);
        auto array = node<Dim>("items", size, TypeName::Real, true, 1);
        const auto result = analyze({subroutineWithBody("Main", {array})});
        CHECK(result.valid);
    }

    SECTION("dynamic expression")
    {
        auto length = node<Dim>("length", nullptr, TypeName::Real, false, 1);
        auto size = node<Variable>("length", 2);
        auto array = node<Dim>("items", size, TypeName::Real, true, 2);
        const auto result = analyze({subroutineWithBody("Main", {length, array})});
        CHECK(result.valid);
    }
}

TEST_CASE("Semantic analyzer requires a scalar REAL array size", "[semantic]")
{
    SECTION("wrong type")
    {
        auto size = node<Text>("large", 1);
        auto array = node<Dim>("items", size, TypeName::Real, true, 1);
        const auto result = analyze({subroutineWithBody("Main", {array})});
        CHECK_FALSE(result.valid);
        REQUIRE(result.errors.size() == 1);
        CHECK(std::get<1>(result.errors.front()) == "Զանգվածի չափը պետք է լինի REAL, բայց ստացվել է TEXT։");
    }

    SECTION("array value")
    {
        auto sizes = node<Dim>("sizes", node<Number>(2.0, 1),
            TypeName::Real, true, 1);
        auto size = node<Variable>("sizes", 2);
        auto items = node<Dim>("items", size, TypeName::Real, true, 2);
        const auto result = analyze({subroutineWithBody("Main", {sizes, items})});
        CHECK_FALSE(result.valid);
        CHECK(result.errors.size() == 1);
    }
}

TEST_CASE("Semantic analyzer validates constant array sizes", "[semantic]")
{
    const std::vector<Expression::Ptr> invalidSizes{
        node<Number>(0.0, 1),
        node<Number>(2.5, 1),
        node<Unary>(Operation::Sub, node<Number>(1.0, 1), 1)};

    for( const auto& size : invalidSizes ) {
        auto array = node<Dim>("items", size, TypeName::Real, true, 1);
        const auto result = analyze({subroutineWithBody("Main", {array})});
        CHECK_FALSE(result.valid);
        CHECK(result.errors.size() == 1);
    }
}

TEST_CASE("Semantic analyzer requires a local array size", "[semantic]")
{
    auto array = node<Dim>("items", nullptr, TypeName::Real, true, 1);
    const auto result = analyze({subroutineWithBody("Main", {array})});

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
    auto assignment = node<Let>(node<Variable>("value", 3), nullptr,
        access, 3);
    auto main = subroutineWithBody("Main", {array, scalar, assignment});
    auto program = node<Program>(std::vector<Subroutine::Ptr>{main}, 1);
    SymbolTable symbols;
    SemanticModel model;
    Diagnostics diagnostics;
    SemanticAnalyzer analyzer{symbols, model, diagnostics};

    REQUIRE(analyzer.analyze(*program));
    CHECK(model.type(access->id()) == TypeName::Text);
}

TEST_CASE("Semantic analyzer rejects indexing a scalar expression", "[semantic]")
{
    auto scalar = node<Dim>("value", nullptr, TypeName::Real, false, 1);
    auto result = node<Dim>("result", nullptr, TypeName::Real, false, 1);
    auto access = node<Binary>(Operation::Index, node<Variable>("value", 2),
        node<Number>(0.0, 2), 2);
    auto assignment = node<Let>(node<Variable>("result", 2), nullptr,
        access, 2);
    const auto analysis = analyze({subroutineWithBody("Main",
        {scalar, result, assignment})});

    CHECK_FALSE(analysis.valid);
    CHECK(analysis.errors.size() == 1);
}

TEST_CASE("Semantic analyzer requires a scalar REAL index", "[semantic]")
{
    auto array = node<Dim>("items", node<Number>(3.0, 1),
        TypeName::Real, true, 1);
    auto index = node<Text>("first", 2);
    auto assignment = node<Let>(node<Variable>("items", 2), index,
        node<Number>(1.0, 2), 2);
    const auto result = analyze({subroutineWithBody("Main", {array, assignment})});

    CHECK_FALSE(result.valid);
    REQUIRE(result.errors.size() == 1);
    CHECK(std::get<1>(result.errors.front()) == "Զանգվածի ինդեքսը պետք է լինի REAL, բայց ստացվել է TEXT։");
}

TEST_CASE("Semantic analyzer binds valid procedure and function calls", "[semantic]")
{
    std::vector<Parameter::Ptr> printParameters{
        node<Parameter>("message", nullptr, TypeName::Text, false, 5)};
    auto printMessage = subroutine("PrintMessage", std::move(printParameters),
        std::nullopt, 5);

    std::vector<Parameter::Ptr> doubleParameters{
        node<Parameter>("value", nullptr, TypeName::Real, false, 8)};
    auto doubleValue = subroutine("Double", std::move(doubleParameters),
        TypeName::Real, 8);

    auto call = node<Call>("PrintMessage",
        std::vector<Expression::Ptr>{node<Text>("hello", 2)}, 2);
    auto result = node<Dim>("result", nullptr, TypeName::Real, false, 3);
    auto apply = node<Apply>("Double",
        std::vector<Expression::Ptr>{node<Number>(2.0, 4)}, 4);
    auto assignment = node<Let>(node<Variable>("result", 4), nullptr,
        apply, 4);
    auto main = subroutineWithBody("Main", {call, result, assignment});
    auto program = node<Program>(
        std::vector<Subroutine::Ptr>{main, printMessage, doubleValue}, 1);
    SymbolTable symbols;
    SemanticModel model;
    Diagnostics diagnostics;
    SemanticAnalyzer analyzer{symbols, model, diagnostics};

    REQUIRE(analyzer.analyze(*program));
    CHECK(model.symbol(call->id()) == model.symbol(printMessage->id()));
    CHECK(model.symbol(apply->id()) == model.symbol(doubleValue->id()));
    CHECK(model.type(apply->id()) == TypeName::Real);
}

TEST_CASE("Semantic analyzer requires a declared call target", "[semantic]")
{
    auto call = node<Call>("Missing", std::vector<Expression::Ptr>{}, 2);
    const auto result = analyze({subroutineWithBody("Main", {call})});

    CHECK_FALSE(result.valid);
    CHECK(result.errors.size() == 1);
}

TEST_CASE("CALL accepts only procedures", "[semantic]")
{
    auto call = node<Call>("Value", std::vector<Expression::Ptr>{}, 2);
    const auto result = analyze({subroutineWithBody("Main", {call}),
        subroutine("Value", {}, TypeName::Real, 4)});

    CHECK_FALSE(result.valid);
    CHECK(result.errors.size() == 1);
}

TEST_CASE("Function application accepts only functions", "[semantic]")
{
    auto declaration = node<Dim>("result", nullptr, TypeName::Real, false, 2);
    auto apply = node<Apply>("Work", std::vector<Expression::Ptr>{}, 3);
    auto assignment = node<Let>(node<Variable>("result", 3), nullptr,
        apply, 3);
    const auto result = analyze({subroutineWithBody("Main", {declaration, assignment}),
        subroutine("Work", {}, std::nullopt, 5)});

    CHECK_FALSE(result.valid);
    CHECK(result.errors.size() == 1);
}

TEST_CASE("Semantic analyzer checks call argument count", "[semantic]")
{
    std::vector<Parameter::Ptr> parameters{
        node<Parameter>("first", nullptr, TypeName::Real, false, 4),
        node<Parameter>("second", nullptr, TypeName::Real, false, 4)};
    auto call = node<Call>("Add",
        std::vector<Expression::Ptr>{node<Number>(1.0, 2)}, 2);
    const auto result = analyze({subroutineWithBody("Main", {call}),
        subroutine("Add", std::move(parameters), std::nullopt, 4)});

    CHECK_FALSE(result.valid);
    CHECK(result.errors.size() == 1);
}

TEST_CASE("Semantic analyzer checks scalar argument types", "[semantic]")
{
    std::vector<Parameter::Ptr> parameters{
        node<Parameter>("value", nullptr, TypeName::Real, false, 4)};
    auto call = node<Call>("UseNumber",
        std::vector<Expression::Ptr>{node<Text>("wrong", 2)}, 2);
    const auto result = analyze({subroutineWithBody("Main", {call}),
        subroutine("UseNumber", std::move(parameters), std::nullopt, 4)});

    CHECK_FALSE(result.valid);
    REQUIRE(result.errors.size() == 1);
    CHECK(std::get<1>(result.errors.front()) == "'UseNumber' ենթածրագրի թիվ 1 արգումենտը պետք է լինի REAL, բայց ստացվել է TEXT։");
}

TEST_CASE("Semantic analyzer checks argument shapes", "[semantic]")
{
    SECTION("array parameter requires an array")
    {
        std::vector<Parameter::Ptr> parameters{
            node<Parameter>("items", nullptr, TypeName::Real, true, 5)};
        auto value = node<Dim>("value", nullptr, TypeName::Real, false, 2);
        auto call = node<Call>("UseItems",
            std::vector<Expression::Ptr>{node<Variable>("value", 3)}, 3);
        const auto result = analyze({subroutineWithBody("Main", {value, call}),
            subroutine("UseItems", std::move(parameters), std::nullopt, 5)});

        CHECK_FALSE(result.valid);
        CHECK(result.errors.size() == 1);
    }

    SECTION("scalar parameter rejects an array")
    {
        std::vector<Parameter::Ptr> parameters{
            node<Parameter>("value", nullptr, TypeName::Real, false, 5)};
        auto items = node<Dim>("items", node<Number>(3.0, 2),
            TypeName::Real, true, 2);
        auto call = node<Call>("UseValue",
            std::vector<Expression::Ptr>{node<Variable>("items", 3)}, 3);
        const auto result = analyze({subroutineWithBody("Main", {items, call}),
            subroutine("UseValue", std::move(parameters), std::nullopt, 5)});

        CHECK_FALSE(result.valid);
        CHECK(result.errors.size() == 1);
    }
}

TEST_CASE("Semantic analyzer checks array argument element types", "[semantic]")
{
    std::vector<Parameter::Ptr> parameters{
        node<Parameter>("items", nullptr, TypeName::Real, true, 5)};
    auto items = node<Dim>("items", node<Number>(3.0, 2),
        TypeName::Text, true, 2);
    auto call = node<Call>("UseItems",
        std::vector<Expression::Ptr>{node<Variable>("items", 3)}, 3);
    const auto result = analyze({subroutineWithBody("Main", {items, call}),
        subroutine("UseItems", std::move(parameters), std::nullopt, 5)});

    CHECK_FALSE(result.valid);
    CHECK(result.errors.size() == 1);
}

TEST_CASE("Semantic analyzer accepts builtin subroutine signatures", "[semantic]")
{
    SECTION("Print accepts every scalar type")
    {
        const std::vector<Expression::Ptr> arguments{
            node<Boolean>(true, 2),
            node<Number>(1.0, 2),
            node<Text>("text", 2),
        };

        for( const auto& argument : arguments ) {
            auto call = node<Call>("Print",
                std::vector<Expression::Ptr>{argument}, 2);
            const auto result = analyze({subroutineWithBody("Main", {call})});
            CHECK(result.valid);
        }
    }

    SECTION("Input and NUM expose their result types")
    {
        auto input = node<Apply>("Input", std::vector<Expression::Ptr>{}, 2);
        auto number = node<Apply>("NUM",
            std::vector<Expression::Ptr>{input}, 2);
        const auto result = analyzeExpression(number, TypeName::Real);
        CHECK(result.valid);
    }
}

TEST_CASE("Semantic analyzer rejects calls that violate builtin signatures", "[semantic]")
{
    SECTION("argument count")
    {
        auto call = node<Call>("Print", std::vector<Expression::Ptr>{}, 2);
        const auto result = analyze({subroutineWithBody("Main", {call})});
        CHECK_FALSE(result.valid);
        CHECK(result.errors.size() == 1);
    }

    SECTION("scalar shape")
    {
        auto items = node<Dim>("items", node<Number>(2.0, 2),
            TypeName::Text, true, 2);
        auto call = node<Call>("Print",
            std::vector<Expression::Ptr>{node<Variable>("items", 3)}, 3);
        const auto result = analyze({subroutineWithBody("Main", {items, call})});
        CHECK_FALSE(result.valid);
        CHECK(result.errors.size() == 1);
    }

    SECTION("argument type")
    {
        auto number = node<Apply>("NUM",
            std::vector<Expression::Ptr>{node<Number>(1.0, 2)}, 2);
        const auto result = analyzeExpression(number, TypeName::Real);
        CHECK_FALSE(result.valid);
        CHECK(result.errors.size() == 1);
    }

    SECTION("call kind")
    {
        auto call = node<Call>("Input", std::vector<Expression::Ptr>{}, 2);
        const auto result = analyze({subroutineWithBody("Main", {call})});
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
        std::vector<Expression::Ptr>{node<Text>("", 2)}, 2);
    auto result = node<Dim>("result", nullptr, TypeName::Bool, false, 2);
    auto assignment = node<Let>(node<Variable>("result", 2), nullptr,
        apply, 2);
    auto main = subroutineWithBody("Main", {result, assignment});
    auto program = node<Program>(std::vector<Subroutine::Ptr>{main}, 1);

    REQUIRE(analyzer.analyze(*program));
    CHECK(model.symbol(apply->id()) == builtin);
    CHECK(model.type(apply->id()) == TypeName::Bool);
}
