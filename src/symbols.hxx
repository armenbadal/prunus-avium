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

// Անունը ծրագում կարող է ունենալ երկու դեր, փոփոխական, որը 
// կարող է նաև զանգված նշանակել, և ենթածրագիր
enum class SymbolKind {
    Variable,   // փոփոխական
    Subroutine, // ենթածրագիր
};

// Ենթածրագրի մարմնում փոփոխականի պահպանման դերը
enum class VariableStorage {
    Local,        // DIM-ով բացահայտ հայտարարված
    Parameter,    // պարամետր
    ForVariable,  // FOR-ի պարամետր, անբացահայտ REAL
    Builtin,      // ներդրված ենթածրագիր
};

// Ենթածրագրի նկարագրությունը
struct SubroutineSignature {
    std::string name;
    std::vector<const Type*> parameters;
    const ScalarType* returnType{};
    bool builtin{false};
};

// Ծրագրում հանդիպող անունի նկարագրիչը որպես ինքնուրույն սիմվոլ
struct Symbol {
    SymbolId id{UnknownSymbol};  // եզակի իդենտիֆիկատոր
    SymbolKind kind{SymbolKind::Variable};  // դերը
    std::string name;
    const Type* type{};
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

    // սկսել նոր անունների տիրույթ
    void openScope();
    // վերադառնալ նախորդ անունների տիրույթին
    bool closeScope();

    SymbolId declareVariable(std::string name, const Type& type, VariableStorage storage = VariableStorage::Local);
    SymbolId declareSubroutine(SubroutineSignature signature);

    std::optional<SymbolId> lookup(std::string_view name) const;
    std::optional<SymbolId> lookupSubroutine(std::string_view name) const;
    bool declaredInCurrentScope(std::string_view name) const;

    const Symbol& symbol(SymbolId id) const;
    std::size_t size() const noexcept;

private:
    // անունների տիրույթը որպես արտապատկերում
    using Scope = std::unordered_map<std::string, SymbolId>;

    SymbolId insert(Symbol symbol, Scope& scope);

    std::vector<Symbol> _symbols;
    std::vector<Scope> _scopes;
    SymbolId _nextId{1};
};

} // namespace avium
