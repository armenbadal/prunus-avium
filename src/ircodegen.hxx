#pragma once

#include "ast.hxx"

#include <llvm/IR/IRBuilder.h>

#include <memory>

namespace llvm {

class LLVMContext;
class Module;
class FunctionType;
class Type;
class Value;

} // namespace llvm

namespace avium {

class SemanticModel;
class SymbolTable;
class SubroutineSignature;

class IRCodeGen {
public:
    IRCodeGen(llvm::LLVMContext& context, Program& program, const SymbolTable& symbols, const SemanticModel& model);

    std::unique_ptr<llvm::Module> generate();

private:
    void declareSubroutines();
    void emitSubroutines();
    void emitMainWrapper();

    void emit(Subroutine& node);
    void emit(Sequence& node);
    void emit(Statement& node);
    void emit(Dim& node);
    void emit(Let& node);
    void emit(If& node);
    void emit(While& node);
    void emit(For& node);
    void emit(Call& node);
    void emit(Return& node);

    llvm::Value* emit(Expression& node);
    llvm::Value* emit(Apply& node);
    llvm::Value* emit(Binary& node);
    llvm::Value* emit(Unary& node);
    llvm::Value* emit(Variable& node);
    llvm::Value* emit(Text& node);
    llvm::Value* emit(Number& node);
    llvm::Value* emit(Boolean& node);

    llvm::FunctionType* createFunctionType(const SubroutineSignature& ss) const;
    llvm::Type* fromCerasusType(const ScalarType& t) const;
    llvm::Type* fromCerasusType(const ArrayType& t) const;

    llvm::LLVMContext& _context;
    Program& _program;
    llvm::IRBuilder<> _builder;
    const SymbolTable& _symbols;
    const SemanticModel& _model;

    std::unique_ptr<llvm::Module> _module;
};

} // namespace avium
