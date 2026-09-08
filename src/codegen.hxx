#pragma once

#include "ast.hxx"
#include "semantic.hxx"
#include "symbols.hxx"

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>

#include <memory>
#include <string_view>
#include <unordered_map>

namespace llvm {

class LLVMContext;
class Function;
class StructType;
class Type;
class Value;

} // namespace llvm

namespace avium {

class CodeGenerator {
public:
    CodeGenerator(llvm::LLVMContext& context, const SymbolTable& symbols, const SemanticModel& model);

    std::unique_ptr<llvm::Module> generate(const Program& program, std::string_view moduleName);

private:
    llvm::Type* llvmType(TypeName type) const;
    llvm::StructType* arrayType() const;
    SymbolId symbolId(const Node& node) const;

    void declareSubroutines(const Program& program);
    void defineSubroutine(const Subroutine& subroutine);
    void allocateParameters(const Subroutine& subroutine, llvm::Function& function);
    void allocateReturnValue(const Subroutine& subroutine);
    void allocateLocals(const Sequence& sequence);
    void allocateVariable(SymbolId id);
    void createEntryPoint(const Program& program);

    llvm::LLVMContext& _context;
    const SymbolTable& _symbols;
    const SemanticModel& _model;
    llvm::IRBuilder<> _builder;
    std::unique_ptr<llvm::Module> _module;
    std::unordered_map<SymbolId, llvm::Function*> _functions;
    std::unordered_map<SymbolId, llvm::Value*> _storage;
};

} // namespace avium
