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
#include <llvm/Support/FileSystem.h>

#include <system_error>

/*
clang++ gen_ex00.cxx $(llvm-config-20 --cxxflags --ldflags --libs --system-libs core support)
*/
int main()
{
    llvm::LLVMContext context;
    llvm::IRBuilder<> builder{context};

    llvm::Module module{"ex01g", context};
    module.setTargetTriple(llvm::sys::getDefaultTargetTriple());
    llvm::IntegerType* int8Ty = llvm::Type::getInt8Ty(context);
    llvm::ArrayType* arrayTy = llvm::ArrayType::get(int8Ty, 14);
    llvm::Constant* initializer = llvm::ConstantDataArray::getString(context, "Hello, world!");
    llvm::GlobalVariable* hwStr = new llvm::GlobalVariable(module, arrayTy, true, llvm::GlobalValue::PrivateLinkage, initializer, "hw.str");

    llvm::IntegerType* int32Ty = llvm::Type::getInt32Ty(context);
    llvm::FunctionType* mainType = llvm::FunctionType::get(int32Ty, false);
    llvm::Function* mainFunc = llvm::Function::Create(mainType, llvm::Function::ExternalLinkage, "main", module);
    llvm::BasicBlock* mainEntry = llvm::BasicBlock::Create(context, "", mainFunc);
    builder.SetInsertPoint(mainEntry);

    llvm::PointerType* ptrTy = builder.getPtrTy();
    llvm::FunctionType* putsType = llvm::FunctionType::get(int32Ty, {ptrTy}, false);
    llvm::Function* putsFunc = llvm::Function::Create(putsType, llvm::Function::ExternalLinkage, "puts", module); 

    builder.CreateCall(putsFunc, {hwStr});

    llvm::ConstantInt* zero = builder.getInt32(0);
    builder.CreateRet(zero);

    if( llvm::verifyModule(module, &llvm::errs()) )
        llvm::report_fatal_error("IR code generation produced an invalid module");

    std::error_code ec;
    llvm::raw_fd_ostream fout{"ex01g.ll", ec, llvm::sys::fs::OF_None};
    if( ec )
        llvm::report_fatal_error("Cannot open file for output");
    module.print(fout, nullptr);

    return 0;
}
