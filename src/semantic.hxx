#pragma once

#include "ast.hxx"
#include "diagnostics.hxx"
#include "symbols.hxx"

#include <optional>
#include <string_view>
#include <unordered_map>

namespace avium {

// AST հանգույցների semantic կապերը՝ առանց AST-ը փոփոխելու։
class SemanticModel {
public:
    void bind(NodeId node, SymbolId symbol);
    void setEntryPoint(SymbolId symbol);
    void setType(NodeId node, const Type& type);

    std::optional<SymbolId> symbol(NodeId node) const;
    std::optional<SymbolId> entryPoint() const;
    const Type* type(NodeId node) const;

private:
    std::unordered_map<NodeId, SymbolId> _symbols;
    std::optional<SymbolId> _entryPoint;
    std::unordered_map<NodeId, const Type*> _types;
};

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

class SemanticAnalyzer {
public:
    explicit SemanticAnalyzer(SemanticContext& context);

    bool analyze(const Program& program);

private:
    SemanticContext& _context;
};

} // namespace avium
