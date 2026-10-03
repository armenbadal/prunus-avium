#include <catch2/catch_test_macros.hpp>

#include "runtimeabi.hxx"

#include <llvm/IR/Attributes.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/TargetParser/Triple.h>

#include <cstddef>
#include <initializer_list>
#include <memory>
#include <optional>
#include <string>

using namespace avium;

extern "C" {

std::size_t avium_test_text_data_offset();
std::size_t avium_test_text_length_offset();
std::size_t avium_test_text_owned_offset();
std::size_t avium_test_text_size();
std::size_t avium_test_text_alignment();
std::size_t avium_test_bool_size();
std::size_t avium_test_int_size();
std::size_t avium_test_unsigned_size();
std::size_t avium_test_size_type_size();
std::size_t avium_test_pointer_size();
}

namespace {

std::unique_ptr<llvm::Module> hostModule(llvm::LLVMContext& context)
{
    REQUIRE_FALSE(llvm::InitializeNativeTarget());
    const llvm::Triple triple{llvm::sys::getDefaultTargetTriple()};
    std::string error;
    const auto* target = llvm::TargetRegistry::lookupTarget(triple, error);
    REQUIRE(target != nullptr);
    llvm::TargetOptions options;
    std::unique_ptr<llvm::TargetMachine> targetMachine{
        target->createTargetMachine(triple, llvm::sys::getHostCPUName(), "", options, std::nullopt)};
    REQUIRE(targetMachine != nullptr);

    auto module = std::make_unique<llvm::Module>("runtime-abi-test", context);
    module->setTargetTriple(triple);
    module->setDataLayout(targetMachine->createDataLayout());
    return module;
}

void checkFunction(llvm::Function* declaration, const char* name,
    llvm::Type* result, std::initializer_list<llvm::Type*> parameters)
{
    REQUIRE(declaration != nullptr);
    CHECK(declaration->getName() == name);
    CHECK(declaration->isDeclaration());
    CHECK(declaration->hasExternalLinkage());
    CHECK(declaration->getReturnType() == result);
    REQUIRE(declaration->arg_size() == parameters.size());
    auto argument = declaration->arg_begin();
    for( auto* parameter : parameters ) {
        CHECK(argument->getType() == parameter);
        ++argument;
    }
}

} // namespace

TEST_CASE("Runtime ABI text layout matches the C ABI", "[runtimeabi]")
{
    llvm::LLVMContext context;
    auto module = hostModule(context);
    RuntimeAbi runtime{*module};

    REQUIRE_FALSE(runtime.textType->isOpaque());
    CHECK_FALSE(runtime.textType->isPacked());
    REQUIRE(runtime.textType->getNumElements() == 3);
    CHECK(runtime.textType->getElementType(0)->isPointerTy());
    REQUIRE(runtime.textType->getElementType(1)->isIntegerTy());
    CHECK(runtime.textType->getElementType(1)->getIntegerBitWidth() == module->getDataLayout().getPointerSizeInBits());
    CHECK(runtime.textType->getElementType(2)->isIntegerTy(8));

    const auto* layout = module->getDataLayout().getStructLayout(runtime.textType);
    CHECK(layout->getElementOffset(0).getFixedValue() == avium_test_text_data_offset());
    CHECK(layout->getElementOffset(1).getFixedValue() == avium_test_text_length_offset());
    CHECK(layout->getElementOffset(2).getFixedValue() == avium_test_text_owned_offset());
    CHECK(layout->getSizeInBytes().getFixedValue() == avium_test_text_size());
    CHECK(layout->getAlignment().value() == avium_test_text_alignment());
    CHECK(runtime.sizeType->getBitWidth() == avium_test_size_type_size() * 8);
    CHECK(module->getDataLayout().getPointerSize() == avium_test_pointer_size());
    CHECK(avium_test_bool_size() == 1);
}

TEST_CASE("Runtime ABI declares every C entry point", "[runtimeabi]")
{
    llvm::LLVMContext context;
    auto module = hostModule(context);
    RuntimeAbi runtime{*module};
    auto* const voidType = llvm::Type::getVoidTy(context);
    auto* const pointerType = llvm::PointerType::getUnqual(context);
    auto* const boolType = llvm::Type::getInt1Ty(context);
    auto* const intType = llvm::Type::getIntNTy(context, avium_test_int_size() * 8);
    auto* const unsignedType = llvm::Type::getIntNTy(context, avium_test_unsigned_size() * 8);
    auto* const realType = llvm::Type::getDoubleTy(context);
    auto* const sizeType = runtime.sizeType;

    checkFunction(runtime.textCreate, "avium_text_create", voidType, {pointerType, pointerType, sizeType, unsignedType});
    checkFunction(runtime.textCopy, "avium_text_copy", voidType, {pointerType, pointerType, unsignedType});
    checkFunction(runtime.textDestroy, "avium_text_destroy", voidType, {pointerType});
    checkFunction(runtime.textMoveAssign, "avium_text_move_assign", voidType, {pointerType, pointerType});
    checkFunction(runtime.textConcat, "avium_text_concat", voidType, {pointerType, pointerType, pointerType, unsignedType});
    checkFunction(runtime.textCompare, "avium_text_compare", intType, {pointerType, pointerType});
    checkFunction(runtime.str, "avium_str", voidType, {pointerType, realType, unsignedType});
    checkFunction(runtime.strBool, "avium_str_bool", voidType, {pointerType, boolType, unsignedType});
    checkFunction(runtime.num, "avium_num", realType, {pointerType, unsignedType});
    checkFunction(runtime.textLength, "avium_text_length", realType, {pointerType});
    checkFunction(runtime.arrayCreate, "avium_array_create", pointerType, {intType, realType, unsignedType});
    checkFunction(runtime.arrayDestroy, "avium_array_destroy", voidType, {pointerType});
    checkFunction(runtime.arrayLength, "avium_array_length", realType, {pointerType, unsignedType});
    checkFunction(runtime.textArrayAt, "avium_text_array_at", pointerType, {pointerType, realType, unsignedType});
    checkFunction(runtime.realArrayAt, "avium_real_array_at", pointerType, {pointerType, realType, unsignedType});
    checkFunction(runtime.boolArrayAt, "avium_bool_array_at", pointerType, {pointerType, realType, unsignedType});
    checkFunction(runtime.printBool, "avium_print_bool", voidType, {boolType, unsignedType});
    checkFunction(runtime.printReal, "avium_print_real", voidType, {realType, unsignedType});
    checkFunction(runtime.printText, "avium_print_text", voidType, {pointerType, unsignedType});
    checkFunction(runtime.input, "avium_input", voidType, {pointerType, unsignedType});
    checkFunction(runtime.sqr, "avium_sqr", realType, {realType, unsignedType});

    CHECK(runtime.strBool->hasParamAttribute(1, llvm::Attribute::ZExt));
    CHECK(runtime.printBool->hasParamAttribute(0, llvm::Attribute::ZExt));
}
