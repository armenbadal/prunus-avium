#include "semantic.hxx"

#include "semanticpasses.hxx"

namespace avium {

void SemanticModel::bind(NodeId node, SymbolId symbol)
{
    _symbols.insert_or_assign(node, symbol);
}

void SemanticModel::setEntryPoint(SymbolId symbol)
{
    _entryPoint = symbol;
}

void SemanticModel::setType(NodeId node, const Type& type)
{
    _types.insert_or_assign(node, &type);
}

std::optional<SymbolId> SemanticModel::symbol(NodeId node) const
{
    if( const auto entry = _symbols.find(node); entry != _symbols.end() )
        return entry->second;
    return std::nullopt;
}

std::optional<SymbolId> SemanticModel::entryPoint() const
{
    return _entryPoint;
}

const Type* SemanticModel::type(NodeId node) const
{
    if( const auto entry = _types.find(node); entry != _types.end() )
        return entry->second;
    return nullptr;
}

SemanticAnalyzer::SemanticAnalyzer(SemanticContext& context)
    : _context{context}
{
}

bool SemanticAnalyzer::analyze(const Program& program)
{
    analyzeNames(program, _context);
    checkTypes(program, _context);
    return _context.diagnostics.count() == 0;
}

} // namespace avium
