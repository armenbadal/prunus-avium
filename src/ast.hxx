#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace avium {

using Position = std::uint32_t;
using NodeId = std::uint64_t;

enum class TypeName : std::uint8_t {
    Unknown,
    Bool,
    Real,
    Text,
};

enum class NodeKind : std::uint8_t {
    Empty,

    Boolean,
    Number,
    Text,
    Variable,
    Unary,
    Binary,
    Apply,

    Sequence,
    Dim,
    Let,
    If,
    IfBranch,
    While,
    For,
    Call,
    Subroutine,
    Program,
};

enum class Operation : std::uint8_t {
    None,
    Add,
    Sub,
    Mul,
    Div,
    Quot,
    Mod,
    Pow,
    Eq,
    Ne,
    Gt,
    Ge,
    Lt,
    Le,
    And,
    Or,
    Not,
    Conc,
    Index,
};

class Node {
public:
    Node() = default;

    Node(NodeKind kind, Position line)
        : kind{kind}, line{line}, _id{_nextId++}
    {
    }

    virtual ~Node() = default;

    NodeId id() const noexcept
    {
        return _id;
    }

    using Ptr = std::unique_ptr<Node>;

    const NodeKind kind{NodeKind::Empty};
    const Position line{0};

private:
    NodeId _id{0};
    inline static NodeId _nextId{1};
};

template<typename T, typename... Args>
std::unique_ptr<T> node(Args&&... args)
{
    return std::make_unique<T>(std::forward<Args>(args)...);
}

class Expression : public Node {
public:
    Expression(NodeKind kind, Position line)
        : Node{kind, line}
    {
    }

    using Ptr = std::unique_ptr<Expression>;
};

class Boolean final : public Expression {
public:
    Boolean(bool value, Position line)
        : Expression{NodeKind::Boolean, line}, _value{value}
    {
    }

    using Ptr = std::unique_ptr<Boolean>;

    const bool _value{false};
};

class Number final : public Expression {
public:
    Number(double value, Position line)
        : Expression{NodeKind::Number, line}, _value{value}
    {
    }

    using Ptr = std::unique_ptr<Number>;

    const double _value{0.0};
};

class Text final : public Expression {
public:
    Text(std::string_view value, Position line)
        : Expression{NodeKind::Text, line}, _value{value}
    {
    }

    using Ptr = std::unique_ptr<Text>;

    const std::string _value;
};

class Variable final : public Expression {
public:
    Variable(std::string_view name, Position line)
        : Expression{NodeKind::Variable, line}, _name{name}
    {
    }

    using Ptr = std::unique_ptr<Variable>;

    const std::string _name;
};

class Unary final : public Expression {
public:
    Unary(Operation operation, Expression::Ptr operand, Position line)
        : Expression{NodeKind::Unary, line}, _operation{operation}, _operand{std::move(operand)}
    {
    }

    using Ptr = std::unique_ptr<Unary>;

    const Operation _operation{Operation::None};
    const Expression::Ptr _operand;
};

class Binary final : public Expression {
public:
    Binary(Operation operation, Expression::Ptr left, Expression::Ptr right, Position line)
        : Expression{NodeKind::Binary, line}, _operation{operation}, _left{std::move(left)}, _right{std::move(right)}
    {
    }

    using Ptr = std::unique_ptr<Binary>;

    const Operation _operation{Operation::None};
    const Expression::Ptr _left;
    const Expression::Ptr _right;
};

class Apply final : public Expression {
public:
    Apply(std::string_view callee, std::vector<Expression::Ptr> arguments, Position line)
        : Expression{NodeKind::Apply, line}, _callee{callee}, _arguments{std::move(arguments)}
    {
    }

    using Ptr = std::unique_ptr<Apply>;

    const std::string _callee;
    const std::vector<Expression::Ptr> _arguments;
};

class Statement : public Node {
public:
    Statement(NodeKind kind, Position line)
        : Node{kind, line}
    {
    }

    using Ptr = std::unique_ptr<Statement>;
};

class Sequence final : public Node {
public:
    Sequence(std::vector<Statement::Ptr> items, Position line)
        : Node{NodeKind::Sequence, line}, _items{std::move(items)}
    {
    }

    using Ptr = std::unique_ptr<Sequence>;

    const std::vector<Statement::Ptr> _items;
};

class Dim final : public Statement {
public:
    Dim(std::string_view name, Expression::Ptr size, TypeName type, bool isArray, Position line)
        : Statement{NodeKind::Dim, line}, _name{name}, _size{std::move(size)}, _type{type}, _isArray{isArray}
    {
    }

    using Ptr = std::unique_ptr<Dim>;

    const std::string _name;
    const Expression::Ptr _size;
    const TypeName _type{TypeName::Unknown};
    const bool _isArray{false};
};

using Parameter = Dim;

class Let final : public Statement {
public:
    Let(Variable::Ptr variable, Expression::Ptr index, Expression::Ptr value, Position line)
        : Statement{NodeKind::Let, line}, _variable{std::move(variable)}, _index{std::move(index)}, _value{std::move(value)}
    {
    }

    using Ptr = std::unique_ptr<Let>;

    // _index-ը դատարկ է պարզ փոփոխականի վերագրման դեպքում։
    const Variable::Ptr _variable;
    const Expression::Ptr _index;
    const Expression::Ptr _value;
};

class IfBranch final : public Node {
public:
    IfBranch(Expression::Ptr condition, Sequence::Ptr body, Position line)
        : Node{NodeKind::IfBranch, line}, _condition{std::move(condition)}, _body{std::move(body)}
    {
    }

    using Ptr = std::unique_ptr<IfBranch>;

    const Expression::Ptr _condition;
    const Sequence::Ptr _body;
};

class If final : public Statement {
public:
    If(std::vector<IfBranch::Ptr> branches, Sequence::Ptr alternative, Position line)
        : Statement{NodeKind::If, line}, _branches{std::move(branches)}, _alternative{std::move(alternative)}
    {
    }

    using Ptr = std::unique_ptr<If>;

    const std::vector<IfBranch::Ptr> _branches;
    const Sequence::Ptr _alternative;
};

class While final : public Statement {
public:
    While(Expression::Ptr condition, Sequence::Ptr body, Position line)
        : Statement{NodeKind::While, line}, _condition{std::move(condition)}, _body{std::move(body)}
    {
    }

    using Ptr = std::unique_ptr<While>;

    const Expression::Ptr _condition;
    const Sequence::Ptr _body;
};

class For final : public Statement {
public:
    For(Variable::Ptr parameter, Expression::Ptr begin, Expression::Ptr end,
        Number::Ptr step, Sequence::Ptr body, Position line)
        : Statement{NodeKind::For, line}, _parameter{std::move(parameter)}, _begin{std::move(begin)}, _end{std::move(end)}, _step{std::move(step)}, _body{std::move(body)}
    {
    }

    using Ptr = std::unique_ptr<For>;

    const Variable::Ptr _parameter;
    const Expression::Ptr _begin;
    const Expression::Ptr _end;
    const Number::Ptr _step;
    const Sequence::Ptr _body;
};

// CALL-ով արված պրոցեդուրային կանչ։ Apply-ից առանձին հանգույց է, քանի որ
// Call-ը արժեք չի պահանջում և սեմանտիկորեն միայն պրոցեդուրա է ընդունում։
class Call final : public Statement {
public:
    Call(std::string_view callee, std::vector<Expression::Ptr> arguments, Position line)
        : Statement{NodeKind::Call, line}, _callee{callee}, _arguments{std::move(arguments)}
    {
    }

    using Ptr = std::unique_ptr<Call>;

    const std::string _callee;
    const std::vector<Expression::Ptr> _arguments;
};

class Subroutine final : public Node {
public:
    Subroutine(std::string_view name, std::vector<Parameter::Ptr> parameters,
        std::optional<TypeName> returnType, Sequence::Ptr body, Position line)
        : Node{NodeKind::Subroutine, line}, _name{name}, _parameters{std::move(parameters)}, _returnType{returnType}, _body{std::move(body)}
    {
    }

    using Ptr = std::unique_ptr<Subroutine>;

    const std::string _name;
    const std::vector<Parameter::Ptr> _parameters;
    const std::optional<TypeName> _returnType;
    const Sequence::Ptr _body;
};

class Program final : public Node {
public:
    Program(std::vector<Subroutine::Ptr> subroutines, Position line)
        : Node{NodeKind::Program, line}, _subroutines{std::move(subroutines)}
    {
    }

    using Ptr = std::unique_ptr<Program>;

    const std::vector<Subroutine::Ptr> _subroutines;
};

} // namespace avium
