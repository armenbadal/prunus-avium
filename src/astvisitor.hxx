#pragma once

#include "ast.hxx"

namespace avium {

template<typename Derived, typename ReturnType = void>
class ASTVisitor {
protected:
    ReturnType visit(Node& value)
    {
        switch( value.kind ) {
            case NodeKind::Program:
                return derived().visit(static_cast<Program&>(value));
            case NodeKind::Subroutine:
                return derived().visit(static_cast<Subroutine&>(value));
            case NodeKind::Sequence:
                return derived().visit(static_cast<Sequence&>(value));
            case NodeKind::Dim:
                return derived().visit(static_cast<Dim&>(value));
            case NodeKind::Let:
                return derived().visit(static_cast<Let&>(value));
            case NodeKind::If:
                return derived().visit(static_cast<If&>(value));
            case NodeKind::IfBranch:
                return derived().visit(static_cast<IfBranch&>(value));
            case NodeKind::While:
                return derived().visit(static_cast<While&>(value));
            case NodeKind::For:
                return derived().visit(static_cast<For&>(value));
            case NodeKind::Call:
                return derived().visit(static_cast<Call&>(value));
            case NodeKind::Apply:
                return derived().visit(static_cast<Apply&>(value));
            case NodeKind::ScalarType:
                return derived().visit(static_cast<ScalarType&>(value));
            case NodeKind::ArrayType:
                return derived().visit(static_cast<ArrayType&>(value));
            case NodeKind::Binary:
                return derived().visit(static_cast<Binary&>(value));
            case NodeKind::Unary:
                return derived().visit(static_cast<Unary&>(value));
            case NodeKind::Variable:
                return derived().visit(static_cast<Variable&>(value));
            case NodeKind::Text:
                return derived().visit(static_cast<Text&>(value));
            case NodeKind::Number:
                return derived().visit(static_cast<Number&>(value));
            case NodeKind::Boolean:
                return derived().visit(static_cast<Boolean&>(value));
            case NodeKind::Empty:
                return ReturnType{};
        }

        return ReturnType{};
    }

private:
    Derived& derived()
    {
        return static_cast<Derived&>(*this);
    }
};

} // namespace avium
