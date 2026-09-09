#include "astlisp.hxx"
#include "formatters.hxx"

#include <format>

namespace avium {

void AstLisp::emit(Program& node, std::ostream& output)
{
    output << visit(node) << '\n';
}

std::string AstLisp::visit(Boolean& node)
{
    return std::format("(avium-boolean :value {})", node._value ? "T" : "NIL");
}

std::string AstLisp::visit(Number& node)
{
    return std::format("(avium-number :value {})", node._value);
}

std::string AstLisp::visit(Text& node)
{
    return std::format("(avium-text :value \"{}\")", node._value);
}

std::string AstLisp::visit(Variable& node)
{
    return std::format("(avium-variable :name \"{}\")", node._name);
}

std::string AstLisp::visit(ScalarType& node)
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

std::string AstLisp::visit(ArrayType& node)
{
    const auto size = node._size ? visit(*node._size) : "NIL";
    return std::format("(avium-array-type :base {} :size {})",
        visit(*node._base), size);
}

std::string AstLisp::visit(Unary& node)
{
    return std::format("(avium-unary :operation \"{}\" :operand {})",
        node._operation, visit(*node._operand));
}

std::string AstLisp::visit(Binary& node)
{
    return std::format("(avium-binary :operation \"{}\" :left {} :right {})",
        node._operation, visit(*node._left), visit(*node._right));
}

std::string AstLisp::visit(Apply& node)
{
    return std::format("(avium-apply :callee \"{}\" :arguments{})",
        node._callee, spaced(node._arguments));
}

std::string AstLisp::visit(Let& node)
{
    const auto index = node._index ? std::format(" :index {}", visit(*node._index)) : "";
    return std::format("(avium-let (avium-variable :name \"{}\"){} :value {})",
        node._variable->_name, index, visit(*node._value));
}

std::string AstLisp::visit(Dim& node)
{
    return std::format("(avium-dim :name \"{}\" :type {})",
        node._name, visit(*node._type));
}

std::string AstLisp::visit(If& node)
{
    const auto alternative = node._alternative ? " " + visit(*node._alternative) : "";
    return std::format("(avium-if :branches {} :alternative{})", visitVector(node._branches), alternative);
}

std::string AstLisp::visit(IfBranch& node)
{
    return std::format("(avium-if-branch :condition {} :body {})",
        visit(*node._condition), visit(*node._body));
}

std::string AstLisp::visit(While& node)
{
    return std::format("(avium-while :condition {} :body {})",
        visit(*node._condition), visit(*node._body));
}

std::string AstLisp::visit(For& node)
{
    return std::format("(avium-for :parameter {} :begin {} :end {} :step {} :body {})",
        visit(*node._parameter), visit(*node._begin), visit(*node._end),
        visit(*node._step), visit(*node._body));
}

std::string AstLisp::visit(Call& node)
{
    return std::format("(avium-call :callee \"{}\" :arguments{})",
        node._callee, spaced(node._arguments));
}

std::string AstLisp::visit(Sequence& node)
{
    return std::format("(avium-sequence :items{})", spaced(node._items));
}

std::string AstLisp::visit(Subroutine& node)
{
    const auto returnType = node._returnType ? visit(*node._returnType) : "NIL";
    return std::format("(avium-subroutine :name \"{}\" :parameters '({}) :return-type {} :body {})",
        node._name, visitVector(node._parameters), returnType, visit(*node._body));
}

std::string AstLisp::visit(Program& node)
{
    return std::format("(avium-program :subroutines{})", spaced(node._subroutines));
}

} // namespace avium
