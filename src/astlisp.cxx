#include "astlisp.hxx"
#include "formatters.hxx"

#include <format>

namespace avium {

void AstLisp::emit(Program& node, std::ostream& output)
{
    output << format(node) << '\n';
}

std::string AstLisp::format(Node& node)
{
    switch( node.kind ) {
        case NodeKind::Program:
            return format(static_cast<Program&>(node));
        case NodeKind::Subroutine:
            return format(static_cast<Subroutine&>(node));
        case NodeKind::Sequence:
            return format(static_cast<Sequence&>(node));
        case NodeKind::Dim:
            return format(static_cast<Dim&>(node));
        case NodeKind::Let:
            return format(static_cast<Let&>(node));
        case NodeKind::If:
            return format(static_cast<If&>(node));
        case NodeKind::IfBranch:
            return format(static_cast<IfBranch&>(node));
        case NodeKind::While:
            return format(static_cast<While&>(node));
        case NodeKind::For:
            return format(static_cast<For&>(node));
        case NodeKind::Call:
            return format(static_cast<Call&>(node));
        case NodeKind::Return:
            return format(static_cast<Return&>(node));
        case NodeKind::Apply:
            return format(static_cast<Apply&>(node));
        case NodeKind::ScalarType:
            return format(static_cast<ScalarType&>(node));
        case NodeKind::ArrayType:
            return format(static_cast<ArrayType&>(node));
        case NodeKind::Binary:
            return format(static_cast<Binary&>(node));
        case NodeKind::Unary:
            return format(static_cast<Unary&>(node));
        case NodeKind::Variable:
            return format(static_cast<Variable&>(node));
        case NodeKind::Text:
            return format(static_cast<Text&>(node));
        case NodeKind::Number:
            return format(static_cast<Number&>(node));
        case NodeKind::Boolean:
            return format(static_cast<Boolean&>(node));
        case NodeKind::Empty:
            return {};
    }

    std::unreachable();
}

std::string AstLisp::format(Boolean& node)
{
    return std::format("(avium-boolean :value {})", node._value ? "T" : "NIL");
}

std::string AstLisp::format(Number& node)
{
    return std::format("(avium-number :value {})", node._value);
}

std::string AstLisp::format(Text& node)
{
    return std::format("(avium-text :value \"{}\")", node._value);
}

std::string AstLisp::format(Variable& node)
{
    return std::format("(avium-variable :name \"{}\")", node._name);
}

std::string AstLisp::format(ScalarType& node)
{
    switch( node._name ) {
        case ScalarType::Name::Real:
            return "(avium-scalar-type :name \"REAL\")";
        case ScalarType::Name::Text:
            return "(avium-scalar-type :name \"TEXT\")";
        case ScalarType::Name::Bool:
            return "(avium-scalar-type :name \"BOOL\")";
    }
    std::unreachable();
}

std::string AstLisp::format(ArrayType& node)
{
    const auto size = node._size ? format(*node._size) : "NIL";
    return std::format("(avium-array-type :base {} :size {})",
        format(*node._base), size);
}

std::string AstLisp::format(Unary& node)
{
    return std::format("(avium-unary :operation \"{}\" :operand {})",
        node._operation, format(*node._operand));
}

std::string AstLisp::format(Binary& node)
{
    return std::format("(avium-binary :operation \"{}\" :left {} :right {})",
        node._operation, format(*node._left), format(*node._right));
}

std::string AstLisp::format(Apply& node)
{
    return std::format("(avium-apply :callee \"{}\" :arguments{})",
        node._callee, spaced(node._arguments));
}

std::string AstLisp::format(Let& node)
{
    const auto index = node._index ? std::format(" :index {}", format(*node._index)) : "";
    return std::format("(avium-let (avium-variable :name \"{}\"){} :value {})",
        node._variable->_name, index, format(*node._value));
}

std::string AstLisp::format(Dim& node)
{
    return std::format("(avium-dim :name \"{}\" :type {})",
        node._name, format(*node._type));
}

std::string AstLisp::format(If& node)
{
    const auto alternative = node._alternative ? " " + format(*node._alternative) : "";
    return std::format("(avium-if :branches {} :alternative{})", formatVector(node._branches), alternative);
}

std::string AstLisp::format(IfBranch& node)
{
    return std::format("(avium-if-branch :condition {} :body {})",
        format(*node._condition), format(*node._body));
}

std::string AstLisp::format(While& node)
{
    return std::format("(avium-while :condition {} :body {})",
        format(*node._condition), format(*node._body));
}

std::string AstLisp::format(For& node)
{
    return std::format("(avium-for :parameter {} :begin {} :end {} :step {} :body {})",
        format(*node._parameter), format(*node._begin), format(*node._end),
        format(*node._step), format(*node._body));
}

std::string AstLisp::format(Call& node)
{
    return std::format("(avium-call :callee \"{}\" :arguments{})",
        node._callee, spaced(node._arguments));
}

std::string AstLisp::format(Return& node)
{
    return std::format("(avium-return :value {})", format(*node._value));
}

std::string AstLisp::format(Sequence& node)
{
    return std::format("(avium-sequence :items{})", spaced(node._items));
}

std::string AstLisp::format(Subroutine& node)
{
    const auto returnType = node._returnType ? format(*node._returnType) : "NIL";
    return std::format("(avium-subroutine :name \"{}\" :parameters '({}) :return-type {} :body {})",
        node._name, formatVector(node._parameters), returnType, format(*node._body));
}

std::string AstLisp::format(Program& node)
{
    return std::format("(avium-program :subroutines{})", spaced(node._subroutines));
}

} // namespace avium
