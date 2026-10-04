#include "ircodegen.hxx"

#include "ast.hxx"
#include "semantic.hxx"
#include "symbols.hxx"

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/TargetParser/Triple.h>

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace avium {

namespace {

std::string subroutineName(SymbolId symbol)
{
    return "avium.subroutine." + std::to_string(symbol);
}

Error unsupportedNode(const Node& node)
{
    return {node.line, "IR code generation does not support this AST node"};
}

} // namespace

IRCodeGen::IRCodeGen(llvm::LLVMContext& context, Program& program, const SymbolTable& symbols, const SemanticModel& model)
    : _context{context}
    , _program{program}
    , _builder{context}
    , _symbols{symbols}
    , _model{model}
{
}

std::expected<std::unique_ptr<llvm::Module>, Error> IRCodeGen::generate()
{
    _module = std::make_unique<llvm::Module>("prunus", _context);
    if( auto result = configureTarget(); !result )
        return std::unexpected(std::move(result.error()));
    _runtime = std::make_unique<RuntimeAbi>(*_module);

    declareSubroutines();
    if( auto result = emitSubroutines(); !result ) {
        _builder.ClearInsertionPoint();
        return std::unexpected(std::move(result.error()));
    }
    if( auto result = emitMainWrapper(); !result ) {
        _builder.ClearInsertionPoint();
        return std::unexpected(std::move(result.error()));
    }
    _builder.ClearInsertionPoint();

    std::string verificationError;
    llvm::raw_string_ostream errorStream{verificationError};
    if( llvm::verifyModule(*_module, &errorStream) ) {
        errorStream.flush();
        return std::unexpected(Error{_program.line, "IR code generation produced an invalid module: " + verificationError});
    }

    return std::move(_module);
}

std::expected<void, Error> IRCodeGen::configureTarget()
{
    if( llvm::InitializeNativeTarget() )
        return std::unexpected(Error{_program.line, "IR code generation could not initialize the native target"});

    const llvm::Triple triple{llvm::sys::getDefaultTargetTriple()};
    std::string targetError;
    const auto* target = llvm::TargetRegistry::lookupTarget(triple, targetError);
    if( target == nullptr )
        return std::unexpected(Error{_program.line, "IR code generation could not find the native target: " + targetError});

    llvm::TargetOptions options;
    std::unique_ptr<llvm::TargetMachine> targetMachine{
        target->createTargetMachine(triple, llvm::sys::getHostCPUName(), "", options, std::nullopt)};
    if( targetMachine == nullptr )
        return std::unexpected(Error{_program.line, "IR code generation could not create the native target machine"});

    _module->setTargetTriple(triple);
    _module->setDataLayout(targetMachine->createDataLayout());
    return {};
}

void IRCodeGen::declareSubroutines()
{
    for( const auto& subroutine : _program._subroutines ) {
        const auto& subroutineSymbol = *_symbols.subroutine(*_model.symbol(subroutine->id()));
        const auto functionType = createFunctionType(subroutineSymbol.signature);
        llvm::Function::Create(functionType, llvm::Function::InternalLinkage,
            subroutineName(subroutineSymbol.id), *_module);
    }
}

std::expected<void, Error> IRCodeGen::emitSubroutines()
{
    for( const auto& subroutine : _program._subroutines )
        if( auto result = emit(*subroutine); !result )
            return std::unexpected(std::move(result.error()));
    return {};
}

std::expected<void, Error> IRCodeGen::emitMainWrapper()
{
    auto* entryFunction = _module->getFunction(subroutineName(*_model.entryPoint()));
    if( entryFunction == nullptr )
        return std::unexpected(Error{_program.line, "IR code generation did not declare the entry-point function"});

    const auto mainType = llvm::FunctionType::get(llvm::Type::getInt32Ty(_context), false);
    auto* main = llvm::Function::Create(mainType, llvm::Function::ExternalLinkage, "main", *_module);
    auto* mainEntry = llvm::BasicBlock::Create(_context, "entry", main);
    _builder.SetInsertPoint(mainEntry);
    _builder.CreateCall(entryFunction);
    _builder.CreateRet(_builder.getInt32(0));
    return {};
}

std::expected<void, Error> IRCodeGen::emit(Subroutine& subroutine)
{
    const auto& subroutineSymbol = *_symbols.subroutine(*_model.symbol(subroutine.id()));
    auto* function = _module->getFunction(subroutineName(subroutineSymbol.id));
    if( function == nullptr )
        return std::unexpected(Error{subroutine.line, "IR code generation requires a declared function"});

    auto* entryBlock = llvm::BasicBlock::Create(_context, "entry", function);
    _builder.SetInsertPoint(entryBlock);
    if( auto result = emit(*subroutine._body); !result )
        return std::unexpected(std::move(result.error()));

    if( _builder.GetInsertBlock()->getTerminator() == nullptr )
        _builder.CreateRetVoid();
    return {};
}

std::expected<void, Error> IRCodeGen::emit(Sequence& sequence)
{
    for( const auto& statement : sequence._items )
        if( auto result = emit(*statement); !result )
            return std::unexpected(std::move(result.error()));
    return {};
}

std::expected<void, Error> IRCodeGen::emit(Statement& statement)
{
    switch( statement.kind ) {
        case NodeKind::Dim:
            return emit(static_cast<Dim&>(statement));
        case NodeKind::Let:
            return emit(static_cast<Let&>(statement));
        case NodeKind::If:
            return emit(static_cast<If&>(statement));
        case NodeKind::While:
            return emit(static_cast<While&>(statement));
        case NodeKind::For:
            return emit(static_cast<For&>(statement));
        case NodeKind::Call:
            return emit(static_cast<Call&>(statement));
        case NodeKind::Return:
            return emit(static_cast<Return&>(statement));
        default:
            std::unreachable();
    }
}

std::expected<void, Error> IRCodeGen::emit(Dim& node)
{
    return std::unexpected(unsupportedNode(node));
}

std::expected<void, Error> IRCodeGen::emit(Let& node)
{
    return std::unexpected(unsupportedNode(node));
}

std::expected<void, Error> IRCodeGen::emit(If& node)
{
    return std::unexpected(unsupportedNode(node));
}

std::expected<void, Error> IRCodeGen::emit(While& node)
{
    return std::unexpected(unsupportedNode(node));
}

std::expected<void, Error> IRCodeGen::emit(For& node)
{
    return std::unexpected(unsupportedNode(node));
}

std::expected<void, Error> IRCodeGen::emit(Call& node)
{
    return std::unexpected(unsupportedNode(node));
}

std::expected<void, Error> IRCodeGen::emit(Return& node)
{
    return std::unexpected(unsupportedNode(node));
}

std::expected<llvm::Value*, Error> IRCodeGen::emit(Expression& expression)
{
    switch( expression.kind ) {
        case NodeKind::Apply:
            return emit(static_cast<Apply&>(expression));
        case NodeKind::Binary:
            return emit(static_cast<Binary&>(expression));
        case NodeKind::Unary:
            return emit(static_cast<Unary&>(expression));
        case NodeKind::Variable:
            return emit(static_cast<Variable&>(expression));
        case NodeKind::Text:
            return emit(static_cast<Text&>(expression));
        case NodeKind::Number:
            return emit(static_cast<Number&>(expression));
        case NodeKind::Boolean:
            return emit(static_cast<Boolean&>(expression));
        default:
            std::unreachable();
    }
}

std::expected<llvm::Value*, Error> IRCodeGen::emit(Apply& node)
{
    return std::unexpected(unsupportedNode(node));
}

std::expected<llvm::Value*, Error> IRCodeGen::emit(Binary& node)
{
    return std::unexpected(unsupportedNode(node));
}

std::expected<llvm::Value*, Error> IRCodeGen::emit(Unary& node)
{
    return std::unexpected(unsupportedNode(node));
}

std::expected<llvm::Value*, Error> IRCodeGen::emit(Variable& node)
{
    return std::unexpected(unsupportedNode(node));
}

std::expected<llvm::Value*, Error> IRCodeGen::emit(Text& node)
{
    return std::unexpected(unsupportedNode(node));
}

std::expected<llvm::Value*, Error> IRCodeGen::emit(Number& node)
{
    return std::unexpected(unsupportedNode(node));
}

std::expected<llvm::Value*, Error> IRCodeGen::emit(Boolean& node)
{
    return std::unexpected(unsupportedNode(node));
}

llvm::FunctionType* IRCodeGen::createFunctionType(const SubroutineSignature& ss) const
{
    const auto returnsText = ss.returnType != nullptr && ss.returnType->_name == ScalarType::Name::Text;

    std::vector<llvm::Type*> parameters;
    parameters.reserve(ss.parameters.size() + returnsText);

    if( returnsText )
        parameters.push_back(fromCerasusType(*ss.returnType));

    for( const auto* parameter : ss.parameters ) {
        switch( parameter->kind ) {
            case NodeKind::ScalarType:
                parameters.push_back(fromCerasusType(
                    static_cast<const ScalarType&>(*parameter)));
                break;
            case NodeKind::ArrayType:
                parameters.push_back(fromCerasusType(
                    static_cast<const ArrayType&>(*parameter)));
                break;
            default:
                std::unreachable();
        }
    }

    auto* returnType = llvm::Type::getVoidTy(_context);
    if( ss.returnType != nullptr && !returnsText )
        returnType = fromCerasusType(*ss.returnType);

    return llvm::FunctionType::get(returnType, parameters, false);
}

llvm::Type* IRCodeGen::fromCerasusType(const ScalarType& t) const
{
    switch( t._name ) {
        case ScalarType::Name::Bool:
            return llvm::Type::getInt1Ty(_context);
        case ScalarType::Name::Real:
            return llvm::Type::getDoubleTy(_context);
        case ScalarType::Name::Text:
            return llvm::PointerType::getUnqual(_context);
    }

    std::unreachable();
}

llvm::Type* IRCodeGen::fromCerasusType(const ArrayType&) const
{
    return llvm::PointerType::getUnqual(_context);
}

} // namespace avium
