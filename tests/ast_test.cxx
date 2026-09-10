#include <catch2/catch_test_macros.hpp>

#include "ast.hxx"
#include "astvisitor.hxx"
#include "test_ast.hxx"

#include <string>
#include <type_traits>
#include <vector>

namespace {

using namespace avium;
using test::NodeList;

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
    NodeKind visit(ScalarType&) { return NodeKind::ScalarType; }
    NodeKind visit(ArrayType&) { return NodeKind::ArrayType; }
    NodeKind visit(Return&) { return NodeKind::Return; }
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
    return node<Sequence>(NodeList<Statement>{}, line);
}

} // namespace

TEST_CASE("AST հանգույցի հիմնական տվյալները", "[ast]")
{
    Node empty;
    CHECK(empty.kind == NodeKind::Empty);
    CHECK(empty.line == 0);
    CHECK(empty.id() == 0);

    auto first = node<Number>(1.0, 4);
    const auto second = node<Number>(2.0, 5);
    const auto firstId = first->id();
    Expression::Ptr expression = std::move(first);

    CHECK_FALSE(first);
    CHECK(expression->kind == NodeKind::Number);
    CHECK(expression->line == 4);
    CHECK(firstId < second->id());
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

TEST_CASE("AST-ը ներկայացնում է պարզ և զանգվածային տիպերը", "[ast]")
{
    const auto scalar = node<ScalarType>(ScalarType::Name::Text, 6);
    CHECK(scalar->kind == NodeKind::ScalarType);
    CHECK(scalar->_name == ScalarType::Name::Text);

    const auto openArray = node<ArrayType>(
        node<ScalarType>(ScalarType::Name::Real, 7), nullptr, 7);
    CHECK(openArray->kind == NodeKind::ArrayType);
    CHECK(openArray->isOpen());
    CHECK(openArray->_base->_name == ScalarType::Name::Real);

    const auto closedArray = node<ArrayType>(
        node<ScalarType>(ScalarType::Name::Bool, 8), node<Number>(4.0, 8), 8);
    CHECK_FALSE(closedArray->isOpen());
    CHECK(closedArray->_base->_name == ScalarType::Name::Bool);
    CHECK(closedArray->_size->kind == NodeKind::Number);
}

TEST_CASE("AST-ը ներկայացնում է կազմական արտահայտությունները", "[ast]")
{
    auto operand = node<Number>(2.0, 7);
    const auto* operandNode = operand.get();
    auto unary = node<Unary>(Operation::Sub, std::move(operand), 7);
    const auto* unaryNode = unary.get();
    auto right = node<Number>(3.0, 7);
    const auto* rightNode = right.get();
    auto binary = node<Binary>(Operation::Pow, std::move(unary), std::move(right), 7);
    const auto* binaryNode = binary.get();
    const auto apply = node<Apply>("SQR", NodeList<Expression>{std::move(binary)}, 7);
    const auto quotient = node<Binary>(Operation::Quot, node<Number>(4.0, 8),
        node<Number>(2.0, 8), 8);
    const auto concatenation = node<Binary>(Operation::Conc, node<Text>("a", 9),
        node<Text>("b", 9), 9);
    const auto index = node<Binary>(Operation::Index, node<Variable>("items", 10),
        node<Number>(0.0, 10), 10);

    CHECK(unaryNode->_operation == Operation::Sub);
    CHECK(unaryNode->_operand.get() == operandNode);
    CHECK(binaryNode->_operation == Operation::Pow);
    CHECK(binaryNode->_left.get() == unaryNode);
    CHECK(binaryNode->_right.get() == rightNode);
    CHECK(apply->_callee == "SQR");
    REQUIRE(apply->_arguments.size() == 1);
    CHECK(apply->_arguments.front().get() == binaryNode);
    CHECK(quotient->_operation == Operation::Quot);
    CHECK(concatenation->_operation == Operation::Conc);
    CHECK(index->_operation == Operation::Index);
}

TEST_CASE("DIM-ն ու Parameter alias-ը ներկայացնում են հայտարարությունները", "[ast]")
{
    static_assert(std::is_same_v<Parameter, Dim>);

    const auto scalar = node<Dim>(
        "total", node<ScalarType>(ScalarType::Name::Real, 11), 11);
    auto size = node<Number>(12.0, 12);
    const auto* sizeNode = size.get();
    auto arrayType = node<ArrayType>(
        node<ScalarType>(ScalarType::Name::Text, 12), std::move(size), 12);
    const auto* arrayTypeNode = arrayType.get();
    const auto array = node<Dim>("items", std::move(arrayType), 12);
    auto parameterType = node<ArrayType>(
        node<ScalarType>(ScalarType::Name::Bool, 13), nullptr, 13);
    const auto parameter = node<Parameter>("values", std::move(parameterType), 13);

    CHECK(scalar->_name == "total");
    REQUIRE(scalar->_type->kind == NodeKind::ScalarType);
    CHECK(static_cast<const ScalarType&>(*scalar->_type)._name == ScalarType::Name::Real);

    CHECK(array->_name == "items");
    CHECK(array->_type.get() == arrayTypeNode);
    REQUIRE(array->_type->kind == NodeKind::ArrayType);
    const auto& closedArray = static_cast<const ArrayType&>(*array->_type);
    CHECK(closedArray._size.get() == sizeNode);
    CHECK(closedArray._base->_name == ScalarType::Name::Text);

    CHECK(parameter->kind == NodeKind::Dim);
    REQUIRE(parameter->_type->kind == NodeKind::ArrayType);
    const auto& openArray = static_cast<const ArrayType&>(*parameter->_type);
    CHECK(openArray.isOpen());
    CHECK(openArray._base->_name == ScalarType::Name::Bool);
}

TEST_CASE("LET-ը պահում է պարզ և ինդեքսավորված վերագրումները", "[ast]")
{
    auto total = node<Variable>("total", 14);
    const auto* totalNode = total.get();
    auto value = node<Number>(42.0, 14);
    const auto* valueNode = value.get();
    const auto simple = node<Let>(std::move(total), nullptr, std::move(value), 14);

    auto items = node<Variable>("items", 15);
    const auto* itemsNode = items.get();
    auto index = node<Number>(1.0, 15);
    const auto* indexNode = index.get();
    auto text = node<Text>("ok", 15);
    const auto* textNode = text.get();
    const auto indexed = node<Let>(
        std::move(items), std::move(index), std::move(text), 15);

    CHECK(simple->_variable.get() == totalNode);
    CHECK_FALSE(simple->_index);
    CHECK(simple->_value.get() == valueNode);

    CHECK(indexed->_variable.get() == itemsNode);
    CHECK(indexed->_index.get() == indexNode);
    CHECK(indexed->_value.get() == textNode);
}

TEST_CASE("AST-ը պահում է ղեկավարող կառուցվածքները", "[ast]")
{
    auto condition = node<Boolean>(true, 20);
    const auto* conditionNode = condition.get();
    auto thenBody = emptySequence(21);
    const auto* thenBodyNode = thenBody.get();
    auto branch = node<IfBranch>(std::move(condition), std::move(thenBody), 20);
    const auto* branchNode = branch.get();
    auto elseBody = emptySequence(22);
    const auto* elseBodyNode = elseBody.get();
    const auto ifStatement = node<If>(NodeList<IfBranch>{std::move(branch)}, std::move(elseBody), 20);
    const auto whileStatement = node<While>(node<Boolean>(true, 23), emptySequence(23), 23);

    auto parameter = node<Variable>("i", 24);
    const auto* parameterNode = parameter.get();
    auto begin = node<Number>(1.0, 24);
    const auto* beginNode = begin.get();
    auto end = node<Number>(10.0, 24);
    const auto* endNode = end.get();
    auto step = node<Number>(2.0, 24);
    const auto* stepNode = step.get();
    auto forBody = emptySequence(24);
    const auto* forBodyNode = forBody.get();
    const auto forStatement = node<For>(std::move(parameter), std::move(begin),
        std::move(end), std::move(step), std::move(forBody), 24);

    REQUIRE(ifStatement->_branches.size() == 1);
    CHECK(ifStatement->_branches.front().get() == branchNode);
    CHECK(ifStatement->_alternative.get() == elseBodyNode);
    CHECK(branchNode->_condition.get() == conditionNode);
    CHECK(branchNode->_body.get() == thenBodyNode);
    CHECK(whileStatement->_condition);
    CHECK(whileStatement->_body);
    CHECK(forStatement->_parameter.get() == parameterNode);
    CHECK(forStatement->_begin.get() == beginNode);
    CHECK(forStatement->_end.get() == endNode);
    CHECK(forStatement->_step.get() == stepNode);
    CHECK(forStatement->_body.get() == forBodyNode);
}

TEST_CASE("CALL, RETURN, ենթածրագիրը և ծրագիրը պահպանում են իրենց կառուցվածքը", "[ast]")
{
    auto argument = node<Text>("Բարև", 30);
    const auto* argumentNode = argument.get();
    auto call = node<Call>("Print", NodeList<Expression>{std::move(argument)}, 30);
    const auto* callNode = call.get();
    auto returnValue = node<Number>(1.0, 31);
    const auto* returnValueNode = returnValue.get();
    auto returnStatement = node<Return>(std::move(returnValue), 31);
    const auto* returnNode = returnStatement.get();
    auto body = node<Sequence>(
        NodeList<Statement>{std::move(call), std::move(returnStatement)}, 30);
    const auto* bodyNode = body.get();
    auto procedure = node<Subroutine>(
        "Main", NodeList<Parameter>{}, nullptr, std::move(body), 28);
    const auto* procedureNode = procedure.get();
    auto parameter = test::arrayDeclaration<Parameter>(
        "items", nullptr, ScalarType::Name::Text, 29);
    const auto* parameterNode = parameter.get();
    auto returnType = node<ScalarType>(ScalarType::Name::Real, 29);
    auto function = node<Subroutine>("Count", NodeList<Parameter>{std::move(parameter)},
        std::move(returnType), emptySequence(31), 29);
    const auto* functionNode = function.get();
    const auto program = node<Program>(
        NodeList<Subroutine>{std::move(procedure), std::move(function)}, 28);

    CHECK(callNode->_callee == "Print");
    REQUIRE(callNode->_arguments.size() == 1);
    CHECK(callNode->_arguments.front().get() == argumentNode);
    REQUIRE(bodyNode->_items.size() == 2);
    CHECK(bodyNode->_items.front().get() == callNode);
    CHECK(bodyNode->_items.back().get() == returnNode);
    CHECK(returnNode->_value.get() == returnValueNode);

    CHECK_FALSE(procedureNode->_returnType);
    REQUIRE(functionNode->_returnType);
    CHECK(functionNode->_returnType->_name == ScalarType::Name::Real);
    REQUIRE(functionNode->_parameters.size() == 1);
    CHECK(functionNode->_parameters.front().get() == parameterNode);
    REQUIRE(program->_subroutines.size() == 2);
    CHECK(program->_subroutines.front().get() == procedureNode);
    CHECK(program->_subroutines.back().get() == functionNode);
}

TEST_CASE("ASTVisitor-ը NodeKind-ով ուղարկում է ճիշտ overload-ին", "[ast][visitor]")
{
    auto number = node<Number>(1.0, 40);
    auto* numberNode = number.get();
    auto variable = node<Variable>("x", 40);
    auto* variableNode = variable.get();
    auto let = node<Let>(std::move(variable), nullptr, std::move(number), 40);
    auto* letNode = let.get();
    auto returnStatement = node<Return>(node<Number>(0.0, 40), 40);
    auto* returnNode = returnStatement.get();
    auto sequence = node<Sequence>(
        NodeList<Statement>{std::move(let), std::move(returnStatement)}, 40);
    auto* sequenceNode = sequence.get();
    auto subroutine = node<Subroutine>(
        "Main", NodeList<Parameter>{}, nullptr, std::move(sequence), 40);
    auto* subroutineNode = subroutine.get();
    const auto program = node<Program>(NodeList<Subroutine>{std::move(subroutine)}, 40);
    auto scalar = node<ScalarType>(ScalarType::Name::Real, 40);
    Node& scalarNode = *scalar;
    auto array = node<ArrayType>(
        node<ScalarType>(ScalarType::Name::Text, 40), nullptr, 40);
    Node& arrayNode = *array;
    Node empty;
    NodeKindVisitor visitor;

    CHECK(visitor.visit(*numberNode) == NodeKind::Number);
    CHECK(visitor.visit(*variableNode) == NodeKind::Variable);
    CHECK(visitor.visit(*letNode) == NodeKind::Let);
    CHECK(visitor.visit(*returnNode) == NodeKind::Return);
    CHECK(visitor.visit(*sequenceNode) == NodeKind::Sequence);
    CHECK(visitor.visit(*subroutineNode) == NodeKind::Subroutine);
    CHECK(visitor.visit(*program) == NodeKind::Program);
    CHECK(visitor.visit(scalarNode) == NodeKind::ScalarType);
    CHECK(visitor.visit(arrayNode) == NodeKind::ArrayType);
    CHECK(visitor.visit(empty) == NodeKind::Empty);
}
