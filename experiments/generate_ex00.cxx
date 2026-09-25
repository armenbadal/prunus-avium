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
    llvm::LLVMContext context;
    llvm::IRBuilder<> builder{context};

    llvm::Module module{"ex00", context};
    module.setTargetTriple(llvm::sys::getDefaultTargetTriple());

    auto* voidTy = llvm::Type::getVoidTy(context);
    auto* cerasusMainFuncType = llvm::FunctionType::get(voidTy, false);
    auto* cerasusMainFunc = llvm::Function::Create(cerasusMainFuncType, llvm::Function::InternalLinkage, "cerasus_Main", module);
    auto* cerasusMainBegin = llvm::BasicBlock::Create(context, "", cerasusMainFunc);
    builder.SetInsertPoint(cerasusMainBegin);
    builder.CreateRetVoid();

    auto* int32Ty = llvm::Type::getInt32Ty(context);
    auto* siMainFuncType = llvm::FunctionType::get(int32Ty, false);
    auto* siMainFunc = llvm::Function::Create(siMainFuncType, llvm::Function::ExternalLinkage, "main", module);
    auto* siMainBegin = llvm::BasicBlock::Create(context, "", siMainFunc);
    builder.SetInsertPoint(siMainBegin);
    builder.CreateCall(cerasusMainFunc);
    builder.CreateRet(builder.getInt32(0));

    if( llvm::verifyModule(module, &llvm::errs()) )
        llvm::report_fatal_error("IR code generation produced an invalid module");

    module.print(llvm::outs(), nullptr);
}

int main()
{
    generate_ex00();
    return 0;
}

/*
clang++ generate_ex00.cxx $(llvm-config-20 --cxxflags --ldflags --libs --system-libs core support)
*/
