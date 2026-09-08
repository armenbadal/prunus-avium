#include "symbols.hxx"

#include <stdexcept>
#include <utility>

namespace avium {

SymbolTable::SymbolTable()
{
    openScope();
}

SymbolTable::~SymbolTable() = default;
SymbolTable::SymbolTable(SymbolTable&&) noexcept = default;
SymbolTable& SymbolTable::operator=(SymbolTable&&) noexcept = default;

void SymbolTable::openScope()
{
    _scopes.emplace_back();
}

bool SymbolTable::closeScope()
{
    if( _scopes.size() <= 1 )
        return false;

    _scopes.pop_back();
    return true;
}

SymbolId SymbolTable::insert(Symbol symbol, Scope& scope)
{
    if( scope.contains(symbol.name) )
        return UnknownSymbol;

    symbol.id = _nextId++;
    const auto id = symbol.id;
    scope.emplace(symbol.name, id);
    _symbols.push_back(std::move(symbol));
    return id;
}

SymbolId SymbolTable::declareVariable(std::string name, TypeName type, bool isArray,
    VariableStorage storage)
{
    Symbol symbol{
        .kind = SymbolKind::Variable,
        .name = std::move(name),
        .type = type,
        .isArray = isArray,
        .storage = storage,
        .subroutine = std::nullopt};
    return insert(std::move(symbol), _scopes.back());
}

SymbolId SymbolTable::declareSubroutine(SubroutineSignature signature)
{
    Symbol symbol{
        .kind = SymbolKind::Subroutine,
        .name = signature.name,
        .type = signature.returnType,
        .isArray = false,
        .storage = signature.builtin ? VariableStorage::Builtin : VariableStorage::Local,
        .subroutine = std::move(signature)};
    return insert(std::move(symbol), _scopes.front());
}

std::optional<SymbolId> SymbolTable::lookup(std::string_view name) const
{
    for( auto scope = _scopes.rbegin(); scope != _scopes.rend(); ++scope )
        if( const auto entry = scope->find(std::string{name}); entry != scope->end() )
            return entry->second;

    return std::nullopt;
}

std::optional<SymbolId> SymbolTable::lookupSubroutine(std::string_view name) const
{
    for( auto scope = _scopes.rbegin(); scope != _scopes.rend(); ++scope ) {
        const auto entry = scope->find(std::string{name});
        if( entry != scope->end() && symbol(entry->second).kind == SymbolKind::Subroutine )
            return entry->second;
    }
    return std::nullopt;
}

bool SymbolTable::declaredInCurrentScope(std::string_view name) const
{
    return _scopes.back().contains(std::string{name});
}

const Symbol& SymbolTable::symbol(SymbolId id) const
{
    if( id == UnknownSymbol || id >= _nextId )
        throw std::out_of_range{"invalid symbol id"};
    return _symbols.at(id - 1);
}

std::size_t SymbolTable::size() const noexcept
{
    return _symbols.size();
}

} // namespace avium
