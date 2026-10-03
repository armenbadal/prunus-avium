#pragma once

#include "ast.hxx"

#include <ostream>
#include <string>
#include <vector>

namespace avium {

class AstLisp {
public:
    void emit(Program& node, std::ostream& output);

    std::string format(Node& node);
    std::string format(Program& node);
    std::string format(Subroutine& node);
    std::string format(Sequence& node);
    std::string format(Dim& node);
    std::string format(Let& node);
    std::string format(If& node);
    std::string format(IfBranch& node);
    std::string format(While& node);
    std::string format(For& node);
    std::string format(Call& node);
    std::string format(Return& node);

    std::string format(ScalarType& node);
    std::string format(ArrayType& node);
    std::string format(Apply& node);
    std::string format(Binary& node);
    std::string format(Unary& node);
    std::string format(Variable& node);
    std::string format(Text& node);
    std::string format(Number& node);
    std::string format(Boolean& node);

private:
    template<typename T>
    std::string formatVector(const std::vector<T>& values)
    {
        std::string result;
        for( const auto& value : values ) {
            if( !result.empty() )
                result += ' ';
            result += format(*value);
        }
        return result;
    }

    template<typename T>
    std::string spaced(const std::vector<T>& values)
    {
        auto result = formatVector(values);
        return result.empty() ? result : " " + result;
    }
};

using Lisper = AstLisp;

} // namespace avium
