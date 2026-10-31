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

// Նկարագրում է ներդրված ենթածրագրերի վերնագրերը
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

    void declareGlobals(const Program& program) const;
    void declareLocals(const Subroutine& subroutine);

private:
    void declareBuiltins() const;
    void declareSubroutines(const Program& program) const;
    void resolveEntryPoint(const Program& program) const;
    void declareParameters(const Subroutine& subroutine) const;
    void declareLocals(const Sequence& sequence);
    void declareDim(const Dim& dim) const;
    void declareForVariable(const For& loop) const;

    SemanticContext& _context;
};

// Գլոբալ անունների հայտարարում
void NameDeclarationPass::declareGlobals(const Program& program) const
{
    declareBuiltins(); // ներդրված ենթածրագրեր
    declareSubroutines(program); // ծրագրավորողի ենթածրագրեր
    resolveEntryPoint(program); // մուտքի կետը
}

// Հայտարարում է լոկալ փոփոխականները
void NameDeclarationPass::declareLocals(const Subroutine& subroutine)
{
    declareParameters(subroutine); // ենթածրագրի պարամետրերը
    declareLocals(*subroutine._body); // ղեկավարող կառուցվածքներինը
}

// Կոնտեքստում հայտարարում է ներդրված ենթածրագրերը
void NameDeclarationPass::declareBuiltins() const
{
    for( const auto& subroutine : builtinSubroutines() )
        _context.symbols.declareSubroutine(subroutine);
}

// Կոնտեքստում հայտարարում է ծրագրավորողի գրած ենթածրագրերը
void NameDeclarationPass::declareSubroutines(const Program& program) const
{
    for( const auto& subroutine : program._subroutines ) {
        // հավաքել պարամետրերի տիպերը
        std::vector<const Type*> parameters;
        parameters.reserve(subroutine->_parameters.size());
        for( const auto& parameter : subroutine->_parameters )
            parameters.push_back(parameter->_type.get());

        // փնտրում ենք աննունն արդեն հայտարարվածների մեջ ...
        if( const auto existing = _context.symbols.lookupSubroutine(subroutine->_name) ) {
            if( const auto* symbol = _context.symbols.subroutine(*existing); symbol->builtin )
                _context.report(*subroutine, std::format("'{}' անունը պատկանում է ներդրված ենթածրագրի։", subroutine->_name));
            else
                _context.report(*subroutine, std::format("'{}' ենթածրագիրն արդեն սահմանված է։", subroutine->_name));
            continue;
        }

        // եթե անունը արդեն սահմանվածներից ու ներդրվածներից չէ, ապա հայտարարում ենք նոր ենթածրագիր-սիմվոլ, ...
        const auto id = _context.symbols.declareSubroutine({subroutine->_name, {std::move(parameters), subroutine->_returnType.get()}});
        // ... իսկ սեմանտիկ մոդելում AST-ի հանգույցի ID-ը կապում ենք նոր ստեղծված սիմվոլ ID-ին
        _context.model.bind(subroutine->id(), id);
    }
}

// Որոշում է ծրագրի մուտքի կետը։ Դա Main անունով, առանց պարամետրերի
// առանց վերադարձվող արժեքի պրոցեդուրա է։
void NameDeclarationPass::resolveEntryPoint(const Program& program) const
{
    // սիմվոլների աղյուսակում փնտրել Main անունով ենթածրագիրը ...
    const auto mainProcId = _context.symbols.lookupSubroutine("Main");
    if( !mainProcId.has_value() ) {
        _context.report(program, "Main ենթածրագիրը բացակայում է։");
        return;
    }

    // ... գտնել սիմվոլը (գիտենք, որ այն հաստատ կա)
    auto* mainProcSym = _context.symbols.subroutine(mainProcId.value());

    const auto* main = [&] -> const Subroutine* {
        for( const auto& subroutine : program._subroutines )
            if( subroutine->_name == "Main" )
                return subroutine.get();
        return nullptr;
    }();

    if( mainProcSym->signature.returnType != nullptr )
        _context.report(*main, "'Main' ենթածրագիրը արժեք չի կարող վերադարձնել։");
    if( !mainProcSym->signature.parameters.empty() )
        _context.report(*main, "'Main' ենթածրագիրը պարամետրեր չի կարող ունենալ։");

    _context.model.setEntryPoint(*mainProcId);
}

// Ենթածրագրի պարամետրերը հայտարարում է ընթացիկ տիրույթում
void NameDeclarationPass::declareParameters(const Subroutine& subroutine) const
{
    for( const auto& parameter : subroutine._parameters ) {
        const auto id = _context.symbols.declareVariable(parameter->_name, *parameter->_type, VariableStorage::Parameter);
        if( id == UnknownSymbol )
            _context.report(*parameter, std::format("'{}' անունն արդեն սահմանված է այս ենթածրագրում։", parameter->_name));
        else
            _context.model.bind(parameter->id(), id);
    }
}

// Հայտարարում է տրված բլոկի բոլոր փոփոխականները որպես լոկալ
void NameDeclarationPass::declareLocals(const Sequence& sequence)
{
    for( const auto& statement : sequence._items )
        switch( statement->kind ) {
            case NodeKind::Dim:
                declareDim(dynamic_cast<const Dim&>(*statement));
                break;
            case NodeKind::If: {
                const auto& conditional = dynamic_cast<const If&>(*statement);
                for( const auto& branch : conditional._branches )
                    declareLocals(*branch->_body);
                if( conditional._alternative )
                    declareLocals(*conditional._alternative);
                break;
            }
            case NodeKind::While:
                declareLocals(*dynamic_cast<const While&>(*statement)._body);
                break;
            case NodeKind::For: {
                const auto& loop = dynamic_cast<const For&>(*statement);
                declareForVariable(loop);
                declareLocals(*loop._body);
                break;
            }
            default:
                break;
        }
}

void NameDeclarationPass::declareDim(const Dim& dim) const
{
    const auto id = _context.symbols.declareVariable(dim._name, *dim._type, VariableStorage::Local);
    if( id == UnknownSymbol )
        _context.report(dim, std::format("'{}' անունն արդեն սահմանված է այս ենթածրագրում։", dim._name));
    else
        _context.model.bind(dim.id(), id);
}

// Հայտարարում է FOR ցիկլի պարամետրը
void NameDeclarationPass::declareForVariable(const For& loop) const
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

    void resolveVariable(const Variable& variable) const;
    void resolveSubroutine(const Node& node, std::string_view name) const;

    SemanticContext& _context;
};

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
            if( const auto& dim = dynamic_cast<const Dim&>(statement); dim._type->isArray() ) {
                if( const auto& array = dynamic_cast<const ArrayType&>(*dim._type); array._size )
                    resolve(*array._size);
            }
            return;
        }
        case NodeKind::Let: {
            const auto& let = dynamic_cast<const Let&>(statement);
            resolve(*let._variable);
            if( let._index )
                resolve(*let._index);
            resolve(*let._value);
            return;
        }
        case NodeKind::If: {
            const auto& conditional = dynamic_cast<const If&>(statement);
            for( const auto& branch : conditional._branches )
                resolve(*branch);
            if( conditional._alternative )
                resolve(*conditional._alternative);
            return;
        }
        case NodeKind::While: {
            const auto& loop = dynamic_cast<const While&>(statement);
            resolve(*loop._condition);
            resolve(*loop._body);
            return;
        }
        case NodeKind::For: {
            const auto& loop = dynamic_cast<const For&>(statement);
            resolve(*loop._parameter);
            resolve(*loop._begin);
            resolve(*loop._end);
            resolve(*loop._step);
            resolve(*loop._body);
            return;
        }
        case NodeKind::Call: {
            const auto& call = dynamic_cast<const Call&>(statement);
            resolveSubroutine(call, call._callee);
            for( const auto& argument : call._arguments )
                resolve(*argument);
            return;
        }
        case NodeKind::Return:
            resolve(*dynamic_cast<const Return&>(statement)._value);
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
            break;
        case NodeKind::Variable:
            resolveVariable(dynamic_cast<const Variable&>(expression));
            break;
        case NodeKind::Unary:
            resolve(*dynamic_cast<const Unary&>(expression)._operand);
            break;
        case NodeKind::Binary: {
            const auto& binary = dynamic_cast<const Binary&>(expression);
            resolve(*binary._left);
            resolve(*binary._right);
            break;
        }
        case NodeKind::Apply: {
            const auto& apply = dynamic_cast<const Apply&>(expression);
            resolveSubroutine(apply, apply._callee);
            for( const auto& argument : apply._arguments )
                resolve(*argument);
            break;
        }
        default:
            std::unreachable();
    }
}

void NameResolutionPass::resolveVariable(const Variable& variable) const
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

void NameResolutionPass::resolveSubroutine(const Node& node, std::string_view name) const
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
