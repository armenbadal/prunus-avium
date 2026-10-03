#include "runtimeabi.hxx"

#include <llvm/IR/Attributes.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

#include <climits>
#include <initializer_list>
#include <string_view>

namespace avium {

namespace {

llvm::Function* declareFunction(llvm::Module& module, std::string_view name,
    llvm::Type* result, std::initializer_list<llvm::Type*> parameters)
{
    auto* type = llvm::FunctionType::get(result, parameters, false);
    return llvm::Function::Create(type, llvm::Function::ExternalLinkage, name, module);
}

} // namespace

RuntimeAbi::RuntimeAbi(llvm::Module& module)
    : textType{llvm::StructType::getTypeByName(module.getContext(), "avium.text")}
    , sizeType{module.getDataLayout().getIntPtrType(module.getContext())}
{
    auto& context = module.getContext();
    auto* const voidType = llvm::Type::getVoidTy(context);
    auto* const pointerType = llvm::PointerType::getUnqual(context);
    auto* const boolType = llvm::Type::getInt1Ty(context);
    auto* const boolStorageType = llvm::Type::getInt8Ty(context);
    auto* const intType = llvm::Type::getIntNTy(context, sizeof(int) * CHAR_BIT);
    auto* const unsignedType = llvm::Type::getIntNTy(context, sizeof(unsigned) * CHAR_BIT);
    auto* const realType = llvm::Type::getDoubleTy(context);

    if( textType == nullptr )
        textType = llvm::StructType::create(context, "avium.text");
    if( textType->isOpaque() )
        textType->setBody({pointerType, sizeType, boolStorageType}, false);

    textCreate = declareFunction(module, "avium_text_create", voidType, {pointerType, pointerType, sizeType, unsignedType});
    textCopy = declareFunction(module, "avium_text_copy", voidType, {pointerType, pointerType, unsignedType});
    textDestroy = declareFunction(module, "avium_text_destroy", voidType, {pointerType});
    textMoveAssign = declareFunction(module, "avium_text_move_assign", voidType, {pointerType, pointerType});
    textConcat = declareFunction(module, "avium_text_concat", voidType, {pointerType, pointerType, pointerType, unsignedType});
    textCompare = declareFunction(module, "avium_text_compare", intType, {pointerType, pointerType});
    str = declareFunction(module, "avium_str", voidType, {pointerType, realType, unsignedType});
    strBool = declareFunction(module, "avium_str_bool", voidType, {pointerType, boolType, unsignedType});
    num = declareFunction(module, "avium_num", realType, {pointerType, unsignedType});
    textLength = declareFunction(module, "avium_text_length", realType, {pointerType});
    arrayCreate = declareFunction(module, "avium_array_create", pointerType, {intType, realType, unsignedType});
    arrayDestroy = declareFunction(module, "avium_array_destroy", voidType, {pointerType});
    arrayLength = declareFunction(module, "avium_array_length", realType, {pointerType, unsignedType});
    textArrayAt = declareFunction(module, "avium_text_array_at", pointerType, {pointerType, realType, unsignedType});
    realArrayAt = declareFunction(module, "avium_real_array_at", pointerType, {pointerType, realType, unsignedType});
    boolArrayAt = declareFunction(module, "avium_bool_array_at", pointerType, {pointerType, realType, unsignedType});
    printBool = declareFunction(module, "avium_print_bool", voidType, {boolType, unsignedType});
    printReal = declareFunction(module, "avium_print_real", voidType, {realType, unsignedType});
    printText = declareFunction(module, "avium_print_text", voidType, {pointerType, unsignedType});
    input = declareFunction(module, "avium_input", voidType, {pointerType, unsignedType});
    sqr = declareFunction(module, "avium_sqr", realType, {realType, unsignedType});

    strBool->addParamAttr(1, llvm::Attribute::ZExt);
    printBool->addParamAttr(0, llvm::Attribute::ZExt);
}

} // namespace avium
