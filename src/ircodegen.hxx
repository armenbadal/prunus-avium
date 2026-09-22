#pragma once

#include "astvisitor.hxx"

#include <llvm/IR/IRBuilder.h>

#include <memory>

namespace llvm {

class LLVMContext;
class Module;
class FunctionType;
class Type;

} // namespace llvm

namespace avium {

class SemanticModel;
class SymbolTable;
class SubroutineSignature;

class IRCodeGen : public ASTVisitor<IRCodeGen> {
public:
    IRCodeGen(llvm::LLVMContext& context, Program& program, const SymbolTable& symbols, const SemanticModel& model);

    std::unique_ptr<llvm::Module> generate();

    using ASTVisitor<IRCodeGen>::visit;

    void visit(Program& node);
    void visit(Subroutine& node);
    void visit(Sequence& node);
    void visit(Dim& node);
    void visit(Let& node);
    void visit(If& node);
    void visit(IfBranch& node);
    void visit(While& node);
    void visit(For& node);
    void visit(Call& node);
    void visit(Return& node);
    void visit(Apply& node);
    void visit(ScalarType& node);
    void visit(ArrayType& node);
    void visit(Binary& node);
    void visit(Unary& node);
    void visit(Variable& node);
    void visit(Text& node);
    void visit(Number& node);
    void visit(Boolean& node);

private:
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
