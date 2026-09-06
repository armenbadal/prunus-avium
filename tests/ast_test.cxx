#include <catch2/catch_test_macros.hpp>

#include "ast.hxx"
#include "astvisitor.hxx"

#include <optional>
#include <string>
#include <type_traits>
#include <vector>

namespace {

using namespace avium;

class NodeKindVisitor final : public ASTVisitor<NodeKindVisitor, NodeKind> {
public:
    using ASTVisitor<NodeKindVisitor, NodeKind>::visit;

    NodeKind visit(Program&) { return NodeKind::Program; }
    NodeKind visit(Subroutine&) { return NodeKind::Subroutine; }
    NodeKind visit(Sequence&) { return NodeKind::Sequence; }
    NodeKind visit(Dim&) { return NodeKind::Dim; }
    NodeKind visit(Let&) { return NodeKind::Let; }
    NodeKind visit(If&) { return NodeKind::If; }
    NodeKind visit(IfBranch&) { return NodeKind::IfBranch; }
    NodeKind visit(While&) { return NodeKind::While; }
    NodeKind visit(For&) { return NodeKind::For; }
    NodeKind visit(Call&) { return NodeKind::Call; }
    NodeKind visit(Apply&) { return NodeKind::Apply; }
    NodeKind visit(Binary&) { return NodeKind::Binary; }
    NodeKind visit(Unary&) { return NodeKind::Unary; }
    NodeKind visit(Variable&) { return NodeKind::Variable; }
    NodeKind visit(Text&) { return NodeKind::Text; }
    NodeKind visit(Number&) { return NodeKind::Number; }
    NodeKind visit(Boolean&) { return NodeKind::Boolean; }
};

Sequence::Ptr emptySequence(Position line = 1)
{
    return node<Sequence>(std::vector<Statement::Ptr>{}, line);
}

} // namespace

TEST_CASE("AST հանգույցի հիմնական տվյալները", "[ast]")
{
    Node empty;
    CHECK(empty.kind == NodeKind::Empty);
    CHECK(empty.line == 0);
    CHECK(empty.id() == 0);

    const auto first = node<Number>(1.0, 4);
    const auto second = node<Number>(2.0, 5);
    const Expression::Ptr expression = first;

    CHECK(first->kind == NodeKind::Number);
    CHECK(first->line == 4);
    CHECK(first->id() < second->id());
    CHECK(expression->kind == NodeKind::Number);
}

TEST_CASE("AST-ի պարզ արտահայտությունները պահում են իրենց արժեքը", "[ast]")
{
    std::string textSource{"կեռաս"};
    std::string nameSource{"fruit_count"};

    const auto boolean = node<Boolean>(true, 3);
    const auto number = node<Number>(3.1415, 4);
    const auto text = node<Text>(textSource, 5);
    const auto variable = node<Variable>(nameSource, 6);
    textSource.clear();
    nameSource.clear();

    CHECK(boolean->_value);
    CHECK(boolean->kind == NodeKind::Boolean);
    CHECK(number->_value == 3.1415);
    CHECK(number->line == 4);
    CHECK(text->_value == "կեռաս");
    CHECK(variable->_name == "fruit_count");
}

TEST_CASE("AST-ը ներկայացնում է կազմական արտահայտությունները", "[ast]")
{
    const auto operand = node<Number>(2.0, 7);
    const auto unary = node<Unary>(Operation::Sub, operand, 7);
    const auto right = node<Number>(3.0, 7);
    const auto binary = node<Binary>(Operation::Pow, unary, right, 7);
    const auto apply = node<Apply>("SQR", std::vector<Expression::Ptr>{binary}, 7);
    const auto quotient = node<Binary>(Operation::Quot, operand, right, 8);
    const auto concatenation = node<Binary>(Operation::Conc, operand, right, 9);
    const auto index = node<Binary>(Operation::Index, operand, right, 10);

    CHECK(unary->_operation == Operation::Sub);
    CHECK(unary->_operand == operand);
    CHECK(binary->_operation == Operation::Pow);
    CHECK(binary->_left == unary);
    CHECK(binary->_right == right);
    CHECK(apply->_callee == "SQR");
    REQUIRE(apply->_arguments.size() == 1);
    CHECK(apply->_arguments.front() == binary);
    CHECK(quotient->_operation == Operation::Quot);
    CHECK(concatenation->_operation == Operation::Conc);
    CHECK(index->_operation == Operation::Index);
}

TEST_CASE("DIM-ն ու Parameter alias-ը ներկայացնում են հայտարարությունները", "[ast]")
{
    static_assert(std::is_same_v<Parameter, Dim>);

    const auto scalar = node<Dim>("total", nullptr, TypeName::Real, false, 11);
    const auto size = node<Number>(12.0, 12);
    const auto array = node<Dim>("items", size, TypeName::Text, true, 12);
    const auto parameter = node<Parameter>("values", nullptr, TypeName::Bool, true, 13);

    CHECK(scalar->_name == "total");
    CHECK_FALSE(scalar->_size);
    CHECK(scalar->_type == TypeName::Real);
    CHECK_FALSE(scalar->_isArray);

    CHECK(array->_name == "items");
    CHECK(array->_size == size);
    CHECK(array->_type == TypeName::Text);
    CHECK(array->_isArray);

    CHECK(parameter->kind == NodeKind::Dim);
    CHECK_FALSE(parameter->_size);
    CHECK(parameter->_type == TypeName::Bool);
    CHECK(parameter->_isArray);
}

TEST_CASE("LET-ը պահում է պարզ և ինդեքսավորված վերագրումները", "[ast]")
{
    const auto total = node<Variable>("total", 14);
    const auto value = node<Number>(42.0, 14);
    const auto simple = node<Let>(total, nullptr, value, 14);

    const auto items = node<Variable>("items", 15);
    const auto index = node<Number>(1.0, 15);
    const auto text = node<Text>("ok", 15);
    const auto indexed = node<Let>(items, index, text, 15);

    CHECK(simple->_variable == total);
    CHECK_FALSE(simple->_index);
    CHECK(simple->_value == value);

    CHECK(indexed->_variable == items);
    CHECK(indexed->_index == index);
    CHECK(indexed->_value == text);
}

TEST_CASE("AST-ը պահում է ղեկավարող կառուցվածքները", "[ast]")
{
    const auto condition = node<Boolean>(true, 20);
    const auto thenBody = emptySequence(21);
    const auto elseBody = emptySequence(22);
    const auto branch = node<IfBranch>(condition, thenBody, 20);
    const auto ifStatement = node<If>(std::vector<IfBranch::Ptr>{branch}, elseBody, 20);
    const auto whileStatement = node<While>(condition, thenBody, 23);

    const auto parameter = node<Variable>("i", 24);
    const auto begin = node<Number>(1.0, 24);
    const auto end = node<Number>(10.0, 24);
    const auto step = node<Number>(2.0, 24);
    const auto forStatement = node<For>(parameter, begin, end, step, thenBody, 24);

    REQUIRE(ifStatement->_branches.size() == 1);
    CHECK(ifStatement->_branches.front() == branch);
    CHECK(ifStatement->_alternative == elseBody);
    CHECK(branch->_condition == condition);
    CHECK(branch->_body == thenBody);
    CHECK(whileStatement->_condition == condition);
    CHECK(whileStatement->_body == thenBody);
    CHECK(forStatement->_parameter == parameter);
    CHECK(forStatement->_begin == begin);
    CHECK(forStatement->_end == end);
    CHECK(forStatement->_step == step);
    CHECK(forStatement->_body == thenBody);
}

TEST_CASE("CALL, ենթածրագիրը և ծրագիրը պահպանում են իրենց կառուցվածքը", "[ast]")
{
    const auto argument = node<Text>("Բարև", 30);
    const auto call = node<Call>("Print", std::vector<Expression::Ptr>{argument}, 30);
    const auto body = node<Sequence>(std::vector<Statement::Ptr>{call}, 30);
    const auto parameter = node<Parameter>("items", nullptr, TypeName::Text, true, 29);
    const auto procedure = node<Subroutine>(
        "Main", std::vector<Parameter::Ptr>{}, std::nullopt, body, 28);
    const auto function = node<Subroutine>(
        "Count", std::vector<Parameter::Ptr>{parameter}, TypeName::Real, emptySequence(31), 29);
    const auto program = node<Program>(std::vector<Subroutine::Ptr>{procedure, function}, 28);

    CHECK(call->_callee == "Print");
    REQUIRE(call->_arguments.size() == 1);
    CHECK(call->_arguments.front() == argument);
    REQUIRE(body->_items.size() == 1);
    CHECK(body->_items.front() == call);

    CHECK_FALSE(procedure->_returnType.has_value());
    REQUIRE(function->_returnType.has_value());
    CHECK(*function->_returnType == TypeName::Real);
    REQUIRE(function->_parameters.size() == 1);
    CHECK(function->_parameters.front() == parameter);
    REQUIRE(program->_subroutines.size() == 2);
    CHECK(program->_subroutines.front() == procedure);
    CHECK(program->_subroutines.back() == function);
}

TEST_CASE("ASTVisitor-ը NodeKind-ով ուղարկում է ճիշտ overload-ին", "[ast][visitor]")
{
    const auto number = node<Number>(1.0, 40);
    const auto variable = node<Variable>("x", 40);
    const auto let = node<Let>(variable, nullptr, number, 40);
    const auto sequence = node<Sequence>(std::vector<Statement::Ptr>{let}, 40);
    const auto subroutine = node<Subroutine>(
        "Main", std::vector<Parameter::Ptr>{}, std::nullopt, sequence, 40);
    const auto program = node<Program>(std::vector<Subroutine::Ptr>{subroutine}, 40);
    Node empty;
    NodeKindVisitor visitor;

    CHECK(visitor.visit(*number) == NodeKind::Number);
    CHECK(visitor.visit(*variable) == NodeKind::Variable);
    CHECK(visitor.visit(*let) == NodeKind::Let);
    CHECK(visitor.visit(*sequence) == NodeKind::Sequence);
    CHECK(visitor.visit(*subroutine) == NodeKind::Subroutine);
    CHECK(visitor.visit(*program) == NodeKind::Program);
    CHECK(visitor.visit(empty) == NodeKind::Empty);
}
