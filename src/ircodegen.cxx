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

    visit(static_cast<Node&>(_program));
    _builder.ClearInsertionPoint();

    if( llvm::verifyModule(*_module, &llvm::errs()) )
        llvm::report_fatal_error("IR code generation produced an invalid module");

    return std::move(_module);
}

void IRCodeGen::visit(Program& program)
{
    const auto entryPoint = _model.entryPoint();
    if( !entryPoint )
        llvm::report_fatal_error("IR code generation requires a semantic entry point");

    const auto* entrySymbol = _symbols.subroutine(*entryPoint);
    if( entrySymbol == nullptr )
        llvm::report_fatal_error("IR code generation entry point is not a subroutine");

    Subroutine* entrySubroutine = nullptr;
    for( const auto& subroutine : program._subroutines ) {
        const auto symbol = _model.symbol(subroutine->id());
        if( symbol && *symbol == *entryPoint ) {
            entrySubroutine = subroutine.get();
            break;
        }
    }
    if( entrySubroutine == nullptr )
        llvm::report_fatal_error("IR code generation cannot find the entry-point AST node");

    visit(*entrySubroutine);

    auto* entryFunction = _module->getFunction(subroutineName(entrySymbol->id));
    if( entryFunction == nullptr )
        llvm::report_fatal_error("IR code generation did not create the entry-point function");

    const auto mainType = llvm::FunctionType::get(llvm::Type::getInt32Ty(_context), false);
    auto* main = llvm::Function::Create(mainType, llvm::Function::ExternalLinkage, "main", *_module);
    auto* mainEntry = llvm::BasicBlock::Create(_context, "entry", main);
    _builder.SetInsertPoint(mainEntry);
    _builder.CreateCall(entryFunction);
    _builder.CreateRet(_builder.getInt32(0));
}

void IRCodeGen::visit(Subroutine& subroutine)
{
    const auto symbol = _model.symbol(subroutine.id());
    if( !symbol )
        llvm::report_fatal_error("IR code generation requires a resolved subroutine");

    const auto* subroutineSymbol = _symbols.subroutine(*symbol);
    if( subroutineSymbol == nullptr )
        llvm::report_fatal_error("IR code generation symbol is not a subroutine");

    const auto functionType = createFunctionType(subroutineSymbol->signature);
    auto* function = llvm::Function::Create(functionType,
        llvm::Function::InternalLinkage, subroutineName(subroutineSymbol->id), *_module);
    auto* entryBlock = llvm::BasicBlock::Create(_context, "entry", function);
    _builder.SetInsertPoint(entryBlock);
    visit(*subroutine._body);
    _builder.CreateRetVoid();
}

void IRCodeGen::visit(Sequence& sequence)
{
    for( const auto& statement : sequence._items )
        visit(*statement);
}

void IRCodeGen::visit(Dim& node)
{
    unsupportedNode(node);
}

void IRCodeGen::visit(Let& node)
{
    unsupportedNode(node);
}

void IRCodeGen::visit(If& node)
{
    unsupportedNode(node);
}

void IRCodeGen::visit(IfBranch& node)
{
    unsupportedNode(node);
}

void IRCodeGen::visit(While& node)
{
    unsupportedNode(node);
}

void IRCodeGen::visit(For& node)
{
    unsupportedNode(node);
}

void IRCodeGen::visit(Call& node)
{
    unsupportedNode(node);
}

void IRCodeGen::visit(Return& node)
{
    unsupportedNode(node);
}

void IRCodeGen::visit(Apply& node)
{
    unsupportedNode(node);
}

void IRCodeGen::visit(ScalarType& node)
{
    unsupportedNode(node);
}

void IRCodeGen::visit(ArrayType& node)
{
    unsupportedNode(node);
}

void IRCodeGen::visit(Binary& node)
{
    unsupportedNode(node);
}

void IRCodeGen::visit(Unary& node)
{
    unsupportedNode(node);
}

void IRCodeGen::visit(Variable& node)
{
    unsupportedNode(node);
}

void IRCodeGen::visit(Text& node)
{
    unsupportedNode(node);
}

void IRCodeGen::visit(Number& node)
{
    unsupportedNode(node);
}

void IRCodeGen::visit(Boolean& node)
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
