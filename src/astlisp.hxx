#pragma once

#include "ast.hxx"
#include "astvisitor.hxx"

#include <ostream>
#include <string>
#include <vector>

namespace avium {

class AstLisp : public ASTVisitor<AstLisp, std::string> {
public:
    void emit(Program& node, std::ostream& output);

    using ASTVisitor<AstLisp, std::string>::visit;

    std::string visit(Program& node);
    std::string visit(Subroutine& node);
    std::string visit(Sequence& node);
    std::string visit(Dim& node);
    std::string visit(Let& node);
    std::string visit(If& node);
    std::string visit(IfBranch& node);
    std::string visit(While& node);
    std::string visit(For& node);
    std::string visit(Call& node);
    std::string visit(Return& node);

    std::string visit(ScalarType& node);
    std::string visit(ArrayType& node);
    std::string visit(Apply& node);
    std::string visit(Binary& node);
    std::string visit(Unary& node);
    std::string visit(Variable& node);
    std::string visit(Text& node);
    std::string visit(Number& node);
    std::string visit(Boolean& node);

private:
    template<typename T>
    std::string visitVector(const std::vector<T>& values)
    {
        std::string result;
        for( const auto& value : values ) {
            if( !result.empty() )
                result += ' ';
            result += visit(*value);
        }
        return result;
    }

    template<typename T>
    std::string spaced(const std::vector<T>& values)
    {
        auto result = visitVector(values);
        return result.empty() ? result : " " + result;
    }
};

using Lisper = AstLisp;

} // namespace avium
