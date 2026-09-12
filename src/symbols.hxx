#pragma once

#include "ast.hxx"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

namespace avium {

using SymbolId = std::uint32_t;
inline constexpr SymbolId UnknownSymbol = 0;

// Ենթածրագրի մարմնում փոփոխականի պահպանման դերը
enum class VariableStorage {
    Local,       // DIM-ով բացահայտ հայտարարված
    Parameter,   // պարամետր
    ForVariable, // FOR-ի պարամետր, անբացահայտ REAL
};

// Ենթածրագրի կանչի տիպային նկարագրությունը
struct SubroutineSignature {
    std::vector<const Type*> parameters;
    const ScalarType* returnType{};
};

// Ծրագրում հանդիպող անվան ընդհանուր նկարագրիչը
struct Symbol {
    explicit Symbol(std::string name);

    SymbolId id{UnknownSymbol}; // եզակի իդենտիֆիկատոր
    std::string name;
};

struct VariableSymbol final : Symbol {
    VariableSymbol(std::string name, const Type& type,
        VariableStorage storage = VariableStorage::Local);

    const Type* type;
    VariableStorage storage{VariableStorage::Local};
};

struct SubroutineSymbol final : Symbol {
    SubroutineSymbol(std::string name, SubroutineSignature signature,
        bool builtin = false);

    SubroutineSignature signature;
    bool builtin{false};
};

using SymbolValue = std::variant<VariableSymbol, SubroutineSymbol>;

class SymbolTable {
public:
    SymbolTable();
    ~SymbolTable();

    SymbolTable(SymbolTable&&) noexcept;
    SymbolTable& operator=(SymbolTable&&) noexcept;

    SymbolTable(const SymbolTable&) = delete;
    SymbolTable& operator=(const SymbolTable&) = delete;

    // սկսել նոր անունների տիրույթ
    void openScope();
    // վերադառնալ նախորդ անունների տիրույթին
    bool closeScope();

    SymbolId declareVariable(std::string name, const Type& type, VariableStorage storage = VariableStorage::Local);
    SymbolId declareSubroutine(SubroutineSymbol symbol);

    std::optional<SymbolId> lookup(std::string_view name) const;
    std::optional<SymbolId> lookupSubroutine(std::string_view name) const;
    bool declaredInCurrentScope(std::string_view name) const;

    const Symbol& symbol(SymbolId id) const;
    const VariableSymbol* variable(SymbolId id) const;
    const SubroutineSymbol* subroutine(SymbolId id) const;
    std::size_t size() const noexcept;

private:
    // անունների տիրույթը որպես արտապատկերում
    using Scope = std::unordered_map<std::string, SymbolId>;

    SymbolId insert(SymbolValue symbol, Scope& scope);
    const SymbolValue& symbolValue(SymbolId id) const;

    std::vector<SymbolValue> _symbols;
    std::vector<Scope> _scopes;
    SymbolId _nextId{1};
};

} // namespace avium
