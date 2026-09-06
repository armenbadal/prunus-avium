#pragma once

#include "ast.hxx"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace avium {

using SymbolId = std::uint32_t;
inline constexpr SymbolId UnknownSymbol = 0;

enum class SymbolKind : std::uint8_t {
    Variable,
    Subroutine,
};

enum class VariableStorage : std::uint8_t {
    Local,
    Parameter,
    ForVariable,
    ReturnValue,
    Builtin,
};

struct ParameterInfo {
    std::optional<TypeName> type;
    bool isArray{false};
};

struct SubroutineSignature {
    std::string name;
    std::vector<ParameterInfo> parameters;
    std::optional<TypeName> returnType;
    bool builtin{false};
};

struct Symbol {
    SymbolId id{UnknownSymbol};
    SymbolKind kind{SymbolKind::Variable};
    std::string name;
    TypeName type{TypeName::Unknown};
    bool isArray{false};
    VariableStorage storage{VariableStorage::Local};
    std::optional<SubroutineSignature> subroutine;
};

class SymbolTable {
public:
    SymbolTable();
    ~SymbolTable();

    SymbolTable(SymbolTable&&) noexcept;
    SymbolTable& operator=(SymbolTable&&) noexcept;

    SymbolTable(const SymbolTable&) = delete;
    SymbolTable& operator=(const SymbolTable&) = delete;

    void openScope();
    bool closeScope();

    SymbolId declareVariable(std::string name, TypeName type, bool isArray = false,
        VariableStorage storage = VariableStorage::Local);
    SymbolId declareSubroutine(SubroutineSignature signature);

    std::optional<SymbolId> lookup(std::string_view name) const;
    std::optional<SymbolId> lookupSubroutine(std::string_view name) const;
    bool declaredInCurrentScope(std::string_view name) const;

    const Symbol& symbol(SymbolId id) const;
    std::size_t size() const noexcept;

private:
    using Scope = std::unordered_map<std::string, SymbolId>;

    SymbolId insert(Symbol symbol, Scope& scope);

    std::vector<Symbol> _symbols;
    std::vector<Scope> _scopes;
    SymbolId _nextId{1};
};

} // namespace avium
