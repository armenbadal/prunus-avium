#include <catch2/catch_test_macros.hpp>

#include "astlisp.hxx"

#include <sstream>

using namespace avium;

namespace {

Program::Ptr program(std::vector<Subroutine::Ptr> subroutines)
{
    return node<Program>(std::move(subroutines), 1);
}

Subroutine::Ptr subroutine(std::string_view name, Sequence::Ptr body)
{
    return node<Subroutine>(name, std::vector<Parameter::Ptr>{}, std::nullopt, std::move(body), 1);
}

std::string emit(Program::Ptr value)
{
    std::ostringstream output;
    AstLisp{}.emit(std::move(value), output);
    return output.str();
}

} // namespace

TEST_CASE("AstLisp emits an empty program", "[astlisp]")
{
    CHECK(emit(program({})) == "(avium-program :subroutines)\n");
}

TEST_CASE("AstLisp emits expressions and statements", "[astlisp]")
{
    auto value = node<Binary>(Operation::Add, node<Number>(1.0, 1), node<Number>(2.0, 1), 1);
    auto assignment = node<Let>(node<Variable>("result", 1), nullptr, value, 1);
    auto body = node<Sequence>(std::vector<Statement::Ptr>{assignment}, 1);

    const auto result = emit(program({subroutine("Main", body)}));
    CHECK(result.find("(avium-binary :operation \"ADD\"") != std::string::npos);
    CHECK(result.find("(avium-let") != std::string::npos);
    CHECK(result.find("(avium-sequence :items") != std::string::npos);
}

TEST_CASE("AstLisp emits calls and indexed assignments", "[astlisp]")
{
    auto call = node<Call>("Print", std::vector<Expression::Ptr>{node<Text>("hello", 1)}, 1);
    auto assignment = node<Let>(node<Variable>("items", 1), node<Number>(0.0, 1),
        node<Text>("value", 1), 1);
    auto body = node<Sequence>(std::vector<Statement::Ptr>{call, assignment}, 1);

    const auto result = emit(program({subroutine("Main", body)}));
    CHECK(result.find("(avium-call :callee \"Print\"") != std::string::npos);
    CHECK(result.find(":index (avium-number :value 0)") != std::string::npos);
}
