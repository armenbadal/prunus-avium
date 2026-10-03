#include "ircodegen.hxx"

#include "ast.hxx"
#include "semantic.hxx"
#include "symbols.hxx"

#include <llvm/ADT/Twine.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/TargetParser/Triple.h>

#include <string>
#include <utility>
#include <vector>

namespace avium {

namespace {

std::string subroutineName(SymbolId symbol)
{
    return "avium.subroutine." + std::to_string(symbol);
}

[[noreturn]] void unsupportedNode(const Node& node)
{
    llvm::report_fatal_error(
        llvm::Twine{"IR code generation does not support the AST node at line "} + llvm::Twine{node.line});
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

std::unique_ptr<llvm::Module> IRCodeGen::generate()
{
    _module = std::make_unique<llvm::Module>("prunus", _context);
    _module->setTargetTriple(llvm::Triple{llvm::sys::getDefaultTargetTriple()});

    declareSubroutines();
    emitSubroutines();
    emitMainWrapper();
    _builder.ClearInsertionPoint();

    if( llvm::verifyModule(*_module, &llvm::errs()) )
        llvm::report_fatal_error("IR code generation produced an invalid module");

    return std::move(_module);
}

void IRCodeGen::declareSubroutines()
{
    for( const auto& subroutine : _program._subroutines ) {
        const auto symbol = _model.symbol(subroutine->id());
        if( !symbol )
            llvm::report_fatal_error("IR code generation requires a resolved subroutine");

        const auto* subroutineSymbol = _symbols.subroutine(*symbol);
        if( subroutineSymbol == nullptr )
            llvm::report_fatal_error("IR code generation symbol is not a subroutine");

        const auto functionType = createFunctionType(subroutineSymbol->signature);
        llvm::Function::Create(functionType, llvm::Function::InternalLinkage,
            subroutineName(subroutineSymbol->id), *_module);
    }
}

void IRCodeGen::emitSubroutines()
{
    for( const auto& subroutine : _program._subroutines )
        emit(*subroutine);
}

void IRCodeGen::emitMainWrapper()
{
    const auto entryPoint = _model.entryPoint();
    if( !entryPoint )
        llvm::report_fatal_error("IR code generation requires a semantic entry point");

    auto* entryFunction = _module->getFunction(subroutineName(*entryPoint));
    if( entryFunction == nullptr )
        llvm::report_fatal_error("IR code generation did not declare the entry-point function");

    const auto mainType = llvm::FunctionType::get(llvm::Type::getInt32Ty(_context), false);
    auto* main = llvm::Function::Create(mainType, llvm::Function::ExternalLinkage, "main", *_module);
    auto* mainEntry = llvm::BasicBlock::Create(_context, "entry", main);
    _builder.SetInsertPoint(mainEntry);
    _builder.CreateCall(entryFunction);
    _builder.CreateRet(_builder.getInt32(0));
}

void IRCodeGen::emit(Subroutine& subroutine)
{
    const auto symbol = _model.symbol(subroutine.id());
    if( !symbol )
        llvm::report_fatal_error("IR code generation requires a resolved subroutine");

    const auto* subroutineSymbol = _symbols.subroutine(*symbol);
    if( subroutineSymbol == nullptr )
        llvm::report_fatal_error("IR code generation symbol is not a subroutine");

    auto* function = _module->getFunction(subroutineName(subroutineSymbol->id));
    if( function == nullptr )
        llvm::report_fatal_error("IR code generation requires a declared function");

    auto* entryBlock = llvm::BasicBlock::Create(_context, "entry", function);
    _builder.SetInsertPoint(entryBlock);
    emit(*subroutine._body);

    if( _builder.GetInsertBlock()->getTerminator() == nullptr ) {
        if( !function->getReturnType()->isVoidTy() )
            llvm::report_fatal_error("IR code generation produced a function without a return value");
        _builder.CreateRetVoid();
    }
}

void IRCodeGen::emit(Sequence& sequence)
{
    for( const auto& statement : sequence._items )
        emit(*statement);
}

void IRCodeGen::emit(Statement& statement)
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

void IRCodeGen::emit(Dim& node)
{
    unsupportedNode(node);
}

void IRCodeGen::emit(Let& node)
{
    unsupportedNode(node);
}

void IRCodeGen::emit(If& node)
{
    unsupportedNode(node);
}

void IRCodeGen::emit(While& node)
{
    unsupportedNode(node);
}

void IRCodeGen::emit(For& node)
{
    unsupportedNode(node);
}

void IRCodeGen::emit(Call& node)
{
    unsupportedNode(node);
}

void IRCodeGen::emit(Return& node)
{
    unsupportedNode(node);
}

llvm::Value* IRCodeGen::emit(Expression& expression)
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

llvm::Value* IRCodeGen::emit(Apply& node)
{
    unsupportedNode(node);
}

llvm::Value* IRCodeGen::emit(Binary& node)
{
    unsupportedNode(node);
}

llvm::Value* IRCodeGen::emit(Unary& node)
{
    unsupportedNode(node);
}

llvm::Value* IRCodeGen::emit(Variable& node)
{
    unsupportedNode(node);
}

llvm::Value* IRCodeGen::emit(Text& node)
{
    unsupportedNode(node);
}

llvm::Value* IRCodeGen::emit(Number& node)
{
    unsupportedNode(node);
}

llvm::Value* IRCodeGen::emit(Boolean& node)
{
    unsupportedNode(node);
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
