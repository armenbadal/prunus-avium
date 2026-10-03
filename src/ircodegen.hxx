#pragma once

#include "ast.hxx"
#include "diagnostics.hxx"

#include <llvm/IR/IRBuilder.h>

#include <expected>
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

    std::expected<std::unique_ptr<llvm::Module>, Error> generate();

private:
    void declareSubroutines();
    std::expected<void, Error> emitSubroutines();
    std::expected<void, Error> emitMainWrapper();

    std::expected<void, Error> emit(Subroutine& node);
    std::expected<void, Error> emit(Sequence& node);
    std::expected<void, Error> emit(Statement& node);
    std::expected<void, Error> emit(Dim& node);
    std::expected<void, Error> emit(Let& node);
    std::expected<void, Error> emit(If& node);
    std::expected<void, Error> emit(While& node);
    std::expected<void, Error> emit(For& node);
    std::expected<void, Error> emit(Call& node);
    std::expected<void, Error> emit(Return& node);

    std::expected<llvm::Value*, Error> emit(Expression& node);
    std::expected<llvm::Value*, Error> emit(Apply& node);
    std::expected<llvm::Value*, Error> emit(Binary& node);
    std::expected<llvm::Value*, Error> emit(Unary& node);
    std::expected<llvm::Value*, Error> emit(Variable& node);
    std::expected<llvm::Value*, Error> emit(Text& node);
    std::expected<llvm::Value*, Error> emit(Number& node);
    std::expected<llvm::Value*, Error> emit(Boolean& node);

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
