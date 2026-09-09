#include <catch2/catch_test_macros.hpp>

#include "codegen.hxx"
#include "test_ast.hxx"

#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_ostream.h>

#include <optional>
#include <string>
#include <vector>

using namespace avium;
using test::NodeList;

TEST_CASE("Code generator creates a valid entry point for an empty Main",
    "[codegen]")
{
    auto body = node<Sequence>(NodeList<Statement>{}, 1);
    auto main = node<Subroutine>("Main", NodeList<Parameter>{},
        std::nullopt, std::move(body), 1);
    auto program = node<Program>(NodeList<Subroutine>{std::move(main)}, 1);

    SymbolTable symbols;
    SemanticModel model;
    Diagnostics diagnostics;
    SemanticAnalyzer analyzer{symbols, model, diagnostics};
    REQUIRE(analyzer.analyze(*program));

    llvm::LLVMContext context;
    CodeGenerator generator{context, symbols, model};
    const auto module = generator.generate(*program, "empty-main");

    REQUIRE(module != nullptr);
    CHECK(module->getModuleIdentifier() == "empty-main");

    const auto* entryPoint = module->getFunction("main");
    REQUIRE(entryPoint != nullptr);
    CHECK(entryPoint->arg_empty());
    CHECK(entryPoint->getReturnType()->isIntegerTy(32));
    CHECK_FALSE(entryPoint->isDeclaration());
    REQUIRE(entryPoint->size() == 1);

    const auto* sourceMain = module->getFunction("avium.sub.Main");
    REQUIRE(sourceMain != nullptr);
    REQUIRE(sourceMain->size() == 1);
    CHECK(llvm::isa<llvm::ReturnInst>(sourceMain->back().getTerminator()));

    const auto* call = llvm::dyn_cast<llvm::CallInst>(&entryPoint->front().front());
    REQUIRE(call != nullptr);
    CHECK(call->getCalledFunction() == sourceMain);

    const auto* returnValue = llvm::dyn_cast<llvm::ConstantInt>(
        entryPoint->back().getTerminator()->getOperand(0));
    REQUIRE(returnValue != nullptr);
    CHECK(returnValue->isZero());

    std::string verificationMessage;
    llvm::raw_string_ostream output{verificationMessage};
    INFO(verificationMessage);
    CHECK_FALSE(llvm::verifyModule(*module, &output));
}

TEST_CASE("Code generator lowers subroutine signatures and parameter storage",
    "[codegen]")
{
    auto mainBody = node<Sequence>(NodeList<Statement>{}, 1);
    auto main = node<Subroutine>("Main", NodeList<Parameter>{},
        std::nullopt, std::move(mainBody), 1);

    NodeList<Parameter> parameters{
        node<Parameter>("flag", nullptr, TypeName::Bool, false, 2),
        node<Parameter>("number", nullptr, TypeName::Real, false, 2),
        node<Parameter>("label", nullptr, TypeName::Text, false, 2),
        node<Parameter>("items", nullptr, TypeName::Real, true, 2),
    };
    auto body = node<Sequence>(NodeList<Statement>{}, 2);
    auto transform = node<Subroutine>("Transform", std::move(parameters),
        TypeName::Text, std::move(body), 2);
    const auto transformId = transform->id();
    auto program = node<Program>(NodeList<Subroutine>{
                                     std::move(main), std::move(transform)},
        1);

    SymbolTable symbols;
    SemanticModel model;
    Diagnostics diagnostics;
    SemanticAnalyzer analyzer{symbols, model, diagnostics};
    REQUIRE(analyzer.analyze(*program));
    REQUIRE(model.returnValue(transformId).has_value());

    llvm::LLVMContext context;
    CodeGenerator generator{context, symbols, model};
    const auto module = generator.generate(*program, "signatures");

    const auto* function = module->getFunction("avium.sub.Transform");
    REQUIRE(function != nullptr);
    CHECK(function->getReturnType()->isPointerTy());
    REQUIRE(function->arg_size() == 4);

    auto argument = function->arg_begin();
    CHECK(argument->getType()->isIntegerTy(1));
    CHECK((++argument)->getType()->isDoubleTy());
    CHECK((++argument)->getType()->isPointerTy());
    CHECK((++argument)->getType()->isPointerTy());

    std::size_t allocations = 0;
    std::size_t stores = 0;
    for( const auto& instruction : function->getEntryBlock() ) {
        allocations += llvm::isa<llvm::AllocaInst>(instruction);
        stores += llvm::isa<llvm::StoreInst>(instruction);
    }
    CHECK(allocations == 4);
    CHECK(stores == 4);
    CHECK_FALSE(llvm::verifyModule(*module));
}

TEST_CASE("Code generator allocates every function-scoped local in the entry block",
    "[codegen]")
{
    auto flag = node<Dim>("flag", nullptr, TypeName::Bool, false, 2);
    auto number = node<Dim>("number", nullptr, TypeName::Real, false, 3);
    auto label = node<Dim>("label", nullptr, TypeName::Text, false, 4);
    auto items = node<Dim>("items", node<Number>(2.0, 5), TypeName::Real, true, 5);

    auto nested = node<Dim>("nested", nullptr, TypeName::Real, false, 7);
    auto branchBody = node<Sequence>(NodeList<Statement>{std::move(nested)}, 7);
    auto branch = node<IfBranch>(
        node<Boolean>(true, 6), std::move(branchBody), 6);
    auto conditional = node<If>(NodeList<IfBranch>{std::move(branch)}, nullptr, 6);

    auto loopBody = node<Sequence>(NodeList<Statement>{}, 8);
    auto loop = node<For>(node<Variable>("index", 8), node<Number>(0.0, 8),
        node<Number>(1.0, 8), node<Number>(1.0, 8),
        std::move(loopBody), 8);
    auto body = node<Sequence>(NodeList<Statement>{std::move(flag),
                                   std::move(number), std::move(label), std::move(items),
                                   std::move(conditional), std::move(loop)},
        1);
    auto main = node<Subroutine>("Main", NodeList<Parameter>{},
        std::nullopt, std::move(body), 1);
    auto program = node<Program>(NodeList<Subroutine>{std::move(main)}, 1);

    SymbolTable symbols;
    SemanticModel model;
    Diagnostics diagnostics;
    SemanticAnalyzer analyzer{symbols, model, diagnostics};
    REQUIRE(analyzer.analyze(*program));

    llvm::LLVMContext context;
    CodeGenerator generator{context, symbols, model};
    const auto module = generator.generate(*program, "locals");
    const auto* function = module->getFunction("avium.sub.Main");
    REQUIRE(function != nullptr);

    std::size_t allocations = 0;
    std::size_t stores = 0;
    const llvm::AllocaInst* arrayStorage = nullptr;
    for( const auto& instruction : function->getEntryBlock() ) {
        if( const auto* allocation = llvm::dyn_cast<llvm::AllocaInst>(&instruction) ) {
            ++allocations;
            if( allocation->getName() == "items" )
                arrayStorage = allocation;
        }
        stores += llvm::isa<llvm::StoreInst>(instruction);
    }

    CHECK(allocations == 6);
    CHECK(stores == 6);
    REQUIRE(arrayStorage != nullptr);
    const auto* descriptor = llvm::dyn_cast<llvm::StructType>(
        arrayStorage->getAllocatedType());
    REQUIRE(descriptor != nullptr);
    REQUIRE(descriptor->getNumElements() == 2);
    CHECK(descriptor->getElementType(0)->isPointerTy());
    CHECK(descriptor->getElementType(1)->isIntegerTy(64));
    CHECK_FALSE(llvm::verifyModule(*module));
}
