#include <llvm/ADT/Twine.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/GlobalValue.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/TargetParser/Triple.h>
#include <llvm/Support/raw_ostream.h>

void generate_ex00()
{
    llvm::LLVMContext cx;
    llvm::IRBuilder<> bl{cx};

    auto m = std::make_unique<llvm::Module>("ex00", cx);
    m->setTargetTriple(llvm::sys::getDefaultTargetTriple());

    auto* cerasusMainFuncType = llvm::FunctionType::get(llvm::Type::getVoidTy(cx), false);
    auto* cerasusMainFunc = llvm::Function::Create(cerasusMainFuncType, llvm::Function::InternalLinkage, "cerasus_Main", *m);
    auto* cerasusMainBegin = llvm::BasicBlock::Create(cx, "", cerasusMainFunc);
    bl.SetInsertPoint(cerasusMainBegin);
    bl.CreateRetVoid();

    auto* siMainFuncType = llvm::FunctionType::get(llvm::Type::getInt32Ty(cx), false);
    auto* siMainFunc = llvm::Function::Create(siMainFuncType, llvm::Function::ExternalLinkage, "main", *m);
    auto* siMainBegin = llvm::BasicBlock::Create(cx, "", siMainFunc);
    bl.SetInsertPoint(siMainBegin);
    bl.CreateCall(cerasusMainFunc);
    bl.CreateRet(bl.getInt32(0));

    m->print(llvm::outs(), nullptr);
}

int main()
{
    generate_ex00();
    return 0;
}
