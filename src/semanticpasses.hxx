#pragma once

#include "semantic.hxx"

#include <string_view>
#include <utility>

namespace avium {

struct SemanticContext {
    SymbolTable& symbols;
    SemanticModel& model;
    Diagnostics& diagnostics;

    void report(const Node& node, std::string_view message)
    {
        diagnostics.advance();
        diagnostics.mark(node.line, message);
    }
};

inline const ScalarType& scalarType(ScalarType::Name name)
{
    static const ScalarType boolType{ScalarType::Name::Bool, 0};
    static const ScalarType realType{ScalarType::Name::Real, 0};
    static const ScalarType textType{ScalarType::Name::Text, 0};
    switch( name ) {
        case ScalarType::Name::Bool:
            return boolType;
        case ScalarType::Name::Real:
            return realType;
        case ScalarType::Name::Text:
            return textType;
    }
    std::unreachable();
}

void analyzeNames(const Program& program, SemanticContext& context);
void checkTypes(const Program& program, SemanticContext& context);

} // namespace avium
