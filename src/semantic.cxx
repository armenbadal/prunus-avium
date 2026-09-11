#include "semantic.hxx"
#include "astvisitor.hxx"
#include "formatters.hxx"

#include <algorithm>
#include <cmath>
#include <format>
#include <utility>

namespace avium {

namespace {

const ScalarType& scalarType(ScalarType::Name name)
{
    static const ScalarType boolType{ScalarType::Name::Bool, 0};
    static const ScalarType realType{ScalarType::Name::Real, 0};
    static const ScalarType textType{ScalarType::Name::Text, 0};
    switch( name ) {
        case ScalarType::Name::Bool:
            return boolType;
        case ScalarType::Name::Real:
            return realType;
        case ScalarType::Name::Text:
            return textType;
    }
    std::unreachable();
}

bool typeMismatch(const Type* actual, const Type& expected)
{
    return actual != nullptr && *actual != expected;
}

bool hasScalarType(const Type* type, ScalarType::Name name)
{
    return type != nullptr && !type->isArray() && type->base()._name == name;
}

std::optional<double> constantReal(const Expression& expression)
{
    if( expression.kind == NodeKind::Number )
        return static_cast<const Number&>(expression)._value;

    if( expression.kind == NodeKind::Unary ) {
        const auto& unary = static_cast<const Unary&>(expression);
        const auto operand = constantReal(*unary._operand);
        if( !operand.has_value() )
            return std::nullopt;
        if( unary._operation == Operation::Add )
            return *operand;
        if( unary._operation == Operation::Sub )
            return -*operand;
        return std::nullopt;
    }

    if( expression.kind != NodeKind::Binary )
        return std::nullopt;

    const auto& binary = static_cast<const Binary&>(expression);
    const auto left = constantReal(*binary._left);
    const auto right = constantReal(*binary._right);
    if( !left.has_value() || !right.has_value() )
        return std::nullopt;

    switch( binary._operation ) {
        case Operation::Add:
            return *left + *right;
        case Operation::Sub:
            return *left - *right;
        case Operation::Mul:
            return *left * *right;
        case Operation::Div:
            if( *right == 0.0 )
                return std::nullopt;
            return *left / *right;
        case Operation::Quot:
            if( *right == 0.0 )
                return std::nullopt;
            return std::trunc(*left / *right);
        case Operation::Mod:
            if( *right == 0.0 )
                return std::nullopt;
            return std::fmod(*left, *right);
        case Operation::Pow:
            return std::pow(*left, *right);
        default:
            return std::nullopt;
    }
}

const std::vector<SubroutineSignature>& builtinSignatures()
{
    const auto& real = scalarType(ScalarType::Name::Real);
    const auto& text = scalarType(ScalarType::Name::Text);
    static const std::vector<SubroutineSignature> signatures{
        {"Print", {nullptr}, nullptr, true},
        {"Input", {}, &text, true},
        {"NUM", {&text}, &real, true},
        {"SQR", {&real}, &real, true},
    };
    return signatures;
}

} // namespace

void SemanticModel::bind(NodeId node, SymbolId symbol)
{
    _symbols.insert_or_assign(node, symbol);
}

void SemanticModel::setEntryPoint(SymbolId symbol)
{
    _entryPoint = symbol;
}

void SemanticModel::setType(NodeId node, const Type& type)
{
    _types.insert_or_assign(node, &type);
}

std::optional<SymbolId> SemanticModel::symbol(NodeId node) const
{
    if( const auto entry = _symbols.find(node); entry != _symbols.end() )
        return entry->second;
    return std::nullopt;
}

std::optional<SymbolId> SemanticModel::entryPoint() const
{
    return _entryPoint;
}

const Type* SemanticModel::type(NodeId node) const
{
    if( const auto entry = _types.find(node); entry != _types.end() )
        return entry->second;
    return nullptr;
}

namespace {

class NameResolutionPass : public ASTVisitor<NameResolutionPass> {
public:
    NameResolutionPass(SymbolTable& symbols, SemanticModel& model, Diagnostics& diagnostics);

    using ASTVisitor<NameResolutionPass>::visit;

    void visit(Program& node);
    void visit(Subroutine& node);
    void visit(Sequence& node);
    void visit(Dim& node);
    void visit(Let& node);
    void visit(If& node);
    void visit(IfBranch& node);
    void visit(While& node);
    void visit(For& node);
    void visit(Call& node);
    void visit(Return& node);

    void visit(ScalarType&) {}

    void visit(ArrayType& node);

    void visit(Boolean&) {}

    void visit(Number&) {}

    void visit(Text&) {}

    void visit(Variable& node);
    void visit(Unary& node);
    void visit(Binary& node);
    void visit(Apply& node);

private:
    void declareBuiltins();
    void declareSubroutines(const Program& program);
    void resolveEntryPoint(const Program& program);
    void declareParameters(const Subroutine& subroutine);
    void declareLocals(const Sequence& sequence);
    void declareDim(const Dim& dim);
    void declareForVariable(const For& loop);

    std::optional<SymbolId> resolveVariable(const Variable& variable);
    std::optional<SymbolId> resolveSubroutine(const Node& node, std::string_view name);
    void report(const Node& node, std::string_view message);

    SymbolTable& _symbols;
    SemanticModel& _model;
    Diagnostics& _diagnostics;
};

class TypeCheckingPass : public ASTVisitor<TypeCheckingPass> {
public:
    TypeCheckingPass(SymbolTable& symbols, SemanticModel& model, Diagnostics& diagnostics);

    using ASTVisitor<TypeCheckingPass>::visit;

    void visit(Program& node);
    void visit(Subroutine& node);
    void visit(Sequence& node);
    void visit(Dim& node);
    void visit(Let& node);
    void visit(If& node);
    void visit(IfBranch& node);
    void visit(While& node);
    void visit(For& node);
    void visit(Call& node);
    void visit(Return& node);

    void visit(ScalarType&) {}

    void visit(ArrayType&) {}

    void visit(Boolean& node);
    void visit(Number& node);
    void visit(Text& node);
    void visit(Variable& node);
    void visit(Unary& node);
    void visit(Binary& node);
    void visit(Apply& node);

private:
    std::optional<SymbolId> boundVariable(const Variable& variable);
    std::optional<SymbolId> boundSubroutine(const Node& node);
    void validateArguments(const Node& node, std::string_view name, const std::vector<Expression::Ptr>& arguments, const SubroutineSignature& signature);
    const Type* expressionType(Expression& expression);
    bool isArrayExpression(const Expression& expression) const;
    bool requireScalar(const Expression& expression);
    void validateIndex(Expression& index);
    bool definitelyReturns(const Sequence& sequence) const;
    bool definitelyReturns(const Statement& statement) const;
    void report(const Node& node, std::string_view message);

    SymbolTable& _symbols;
    SemanticModel& _model;
    Diagnostics& _diagnostics;
    const ScalarType* _currentReturnType{nullptr};
};

NameResolutionPass::NameResolutionPass(SymbolTable& symbols, SemanticModel& model, Diagnostics& diagnostics)
    : _symbols{symbols}
    , _model{model}
    , _diagnostics{diagnostics}
{
}

void NameResolutionPass::visit(Program& program)
{
    declareBuiltins();
    declareSubroutines(program);
    resolveEntryPoint(program);

    for( const auto& subroutine : program._subroutines )
        visit(*subroutine);
}

void NameResolutionPass::visit(Subroutine& subroutine)
{
    _symbols.openScope();
    declareParameters(subroutine);
    declareLocals(*subroutine._body);
    visit(*subroutine._body);
    _symbols.closeScope();
}

void NameResolutionPass::visit(Sequence& sequence)
{
    for( const auto& statement : sequence._items )
        visit(*statement);
}

void NameResolutionPass::visit(Dim& dim)
{
    if( dim._type->isArray() ) {
        const auto& array = static_cast<const ArrayType&>(*dim._type);
        if( array._size )
            visit(*array._size);
    }
}

void NameResolutionPass::visit(Let& let)
{
    resolveVariable(*let._variable);
    if( let._index )
        visit(*let._index);
    visit(*let._value);
}

void NameResolutionPass::visit(If& conditional)
{
    for( const auto& branch : conditional._branches )
        visit(*branch);
    if( conditional._alternative )
        visit(*conditional._alternative);
}

void NameResolutionPass::visit(IfBranch& branch)
{
    visit(*branch._condition);
    visit(*branch._body);
}

void NameResolutionPass::visit(While& loop)
{
    visit(*loop._condition);
    visit(*loop._body);
}

void NameResolutionPass::visit(For& loop)
{
    resolveVariable(*loop._parameter);
    visit(*loop._begin);
    visit(*loop._end);
    visit(*loop._step);
    visit(*loop._body);
}

void NameResolutionPass::visit(Call& call)
{
    resolveSubroutine(call, call._callee);
    for( const auto& argument : call._arguments )
        visit(*argument);
}

void NameResolutionPass::visit(Return& statement)
{
    visit(*statement._value);
}

void NameResolutionPass::visit(ArrayType& array)
{
    if( array._size )
        visit(*array._size);
}

void NameResolutionPass::visit(Variable& variable)
{
    resolveVariable(variable);
}

void NameResolutionPass::visit(Unary& unary)
{
    visit(*unary._operand);
}

void NameResolutionPass::visit(Binary& binary)
{
    visit(*binary._left);
    visit(*binary._right);
}

void NameResolutionPass::visit(Apply& apply)
{
    resolveSubroutine(apply, apply._callee);
    for( const auto& argument : apply._arguments )
        visit(*argument);
}

void NameResolutionPass::declareBuiltins()
{
    for( const auto& signature : builtinSignatures() )
        _symbols.declareSubroutine(signature);
}

void NameResolutionPass::declareSubroutines(const Program& program)
{
    for( const auto& subroutine : program._subroutines ) {
        std::vector<const Type*> parameters;
        parameters.reserve(subroutine->_parameters.size());
        for( const auto& parameter : subroutine->_parameters )
            parameters.push_back(parameter->_type.get());

        const auto existing = _symbols.lookupSubroutine(subroutine->_name);
        if( existing ) {
            const auto& symbol = _symbols.symbol(*existing);
            if( symbol.subroutine->builtin )
                report(*subroutine, std::format("'{}' անունը պատկանում է ներդրված ենթածրագրի։", subroutine->_name));
            else
                report(*subroutine, std::format("'{}' ենթածրագիրն արդեն սահմանված է։", subroutine->_name));
            continue;
        }

        const auto id = _symbols.declareSubroutine({subroutine->_name, std::move(parameters), subroutine->_returnType.get(), false});
        _model.bind(subroutine->id(), id);
    }
}

void NameResolutionPass::resolveEntryPoint(const Program& program)
{
    const Subroutine* main = nullptr;
    for( const auto& subroutine : program._subroutines )
        if( subroutine->_name == "Main" && main == nullptr )
            main = subroutine.get();

    if( main == nullptr ) {
        report(program, "Main ենթածրագիրը բացակայում է։");
        return;
    }

    if( const auto id = _model.symbol(main->id()) )
        _model.setEntryPoint(*id);
    if( !main->_parameters.empty() )
        report(*main, "'Main' ենթածրագիրը պարամետրեր չի կարող ունենալ։");
    if( main->_returnType )
        report(*main, "'Main' ենթածրագիրը արժեք չի կարող վերադարձնել։");
}

void NameResolutionPass::declareParameters(const Subroutine& subroutine)
{
    for( const auto& parameter : subroutine._parameters ) {
        const auto id = _symbols.declareVariable(parameter->_name, *parameter->_type, VariableStorage::Parameter);
        if( id == UnknownSymbol )
            report(*parameter, std::format("'{}' անունն արդեն սահմանված է այս ենթածրագրում։", parameter->_name));
        else
            _model.bind(parameter->id(), id);
    }
}

void NameResolutionPass::declareLocals(const Sequence& sequence)
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

void NameResolutionPass::declareDim(const Dim& dim)
{
    const auto id = _symbols.declareVariable(dim._name, *dim._type, VariableStorage::Local);
    if( id == UnknownSymbol )
        report(dim, std::format("'{}' անունն արդեն սահմանված է այս ենթածրագրում։", dim._name));
    else
        _model.bind(dim.id(), id);
}

void NameResolutionPass::declareForVariable(const For& loop)
{
    const auto& name = loop._parameter->_name;
    if( _symbols.declaredInCurrentScope(name) ) {
        const auto id = *_symbols.lookup(name);
        const auto& symbol = _symbols.symbol(id);
        _model.bind(loop._parameter->id(), id);
        if( symbol.kind != SymbolKind::Variable || !hasScalarType(symbol.type, ScalarType::Name::Real) )
            report(loop, std::format("FOR-ի '{}' հաշվիչը պետք է լինի պարզ REAL փոփոխական։", name));
        return;
    }

    const auto id = _symbols.declareVariable(name, scalarType(ScalarType::Name::Real), VariableStorage::ForVariable);
    _model.bind(loop._parameter->id(), id);
}

std::optional<SymbolId> NameResolutionPass::resolveVariable(const Variable& variable)
{
    const auto id = _symbols.lookup(variable._name);
    if( !id ) {
        report(variable, std::format("'{}' անունով փոփոխական սահմանված չէ։", variable._name));
        return std::nullopt;
    }

    const auto& symbol = _symbols.symbol(*id);
    if( symbol.kind != SymbolKind::Variable ) {
        report(variable, std::format("'{}' անունը փոփոխական չէ։", variable._name));
        return std::nullopt;
    }

    _model.bind(variable.id(), *id);
    return id;
}

std::optional<SymbolId> NameResolutionPass::resolveSubroutine(const Node& node, std::string_view name)
{
    const auto id = _symbols.lookupSubroutine(name);
    if( !id ) {
        report(node, std::format("'{}' անունով ենթածրագիր սահմանված չէ։", name));
        return std::nullopt;
    }
    _model.bind(node.id(), *id);
    return id;
}

void NameResolutionPass::report(const Node& node, std::string_view message)
{
    _diagnostics.advance();
    _diagnostics.mark(node.line, message);
}

TypeCheckingPass::TypeCheckingPass(SymbolTable& symbols, SemanticModel& model, Diagnostics& diagnostics)
    : _symbols{symbols}
    , _model{model}
    , _diagnostics{diagnostics}
{
}

void TypeCheckingPass::visit(Program& program)
{
    for( const auto& subroutine : program._subroutines )
        visit(*subroutine);
}

void TypeCheckingPass::visit(Subroutine& subroutine)
{
    _currentReturnType = subroutine._returnType.get();
    visit(*subroutine._body);

    if( subroutine._name != "Main" && _currentReturnType != nullptr && !definitelyReturns(*subroutine._body) )
        report(subroutine, std::format("'{}' ֆունկցիայի ոչ բոլոր կատարման ուղիներն են արժեք վերադարձնում։", subroutine._name));

    _currentReturnType = nullptr;
}

void TypeCheckingPass::visit(Sequence& sequence)
{
    for( const auto& statement : sequence._items )
        visit(*statement);
}

void TypeCheckingPass::visit(Dim& dim)
{
    if( !dim._type->isArray() )
        return;

    const auto& array = static_cast<ArrayType&>(*dim._type);
    if( !array._size ) {
        report(dim, "Զանգվածի չափը նշված չէ։");
        return;
    }

    const auto sizeType = expressionType(*array._size);
    const auto scalarSize = requireScalar(*array._size);
    const auto& realType = scalarType(ScalarType::Name::Real);
    const auto wrongSizeType = scalarSize && typeMismatch(sizeType, realType);
    if( wrongSizeType )
        report(*array._size, std::format("Զանգվածի չափը պետք է լինի REAL, բայց ստացվել է {}։", *sizeType));

    const auto size = constantReal(*array._size);
    if( size.has_value() ) {
        const auto positive = *size > 0.0;
        const auto integral = std::trunc(*size) == *size;
        const auto finite = std::isfinite(*size);
        if( !positive || !integral || !finite )
            report(*array._size, "Զանգվածի հաստատուն չափը պետք է լինի դրական ամբողջ թիվ։");
    }
}

void TypeCheckingPass::visit(Let& let)
{
    const auto target = boundVariable(*let._variable);
    if( let._index )
        validateIndex(*let._index);
    const auto valueType = expressionType(*let._value);

    if( !target.has_value() )
        return;

    const auto& symbol = _symbols.symbol(*target);
    const auto& declaredType = *symbol.type;
    const auto arrayTarget = declaredType.isArray();
    const Type& targetType = let._index && arrayTarget ? declaredType.base() : declaredType;
    bool validTarget = true;
    if( let._index && !arrayTarget ) {
        report(*let._variable, std::format("'{}' փոփոխականը զանգված չէ։", let._variable->_name));
        validTarget = false;
    }
    else if( !let._index && arrayTarget ) {
        report(*let._variable, std::format("'{}' զանգվածին ամբողջությամբ արժեք վերագրել չի կարելի։", let._variable->_name));
        validTarget = false;
    }

    if( isArrayExpression(*let._value) ) {
        report(*let._value, "Զանգվածը չի կարող վերագրվել որպես պարզ արժեք։");
        return;
    }

    const auto wrongValueType = validTarget && typeMismatch(valueType, targetType);
    if( wrongValueType )
        report(*let._value, std::format("'{}' փոփոխականին պետք է վերագրվի {}, բայց ստացվել է {}։", let._variable->_name, targetType, *valueType));
}

void TypeCheckingPass::visit(If& conditional)
{
    for( const auto& branch : conditional._branches )
        visit(*branch);
    if( conditional._alternative )
        visit(*conditional._alternative);
}

void TypeCheckingPass::visit(IfBranch& branch)
{
    const auto conditionType = expressionType(*branch._condition);
    const auto scalarCondition = requireScalar(*branch._condition);
    const auto& boolType = scalarType(ScalarType::Name::Bool);
    const auto wrongConditionType = scalarCondition && typeMismatch(conditionType, boolType);
    if( wrongConditionType )
        report(*branch._condition, std::format("Ճյուղավորման պայմանը պետք է լինի BOOL, բայց ստացվել է {}։", *conditionType));
    visit(*branch._body);
}

void TypeCheckingPass::visit(While& loop)
{
    const auto conditionType = expressionType(*loop._condition);
    const auto scalarCondition = requireScalar(*loop._condition);
    const auto& boolType = scalarType(ScalarType::Name::Bool);
    const auto wrongConditionType = scalarCondition && typeMismatch(conditionType, boolType);
    if( wrongConditionType )
        report(*loop._condition, std::format("WHILE-ի պայմանը պետք է լինի BOOL, բայց ստացվել է {}։", *conditionType));
    visit(*loop._body);
}

void TypeCheckingPass::visit(For& loop)
{
    visit(*loop._parameter);

    const auto beginType = expressionType(*loop._begin);
    const auto scalarBegin = requireScalar(*loop._begin);
    const auto& realType = scalarType(ScalarType::Name::Real);
    const auto wrongBeginType = scalarBegin && typeMismatch(beginType, realType);
    if( wrongBeginType )
        report(*loop._begin, std::format("FOR-ի սկզբնական արժեքը պետք է լինի REAL, բայց ստացվել է {}։", *beginType));

    const auto endType = expressionType(*loop._end);
    const auto scalarEnd = requireScalar(*loop._end);
    const auto wrongEndType = scalarEnd && typeMismatch(endType, realType);
    if( wrongEndType )
        report(*loop._end, std::format("FOR-ի վերջնական արժեքը պետք է լինի REAL, բայց ստացվել է {}։", *endType));

    const auto stepType = expressionType(*loop._step);
    const auto scalarStep = requireScalar(*loop._step);
    const auto wrongStepType = scalarStep && typeMismatch(stepType, realType);
    if( wrongStepType )
        report(*loop._step, std::format("FOR-ի քայլը պետք է լինի REAL, բայց ստացվել է {}։", *stepType));
    if( loop._step->_value == 0.0 )
        report(*loop._step, "FOR-ի քայլը չի կարող լինել 0։");

    visit(*loop._body);
}

void TypeCheckingPass::visit(Call& call)
{
    for( const auto& argument : call._arguments )
        expressionType(*argument);

    const auto id = boundSubroutine(call);
    if( !id.has_value() )
        return;

    const auto& signature = *_symbols.symbol(*id).subroutine;
    if( signature.returnType )
        report(call, "CALL-ով կարելի է կանչել միայն պրոցեդուրա։");
    validateArguments(call, call._callee, call._arguments, signature);
}

void TypeCheckingPass::visit(Return& statement)
{
    const auto valueType = expressionType(*statement._value);
    const auto scalarValue = requireScalar(*statement._value);

    if( _currentReturnType == nullptr ) {
        report(statement, "RETURN հրամանը թույլատրելի է միայն ֆունկցիայում։");
        return;
    }

    const auto wrongValueType = scalarValue && typeMismatch(valueType, *_currentReturnType);
    if( wrongValueType )
        report(*statement._value, std::format("Ֆունկցիայից պետք է վերադարձվի {}, բայց ստացվել է {}։", static_cast<const Type&>(*_currentReturnType), *valueType));
}

void TypeCheckingPass::visit(Boolean& boolean)
{
    _model.setType(boolean.id(), scalarType(ScalarType::Name::Bool));
}

void TypeCheckingPass::visit(Number& number)
{
    _model.setType(number.id(), scalarType(ScalarType::Name::Real));
}

void TypeCheckingPass::visit(Text& text)
{
    _model.setType(text.id(), scalarType(ScalarType::Name::Text));
}

void TypeCheckingPass::visit(Variable& variable)
{
    const auto id = boundVariable(variable);
    if( id.has_value() )
        _model.setType(variable.id(), *_symbols.symbol(*id).type);
}

void TypeCheckingPass::visit(Unary& unary)
{
    const auto operandType = expressionType(*unary._operand);
    const auto scalar = requireScalar(*unary._operand);

    switch( unary._operation ) {
        case Operation::Not: {
            const auto& boolType = scalarType(ScalarType::Name::Bool);
            const auto wrongOperandType = scalar && typeMismatch(operandType, boolType);
            if( wrongOperandType )
                report(unary, std::format("'{}' գործողության օպերանդը պետք է լինի BOOL, բայց ստացվել է {}։", unary._operation, *operandType));
            _model.setType(unary.id(), boolType);
            break;
        }
        case Operation::Add:
        case Operation::Sub: {
            const auto& realType = scalarType(ScalarType::Name::Real);
            const auto wrongOperandType = scalar && typeMismatch(operandType, realType);
            if( wrongOperandType )
                report(unary, std::format("Ունար '{}' գործողության օպերանդը պետք է լինի REAL, բայց ստացվել է {}։", unary._operation, *operandType));
            _model.setType(unary.id(), realType);
            break;
        }
        default:
            std::unreachable();
    }
}

void TypeCheckingPass::visit(Binary& binary)
{
    const auto leftType = expressionType(*binary._left);
    const auto rightType = expressionType(*binary._right);

    const Type* type = nullptr;
    switch( binary._operation ) {
        case Operation::Add:
        case Operation::Sub:
        case Operation::Mul:
        case Operation::Div:
        case Operation::Quot:
        case Operation::Mod:
        case Operation::Pow:  {
            const auto& realType = scalarType(ScalarType::Name::Real);
            const auto leftScalar = requireScalar(*binary._left);
            const auto rightScalar = requireScalar(*binary._right);
            const auto wrongLeftType = leftScalar && typeMismatch(leftType, realType);
            const auto wrongRightType = rightScalar && typeMismatch(rightType, realType);
            if( wrongLeftType )
                report(*binary._left, std::format("'{}' գործողության ձախ օպերանդը պետք է լինի REAL, բայց ստացվել է {}։", binary._operation, *leftType));
            if( wrongRightType )
                report(*binary._right, std::format("'{}' գործողության աջ օպերանդը պետք է լինի REAL, բայց ստացվել է {}։", binary._operation, *rightType));
            type = &realType;
            break;
        }
        case Operation::Eq:
        case Operation::Ne: {
            const auto leftScalar = requireScalar(*binary._left);
            const auto rightScalar = requireScalar(*binary._right);
            const auto typesKnown = leftType != nullptr && rightType != nullptr;
            const auto typesMatch = typesKnown && *leftType == *rightType;
            if( leftScalar && rightScalar && typesKnown && !typesMatch )
                report(binary, std::format("'{}' գործողության օպերանդները պետք է լինեն նույն տիպի։", binary._operation));
            type = &scalarType(ScalarType::Name::Bool);
            break;
        }
        case Operation::Gt:
        case Operation::Ge:
        case Operation::Lt:
        case Operation::Le: {
            const auto leftScalar = requireScalar(*binary._left);
            const auto rightScalar = requireScalar(*binary._right);
            const auto typesKnown = leftType != nullptr && rightType != nullptr;
            const auto bothReal = hasScalarType(leftType, ScalarType::Name::Real) && hasScalarType(rightType, ScalarType::Name::Real);
            const auto bothText = hasScalarType(leftType, ScalarType::Name::Text) && hasScalarType(rightType, ScalarType::Name::Text);
            const auto comparable = bothReal || bothText;
            if( leftScalar && rightScalar && typesKnown && !comparable )
                report(binary, std::format("'{}' գործողության օպերանդները պետք է լինեն երկու REAL կամ երկու TEXT արժեք։", binary._operation));
            type = &scalarType(ScalarType::Name::Bool);
            break;
        }
        case Operation::And:
        case Operation::Or:  {
            const auto& boolType = scalarType(ScalarType::Name::Bool);
            const auto leftScalar = requireScalar(*binary._left);
            const auto rightScalar = requireScalar(*binary._right);
            const auto wrongLeftType = leftScalar && typeMismatch(leftType, boolType);
            const auto wrongRightType = rightScalar && typeMismatch(rightType, boolType);
            if( wrongLeftType )
                report(*binary._left, std::format("'{}' գործողության ձախ օպերանդը պետք է լինի BOOL, բայց ստացվել է {}։", binary._operation, *leftType));
            if( wrongRightType )
                report(*binary._right, std::format("'{}' գործողության աջ օպերանդը պետք է լինի BOOL, բայց ստացվել է {}։", binary._operation, *rightType));
            type = &boolType;
            break;
        }
        case Operation::Conc: {
            const auto& textType = scalarType(ScalarType::Name::Text);
            const auto leftScalar = requireScalar(*binary._left);
            const auto rightScalar = requireScalar(*binary._right);
            const auto wrongLeftType = leftScalar && typeMismatch(leftType, textType);
            const auto wrongRightType = rightScalar && typeMismatch(rightType, textType);
            if( wrongLeftType )
                report(*binary._left, std::format("'{}' գործողության ձախ օպերանդը պետք է լինի TEXT, բայց ստացվել է {}։", binary._operation, *leftType));
            if( wrongRightType )
                report(*binary._right, std::format("'{}' գործողության աջ օպերանդը պետք է լինի TEXT, բայց ստացվել է {}։", binary._operation, *rightType));
            type = &textType;
            break;
        }
        case Operation::Index: {
            const auto array = leftType != nullptr && leftType->isArray();
            if( leftType != nullptr && !array )
                report(*binary._left, "Ինդեքսավորվող արտահայտությունը զանգված չէ։");
            validateIndex(*binary._right);
            if( array )
                type = &leftType->base();
            break;
        }
        case Operation::Not:
        case Operation::None:
            std::unreachable();
    }
    if( type != nullptr )
        _model.setType(binary.id(), *type);
}

void TypeCheckingPass::visit(Apply& apply)
{
    for( const auto& argument : apply._arguments )
        expressionType(*argument);

    const auto id = boundSubroutine(apply);
    if( !id.has_value() )
        return;

    const auto& signature = *_symbols.symbol(*id).subroutine;
    if( !signature.returnType )
        report(apply, std::format("'{}' ենթածրագիրը արժեք չի վերադարձնում։", apply._callee));
    else
        _model.setType(apply.id(), *signature.returnType);
    validateArguments(apply, apply._callee, apply._arguments, signature);
}

std::optional<SymbolId> TypeCheckingPass::boundVariable(const Variable& variable)
{
    const auto id = _model.symbol(variable.id());
    if( !id )
        return std::nullopt;

    const auto& symbol = _symbols.symbol(*id);
    if( symbol.kind != SymbolKind::Variable )
        return std::nullopt;

    _model.setType(variable.id(), *symbol.type);
    return id;
}

std::optional<SymbolId> TypeCheckingPass::boundSubroutine(const Node& node)
{
    const auto id = _model.symbol(node.id());
    if( !id )
        return std::nullopt;

    const auto& symbol = _symbols.symbol(*id);
    return symbol.kind == SymbolKind::Subroutine ? id : std::nullopt;
}

void TypeCheckingPass::validateArguments(const Node& node, std::string_view name, const std::vector<Expression::Ptr>& arguments, const SubroutineSignature& signature)
{
    const auto expectedCount = signature.parameters.size();
    const auto actualCount = arguments.size();
    if( actualCount != expectedCount )
        report(node, std::format("'{}' ենթածրագիրն ունի {} պարամետր, բայց ստացել է {} արգումենտ։", name, expectedCount, actualCount));

    const auto commonCount = std::min(actualCount, expectedCount);
    for( std::size_t index = 0; index < commonCount; ++index ) {
        const auto& argument = arguments[index];
        const auto argumentType = expressionType(*argument);
        if( argumentType == nullptr )
            continue;

        const auto parameterType = signature.parameters[index];
        if( parameterType == nullptr ) {
            requireScalar(*argument);
            continue;
        }

        if( parameterType->isArray() ) {
            if( !argumentType->isArray() ) {
                report(*argument, "Սպասվում է զանգվածային արգումենտ։");
                continue;
            }
        }
        else if( !requireScalar(*argument) )
            continue;

        const auto& expectedType = *parameterType;
        const auto wrongArgumentType = typeMismatch(argumentType, expectedType);
        if( wrongArgumentType )
            report(*argument, std::format("'{}' ենթածրագրի թիվ {} արգումենտը պետք է լինի {}, բայց ստացվել է {}։", name, index + 1, expectedType, *argumentType));
    }
}

const Type* TypeCheckingPass::expressionType(Expression& expression)
{
    if( const auto type = _model.type(expression.id()) )
        return type;

    visit(expression);
    return _model.type(expression.id());
}

bool TypeCheckingPass::isArrayExpression(const Expression& expression) const
{
    const auto type = _model.type(expression.id());
    return type != nullptr && type->isArray();
}

bool TypeCheckingPass::requireScalar(const Expression& expression)
{
    if( !isArrayExpression(expression) )
        return true;

    report(expression, "Զանգվածը չի կարող օգտագործվել որպես պարզ արժեք։");
    return false;
}

void TypeCheckingPass::validateIndex(Expression& index)
{
    const auto indexType = expressionType(index);
    const auto scalarIndex = requireScalar(index);
    const auto& realType = scalarType(ScalarType::Name::Real);
    const auto wrongIndexType = scalarIndex && typeMismatch(indexType, realType);
    if( wrongIndexType )
        report(index, std::format("Զանգվածի ինդեքսը պետք է լինի REAL, բայց ստացվել է {}։", *indexType));
}

bool TypeCheckingPass::definitelyReturns(const Sequence& sequence) const
{
    return std::ranges::any_of(sequence._items,
        [this](const auto& statement) { return definitelyReturns(*statement); });
}

bool TypeCheckingPass::definitelyReturns(const Statement& statement) const
{
    if( statement.kind == NodeKind::Return )
        return true;
    if( statement.kind != NodeKind::If )
        return false;

    const auto& conditional = static_cast<const If&>(statement);
    if( !conditional._alternative || !definitelyReturns(*conditional._alternative) )
        return false;

    return std::ranges::all_of(conditional._branches,
        [this](const auto& branch) { return definitelyReturns(*branch->_body); });
}

void TypeCheckingPass::report(const Node& node, std::string_view message)
{
    _diagnostics.advance();
    _diagnostics.mark(node.line, message);
}

} // namespace

SemanticAnalyzer::SemanticAnalyzer(SymbolTable& symbols, SemanticModel& model, Diagnostics& diagnostics)
    : _symbols{symbols}
    , _model{model}
    , _diagnostics{diagnostics}
{
}

bool SemanticAnalyzer::analyze(Program& program)
{
    NameResolutionPass nameResolution{_symbols, _model, _diagnostics};
    nameResolution.visit(program);
    TypeCheckingPass typeChecking{_symbols, _model, _diagnostics};
    typeChecking.visit(program);
    return _diagnostics.count() == 0;
}

} // namespace avium
