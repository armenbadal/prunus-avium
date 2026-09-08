#include "codegen.hxx"

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_ostream.h>

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace avium {

namespace {

void verifyGeneratedModule(const llvm::Module& module)
{
    std::string message;
    llvm::raw_string_ostream output{message};
    if( llvm::verifyModule(module, &output) )
        throw std::runtime_error{"Ստեղծված LLVM module-ն անվավեր է։\n" + output.str()};
}

std::string mangledName(std::string_view name)
{
    return "avium.sub." + std::string{name};
}

} // namespace

CodeGenerator::CodeGenerator(llvm::LLVMContext& context, const SymbolTable& symbols, const SemanticModel& model)
    : _context{context}
    , _symbols{symbols}
    , _model{model}
    , _builder{context}
{
}

std::unique_ptr<llvm::Module> CodeGenerator::generate(const Program& program, std::string_view moduleName)
{
    _builder.ClearInsertionPoint();
    _functions.clear();
    _storage.clear();
    _module = std::make_unique<llvm::Module>(moduleName, _context);

    declareSubroutines(program);
    for( const auto& subroutine : program._subroutines )
        defineSubroutine(*subroutine);
    createEntryPoint();

    verifyGeneratedModule(*_module);
    _builder.ClearInsertionPoint();
    _functions.clear();
    _storage.clear();
    return std::move(_module);
}

llvm::Type* CodeGenerator::llvmType(TypeName type) const
{
    switch( type ) {
        case TypeName::Bool:
            return llvm::Type::getInt1Ty(_context);
        case TypeName::Real:
            return llvm::Type::getDoubleTy(_context);
        case TypeName::Text:
            return llvm::PointerType::get(_context, 0);
    }
    std::unreachable();
}

llvm::StructType* CodeGenerator::arrayType() const
{
    auto* data = llvm::PointerType::get(_context, 0);
    auto* length = llvm::Type::getInt64Ty(_context);
    return llvm::StructType::get(_context, {data, length});
}

SymbolId CodeGenerator::symbolId(const Node& node) const
{
    return *_model.symbol(node.id());
}

void CodeGenerator::declareSubroutines(const Program& program)
{
    for( const auto& subroutine : program._subroutines ) {
        const auto id = symbolId(*subroutine);
        const auto& symbol = _symbols.symbol(id);
        const auto& signature = *symbol.subroutine;

        std::vector<llvm::Type*> parameterTypes;
        parameterTypes.reserve(signature.parameters.size());
        for( const auto& parameter : signature.parameters ) {
            if( parameter.isArray )
                parameterTypes.push_back(llvm::PointerType::get(_context, 0));
            else
                parameterTypes.push_back(llvmType(*parameter.type));
        }

        auto* returnType = signature.returnType.has_value() ? llvmType(*signature.returnType) : llvm::Type::getVoidTy(_context);
        auto* functionType = llvm::FunctionType::get(returnType, parameterTypes, false);
        const auto name = mangledName(signature.name);
        auto* function = llvm::Function::Create(functionType, llvm::Function::ExternalLinkage, name, *_module);
        _functions.emplace(id, function);
    }
}

void CodeGenerator::defineSubroutine(const Subroutine& subroutine)
{
    const auto id = symbolId(subroutine);
    auto* function = _functions.at(id);
    auto* entry = llvm::BasicBlock::Create(_context, "entry", function);
    _builder.SetInsertPoint(entry);

    allocateParameters(subroutine, *function);
    allocateReturnValue(subroutine);
    allocateLocals(*subroutine._body);

    if( subroutine._returnType.has_value() ) {
        const auto returnId = *_model.returnValue(subroutine.id());
        const auto& symbol = _symbols.symbol(returnId);
        auto* value = _builder.CreateLoad(llvmType(*symbol.type), _storage.at(returnId));
        _builder.CreateRet(value);
    }
    else {
        _builder.CreateRetVoid();
    }
}

void CodeGenerator::allocateParameters(const Subroutine& subroutine, llvm::Function& function)
{
    auto argument = function.arg_begin();
    for( const auto& parameter : subroutine._parameters ) {
        const auto id = symbolId(*parameter);
        argument->setName(parameter->_name);

        if( parameter->_isArray ) {
            _storage.emplace(id, &*argument);
        }
        else {
            auto* address = _builder.CreateAlloca(llvmType(parameter->_type), nullptr, parameter->_name);
            _builder.CreateStore(&*argument, address);
            _storage.emplace(id, address);
        }
        ++argument;
    }
}

void CodeGenerator::allocateReturnValue(const Subroutine& subroutine)
{
    const auto id = _model.returnValue(subroutine.id());
    if( !id.has_value() )
        return;
    allocateVariable(*id);
}

void CodeGenerator::allocateLocals(const Sequence& sequence)
{
    for( const auto& statement : sequence._items ) {
        switch( statement->kind ) {
            case NodeKind::Dim:
                allocateVariable(symbolId(*statement));
                break;
            case NodeKind::If: {
                const auto& conditional = static_cast<const If&>(*statement);
                for( const auto& branch : conditional._branches )
                    allocateLocals(*branch->_body);
                if( conditional._alternative )
                    allocateLocals(*conditional._alternative);
                break;
            }
            case NodeKind::While:
                allocateLocals(*static_cast<const While&>(*statement)._body);
                break;
            case NodeKind::For: {
                const auto& loop = static_cast<const For&>(*statement);
                allocateVariable(symbolId(*loop._parameter));
                allocateLocals(*loop._body);
                break;
            }
            default:
                break;
        }
    }
}

void CodeGenerator::allocateVariable(SymbolId id)
{
    if( _storage.contains(id) )
        return;

    const auto& symbol = _symbols.symbol(id);
    auto* type = symbol.isArray ? static_cast<llvm::Type*>(arrayType()) : llvmType(*symbol.type);
    auto* address = _builder.CreateAlloca(type, nullptr, symbol.name);
    auto* initialValue = llvm::Constant::getNullValue(type);
    _builder.CreateStore(initialValue, address);
    _storage.emplace(id, address);
}

void CodeGenerator::createEntryPoint()
{
    auto* returnType = llvm::Type::getInt32Ty(_context);
    auto* functionType = llvm::FunctionType::get(returnType, false);
    auto* main = llvm::Function::Create(functionType, llvm::Function::ExternalLinkage, "main", *_module);
    auto* entry = llvm::BasicBlock::Create(_context, "entry", main);
    _builder.SetInsertPoint(entry);

    const auto mainId = *_model.entryPoint();
    _builder.CreateCall(_functions.at(mainId));
    _builder.CreateRet(llvm::ConstantInt::get(returnType, 0));
}

} // namespace avium
