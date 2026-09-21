#pragma once

#include <memory>

namespace llvm {

class LLVMContext;
class Module;

} // namespace llvm

namespace avium {

class Program;
class SemanticModel;
class SymbolTable;

class IRCodeGen {
public:
    IRCodeGen(llvm::LLVMContext& context, const SymbolTable& symbols, const SemanticModel& model);

    std::unique_ptr<llvm::Module> generate(const Program& program) const;

private:
    llvm::LLVMContext& _context;
    const SymbolTable& _symbols;
    const SemanticModel& _model;
};

} // namespace avium
