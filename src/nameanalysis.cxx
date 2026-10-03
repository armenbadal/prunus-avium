#include "semanticpasses.hxx"

#include <format>
#include <utility>
#include <vector>

namespace avium {

namespace {

bool hasScalarType(const Type* type, ScalarType::Name name)
{
    return type != nullptr && !type->isArray() && type->base()._name == name;
}

const std::vector<SubroutineSymbol>& builtinSubroutines()
{
    const auto& real = scalarType(ScalarType::Name::Real);
    const auto& text = scalarType(ScalarType::Name::Text);
    static const std::vector<SubroutineSymbol> subroutines{
        {"Print", {{&text}, nullptr}, true},
        {"Input", {{}, &text}, true},
        {"NUM", {{&text}, &real}, true},
        {"SQR", {{&real}, &real}, true},
        {"STR", {{nullptr}, &text}, true},
        {"LEN", {{nullptr}, &real}, true},
    };
    return subroutines;
}

class NameDeclarationPass {
public:
    explicit NameDeclarationPass(SemanticContext& context)
        : _context{context}
    {
    }

    void declareGlobals(const Program& program);
    void declareLocals(const Subroutine& subroutine);

private:
    void declareBuiltins();
    void declareSubroutines(const Program& program);
    void resolveEntryPoint(const Program& program);
    void declareParameters(const Subroutine& subroutine);
    void declareLocals(const Sequence& sequence);
    void declareDim(const Dim& dim);
    void declareForVariable(const For& loop);

    SemanticContext& _context;
};

class NameResolutionPass {
public:
    explicit NameResolutionPass(SemanticContext& context)
        : _context{context}
    {
    }

    void resolve(const Subroutine& subroutine);

private:
    void resolve(const Sequence& sequence);
    void resolve(const Statement& statement);
    void resolve(const IfBranch& branch);
    void resolve(const Expression& expression);

    void resolveVariable(const Variable& variable);
    void resolveSubroutine(const Node& node, std::string_view name);

    SemanticContext& _context;
};

void NameDeclarationPass::declareGlobals(const Program& program)
{
    declareBuiltins();
    declareSubroutines(program);
    resolveEntryPoint(program);
}

void NameDeclarationPass::declareLocals(const Subroutine& subroutine)
{
    declareParameters(subroutine);
    declareLocals(*subroutine._body);
}

void NameDeclarationPass::declareBuiltins()
{
    for( const auto& subroutine : builtinSubroutines() )
        _context.symbols.declareSubroutine(subroutine);
}

void NameDeclarationPass::declareSubroutines(const Program& program)
{
    for( const auto& subroutine : program._subroutines ) {
        std::vector<const Type*> parameters;
        parameters.reserve(subroutine->_parameters.size());
        for( const auto& parameter : subroutine->_parameters )
            parameters.push_back(parameter->_type.get());

        const auto existing = _context.symbols.lookupSubroutine(subroutine->_name);
        if( existing ) {
            const auto* symbol = _context.symbols.subroutine(*existing);
            if( symbol->builtin )
                _context.report(*subroutine, std::format("'{}' անունը պատկանում է ներդրված ենթածրագրի։", subroutine->_name));
            else
                _context.report(*subroutine, std::format("'{}' ենթածրագիրն արդեն սահմանված է։", subroutine->_name));
            continue;
        }

        const auto id = _context.symbols.declareSubroutine({subroutine->_name, {std::move(parameters), subroutine->_returnType.get()}});
        _context.model.bind(subroutine->id(), id);
    }
}

void NameDeclarationPass::resolveEntryPoint(const Program& program)
{
    const Subroutine* main = nullptr;
    for( const auto& subroutine : program._subroutines )
        if( subroutine->_name == "Main" && main == nullptr )
            main = subroutine.get();

    if( main == nullptr ) {
        _context.report(program, "Main ենթածրագիրը բացակայում է։");
        return;
    }

    if( const auto id = _context.model.symbol(main->id()) )
        _context.model.setEntryPoint(*id);
    if( !main->_parameters.empty() )
        _context.report(*main, "'Main' ենթածրագիրը պարամետրեր չի կարող ունենալ։");
    if( main->_returnType )
        _context.report(*main, "'Main' ենթածրագիրը արժեք չի կարող վերադարձնել։");
}

void NameDeclarationPass::declareParameters(const Subroutine& subroutine)
{
    for( const auto& parameter : subroutine._parameters ) {
        const auto id = _context.symbols.declareVariable(parameter->_name, *parameter->_type, VariableStorage::Parameter);
        if( id == UnknownSymbol )
            _context.report(*parameter, std::format("'{}' անունն արդեն սահմանված է այս ենթածրագրում։", parameter->_name));
        else
            _context.model.bind(parameter->id(), id);
    }
}

void NameDeclarationPass::declareLocals(const Sequence& sequence)
{
    for( const auto& statement : sequence._items )
        switch( statement->kind ) {
            case NodeKind::Dim:
                declareDim(static_cast<const Dim&>(*statement));
                break;
            case NodeKind::If: {
                const auto& conditional = static_cast<const If&>(*statement);
                for( const auto& branch : conditional._branches )
                    declareLocals(*branch->_body);
                if( conditional._alternative )
                    declareLocals(*conditional._alternative);
                break;
            }
            case NodeKind::While:
                declareLocals(*static_cast<const While&>(*statement)._body);
                break;
            case NodeKind::For: {
                const auto& loop = static_cast<const For&>(*statement);
                declareForVariable(loop);
                declareLocals(*loop._body);
                break;
            }
            default:
                break;
        }
}

void NameDeclarationPass::declareDim(const Dim& dim)
{
    const auto id = _context.symbols.declareVariable(dim._name, *dim._type, VariableStorage::Local);
    if( id == UnknownSymbol )
        _context.report(dim, std::format("'{}' անունն արդեն սահմանված է այս ենթածրագրում։", dim._name));
    else
        _context.model.bind(dim.id(), id);
}

void NameDeclarationPass::declareForVariable(const For& loop)
{
    const auto& name = loop._parameter->_name;
    if( _context.symbols.declaredInCurrentScope(name) ) {
        const auto id = *_context.symbols.lookup(name);
        const auto* symbol = _context.symbols.variable(id);
        _context.model.bind(loop._parameter->id(), id);
        if( symbol == nullptr || !hasScalarType(symbol->type, ScalarType::Name::Real) )
            _context.report(loop, std::format("FOR-ի '{}' հաշվիչը պետք է լինի պարզ REAL փոփոխական։", name));
        return;
    }

    const auto id = _context.symbols.declareVariable(name, scalarType(ScalarType::Name::Real), VariableStorage::ForVariable);
    _context.model.bind(loop._parameter->id(), id);
}

void NameResolutionPass::resolve(const Subroutine& subroutine)
{
    resolve(*subroutine._body);
}

void NameResolutionPass::resolve(const Sequence& sequence)
{
    for( const auto& statement : sequence._items )
        resolve(*statement);
}

void NameResolutionPass::resolve(const Statement& statement)
{
    switch( statement.kind ) {
        case NodeKind::Dim: {
            const auto& dim = static_cast<const Dim&>(statement);
            if( dim._type->isArray() ) {
                const auto& array = static_cast<const ArrayType&>(*dim._type);
                if( array._size )
                    resolve(*array._size);
            }
            return;
        }
        case NodeKind::Let: {
            const auto& let = static_cast<const Let&>(statement);
            resolve(*let._variable);
            if( let._index )
                resolve(*let._index);
            resolve(*let._value);
            return;
        }
        case NodeKind::If: {
            const auto& conditional = static_cast<const If&>(statement);
            for( const auto& branch : conditional._branches )
                resolve(*branch);
            if( conditional._alternative )
                resolve(*conditional._alternative);
            return;
        }
        case NodeKind::While: {
            const auto& loop = static_cast<const While&>(statement);
            resolve(*loop._condition);
            resolve(*loop._body);
            return;
        }
        case NodeKind::For: {
            const auto& loop = static_cast<const For&>(statement);
            resolve(*loop._parameter);
            resolve(*loop._begin);
            resolve(*loop._end);
            resolve(*loop._step);
            resolve(*loop._body);
            return;
        }
        case NodeKind::Call: {
            const auto& call = static_cast<const Call&>(statement);
            resolveSubroutine(call, call._callee);
            for( const auto& argument : call._arguments )
                resolve(*argument);
            return;
        }
        case NodeKind::Return:
            resolve(*static_cast<const Return&>(statement)._value);
            return;
        default:
            std::unreachable();
    }
}

void NameResolutionPass::resolve(const IfBranch& branch)
{
    resolve(*branch._condition);
    resolve(*branch._body);
}

void NameResolutionPass::resolve(const Expression& expression)
{
    switch( expression.kind ) {
        case NodeKind::Boolean:
        case NodeKind::Number:
        case NodeKind::Text:
            return;
        case NodeKind::Variable:
            resolveVariable(static_cast<const Variable&>(expression));
            return;
        case NodeKind::Unary:
            resolve(*static_cast<const Unary&>(expression)._operand);
            return;
        case NodeKind::Binary: {
            const auto& binary = static_cast<const Binary&>(expression);
            resolve(*binary._left);
            resolve(*binary._right);
            return;
        }
        case NodeKind::Apply: {
            const auto& apply = static_cast<const Apply&>(expression);
            resolveSubroutine(apply, apply._callee);
            for( const auto& argument : apply._arguments )
                resolve(*argument);
            return;
        }
        default:
            std::unreachable();
    }
}

void NameResolutionPass::resolveVariable(const Variable& variable)
{
    const auto id = _context.symbols.lookup(variable._name);
    if( !id ) {
        _context.report(variable, std::format("'{}' անունով փոփոխական սահմանված չէ։", variable._name));
        return;
    }

    if( _context.symbols.variable(*id) == nullptr ) {
        _context.report(variable, std::format("'{}' անունը փոփոխական չէ։", variable._name));
        return;
    }

    _context.model.bind(variable.id(), *id);
}

void NameResolutionPass::resolveSubroutine(const Node& node, std::string_view name)
{
    const auto id = _context.symbols.lookupSubroutine(name);
    if( !id ) {
        _context.report(node, std::format("'{}' անունով ենթածրագիր սահմանված չէ։", name));
        return;
    }
    _context.model.bind(node.id(), *id);
}

} // namespace

void analyzeNames(const Program& program, SemanticContext& context)
{
    NameDeclarationPass declarations{context};
    declarations.declareGlobals(program);

    NameResolutionPass resolution{context};
    for( const auto& subroutine : program._subroutines ) {
        context.symbols.openScope();
        declarations.declareLocals(*subroutine);
        resolution.resolve(*subroutine);
        context.symbols.closeScope();
    }
}

} // namespace avium
