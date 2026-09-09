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

// Հանդիպում են երկու անուններ. փոփոխական, որը կարող է նաև 
// զանգված նշանակել, և ենթածրագիր
enum class SymbolKind : std::uint8_t {
    Variable,
    Subroutine,
};

// Փոփոխականի դերերը
enum class VariableStorage : std::uint8_t {
    Local,        // լոկալ, DIM-ով բացահայտ սահմանված
    Parameter,    // ենթածրագիր պարամետր
    ForVariable,  // լոկալ, բայց FOR-ով անբացահայտ սահմանված
    ReturnValue,  // ենթածրագիր-ֆունկցիայի վերադարձվող արժեք
    Builtin,      // լեզվում ներդրված, նախասահմանված անուն
};

// Ենթածրագրի պարամետրի հատկությունները
struct ParameterInfo {
    TypeName type;       // տիպը
    bool isArray{false}; // զանգված է, թե ոչ
};

// Ենթածրագրի նկարագրությունը
struct SubroutineSignature {
    std::string name;                       // անուն
    std::vector<ParameterInfo> parameters;  // պարամետրեր
    std::optional<TypeName> returnType;     // վերադարձվող տիպը
    bool builtin{false};                    // ներդրված է, թե ոչ
};

// Ծրագրում հանդիպող անունի նկարագրիչը որպես ինքնուրույն սիմվոլ
struct Symbol {
    SymbolId id{UnknownSymbol};                      // եզակի իդենտիֆիկատոր
    SymbolKind kind{SymbolKind::Variable};           // փոփոխական, թե՞ ենթածրագիր
    std::string name;                                // անունը
    TypeName type{TypeName::Unknown};                // տիպը
    bool isArray{false};                             // սկալյա՞ր, թե՞ զանգված
    VariableStorage storage{VariableStorage::Local}; // դերը 
    std::optional<SubroutineSignature> subroutine;   // եթե ենթածրագիր է, ապա դրա նկարագրությունը
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

    SymbolId declareVariable(std::string name, TypeName type, bool isArray = false, VariableStorage storage = VariableStorage::Local);
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
