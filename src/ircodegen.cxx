#include "ircodegen.hxx"

#include "ast.hxx"
#include "semantic.hxx"
#include "symbols.hxx"

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/TargetParser/Triple.h>

#include <string>

namespace avium {

namespace {

std::string subroutineName(SymbolId symbol)
{
    return "avium.subroutine." + std::to_string(symbol);
}

} // namespace

IRCodeGen::IRCodeGen(llvm::LLVMContext& context, const SymbolTable& symbols, const SemanticModel& model)
    : _context{context}
    , _symbols{symbols}
    , _model{model}
{
}

std::unique_ptr<llvm::Module> IRCodeGen::generate(const Program& program) const
{
    const auto entryPoint = _model.entryPoint();
    if( !entryPoint )
        llvm::report_fatal_error("IR code generation requires a semantic entry point");

    const auto* entrySymbol = _symbols.subroutine(*entryPoint);
    if( entrySymbol == nullptr )
        llvm::report_fatal_error("IR code generation entry point is not a subroutine");

    const Subroutine* entrySubroutine = nullptr;
    for( const auto& subroutine : program._subroutines ) {
        const auto symbol = _model.symbol(subroutine->id());
        if( symbol && *symbol == *entryPoint ) {
            entrySubroutine = subroutine.get();
            break;
        }
    }
    if( entrySubroutine == nullptr )
        llvm::report_fatal_error("IR code generation cannot find the entry-point AST node");
    if( !entrySubroutine->_body->_items.empty() )
        llvm::report_fatal_error("minimal IR code generation only supports an empty Main");

    auto module = std::make_unique<llvm::Module>("prunus", _context);
    module->setTargetTriple(llvm::Triple{llvm::sys::getDefaultTargetTriple()});

    const auto procedureType = llvm::FunctionType::get(llvm::Type::getVoidTy(_context), false);
    auto* entryFunction = llvm::Function::Create(procedureType,
        llvm::Function::InternalLinkage, subroutineName(entrySymbol->id), *module);
    auto* entryBlock = llvm::BasicBlock::Create(_context, "entry", entryFunction);
    llvm::IRBuilder entryBuilder{entryBlock};
    entryBuilder.CreateRetVoid();

    const auto mainType = llvm::FunctionType::get(llvm::Type::getInt32Ty(_context), false);
    auto* main = llvm::Function::Create(mainType, llvm::Function::ExternalLinkage, "main", *module);
    auto* mainEntry = llvm::BasicBlock::Create(_context, "entry", main);
    llvm::IRBuilder mainBuilder{mainEntry};
    mainBuilder.CreateCall(entryFunction);
    mainBuilder.CreateRet(mainBuilder.getInt32(0));

    if( llvm::verifyModule(*module, &llvm::errs()) )
        llvm::report_fatal_error("IR code generation produced an invalid module");

    return module;
}

} // namespace avium
