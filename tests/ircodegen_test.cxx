#include <catch2/catch_test_macros.hpp>

#include "ircodegen.hxx"
#include "semantic.hxx"
#include "test_ast.hxx"

#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/TargetParser/Host.h>

#include <concepts>

using namespace avium;
using test::NodeList;

static_assert(std::derived_from<IRCodeGen, ASTVisitor<IRCodeGen>>);

TEST_CASE("IR code generator emits an empty Main and a C entry point", "[ircodegen]")
{
    auto main = node<Subroutine>("Main", NodeList<Parameter>{}, nullptr,
        node<Sequence>(NodeList<Statement>{}, 1), 1);
    auto program = node<Program>(NodeList<Subroutine>{std::move(main)}, 1);
    SymbolTable symbols;
    SemanticModel model;
    Diagnostics diagnostics;
    SemanticAnalyzer analyzer{symbols, model, diagnostics};
    REQUIRE(analyzer.analyze(*program));

    llvm::LLVMContext context;
    auto module = IRCodeGen{context, *program, symbols, model}.generate();

    REQUIRE(module != nullptr);
    CHECK(module->getTargetTriple().str() == llvm::sys::getDefaultTargetTriple());
    CHECK_FALSE(llvm::verifyModule(*module));

    auto* cMain = module->getFunction("main");
    REQUIRE(cMain != nullptr);
    CHECK(cMain->getReturnType()->isIntegerTy(32));
    CHECK(cMain->arg_empty());
    REQUIRE(cMain->size() == 1);

    auto& entry = cMain->getEntryBlock();
    REQUIRE(entry.size() == 2);
    const auto* call = llvm::dyn_cast<llvm::CallInst>(&entry.front());
    REQUIRE(call != nullptr);
    REQUIRE(call->getCalledFunction() != nullptr);
    CHECK(call->getCalledFunction()->getReturnType()->isVoidTy());
    CHECK(call->getCalledFunction()->hasInternalLinkage());
    CHECK(call->arg_empty());

    const auto* result = llvm::dyn_cast<llvm::ReturnInst>(&entry.back());
    REQUIRE(result != nullptr);
    const auto* value = llvm::dyn_cast<llvm::ConstantInt>(result->getReturnValue());
    REQUIRE(value != nullptr);
    CHECK(value->isZero());

    const auto entryPoint = model.entryPoint();
    REQUIRE(entryPoint.has_value());
    const auto* entryFunction = call->getCalledFunction();
    CHECK(entryFunction->getName() == "avium.subroutine." + std::to_string(*entryPoint));
    REQUIRE(entryFunction->size() == 1);
    CHECK(llvm::isa<llvm::ReturnInst>(entryFunction->getEntryBlock().getTerminator()));
}
