#include "symbols.hxx"

#include <stdexcept>
#include <utility>

namespace avium {

Symbol::Symbol(std::string name)
    : name{std::move(name)}
{
}

VariableSymbol::VariableSymbol(std::string name, const Type& type, VariableStorage storage)
    : Symbol{std::move(name)}
    , type{&type}
    , storage{storage}
{
}

SubroutineSymbol::SubroutineSymbol(std::string name, SubroutineSignature signature, bool builtin)
    : Symbol{std::move(name)}
    , signature{std::move(signature)}
    , builtin{builtin}
{
}

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

SymbolId SymbolTable::insert(SymbolValue symbol, Scope& scope)
{
    auto& common = std::visit([](auto& value) -> Symbol& { return value; }, symbol);
    if( scope.contains(common.name) )
        return UnknownSymbol;

    common.id = _nextId++;
    const auto id = common.id;
    scope.emplace(common.name, id);
    _symbols.push_back(std::move(symbol));
    return id;
}

SymbolId SymbolTable::declareVariable(std::string name, const Type& type, VariableStorage storage)
{
    return insert(VariableSymbol{std::move(name), type, storage}, _scopes.back());
}

SymbolId SymbolTable::declareSubroutine(SubroutineSymbol symbol)
{
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
        if( entry != scope->end() && subroutine(entry->second) != nullptr )
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
    return std::visit([](const auto& value) -> const Symbol& { return value; }, symbolValue(id));
}

const VariableSymbol* SymbolTable::variable(SymbolId id) const
{
    return std::get_if<VariableSymbol>(&symbolValue(id));
}

const SubroutineSymbol* SymbolTable::subroutine(SymbolId id) const
{
    return std::get_if<SubroutineSymbol>(&symbolValue(id));
}

const SymbolValue& SymbolTable::symbolValue(SymbolId id) const
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
