#include "semanticpasses.hxx"

#include "formatters.hxx"

#include <algorithm>
#include <format>
#include <vector>

namespace avium {

namespace {

bool typeMismatch(const Type* actual, const Type& expected)
{
    return actual != nullptr && *actual != expected;
}

bool hasScalarType(const Type* type, ScalarType::Name name)
{
    return type != nullptr && !type->isArray() && type->base()._name == name;
}

class TypeCheckingPass {
public:
    explicit TypeCheckingPass(SemanticContext& context)
        : _context{context}
    {
    }

    void check(const Program& program);

private:
    void check(const Subroutine& subroutine);
    void check(const Sequence& sequence);
    void check(const Statement& statement);
    void check(const Dim& node);
    void check(const Let& node);
    void check(const If& node);
    void check(const IfBranch& node);
    void check(const While& node);
    void check(const For& node);
    void check(const Call& node);
    void check(const Return& node);

    const Type* typeOf(const Expression& expression);
    const Type* typeOf(const Boolean& node);
    const Type* typeOf(const Number& node);
    const Type* typeOf(const Text& node);
    const Type* typeOf(const Variable& node);
    const Type* typeOf(const Unary& node);
    const Type* typeOf(const Binary& node);
    const Type* typeOf(const Apply& node);
    const Type* cacheType(const Expression& expression, const Type& type);

    const Type& typeOfArithmetic(const Binary& binary);
    const Type& typeOfEquality(const Binary& binary);
    const Type& typeOfComparison(const Binary& binary);
    const Type& typeOfLogical(const Binary& binary);
    const Type& typeOfConcatenation(const Binary& binary);
    const Type* typeOfIndex(const Binary& binary);

    const VariableSymbol* boundVariable(const Variable& variable) const;
    const SubroutineSymbol* boundSubroutine(const Node& node) const;
    std::vector<const Type*> argumentTypes(const std::vector<Expression::Ptr>& arguments);
    void validateArguments(const Node& node, const SubroutineSymbol& subroutine, const std::vector<Expression::Ptr>& arguments, const std::vector<const Type*>& argumentTypes);
    const Type* requireScalar(const Expression& expression);
    void validateScalarOperands(const Binary& binary, const Type& expected);
    void validateIndex(const Expression& index);
    bool definitelyReturns(const Sequence& sequence) const;
    bool definitelyReturns(const Statement& statement) const;

    SemanticContext& _context;
    const ScalarType* _currentReturnType{nullptr};
};

void TypeCheckingPass::check(const Program& program)
{
    for( const auto& subroutine : program._subroutines )
        check(*subroutine);
}

void TypeCheckingPass::check(const Subroutine& subroutine)
{
    _currentReturnType = subroutine._returnType.get();
    check(*subroutine._body);

    if( subroutine._name != "Main" && _currentReturnType != nullptr && !definitelyReturns(*subroutine._body) )
        _context.report(subroutine, std::format("'{}' ֆունկցիայի ոչ բոլոր կատարման ուղիներն են արժեք վերադարձնում։", subroutine._name));

    _currentReturnType = nullptr;
}

void TypeCheckingPass::check(const Sequence& sequence)
{
    for( const auto& statement : sequence._items )
        check(*statement);
}

void TypeCheckingPass::check(const Statement& statement)
{
    switch( statement.kind ) {
        case NodeKind::Dim:
            return check(static_cast<const Dim&>(statement));
        case NodeKind::Let:
            return check(static_cast<const Let&>(statement));
        case NodeKind::If:
            return check(static_cast<const If&>(statement));
        case NodeKind::While:
            return check(static_cast<const While&>(statement));
        case NodeKind::For:
            return check(static_cast<const For&>(statement));
        case NodeKind::Call:
            return check(static_cast<const Call&>(statement));
        case NodeKind::Return:
            return check(static_cast<const Return&>(statement));
        default:
            std::unreachable();
    }
}

void TypeCheckingPass::check(const Dim& dim)
{
    if( !dim._type->isArray() )
        return;

    const auto& array = static_cast<const ArrayType&>(*dim._type);
    if( !array._size ) {
        _context.report(dim, "Զանգվածի չափը նշված չէ։");
        return;
    }

    const auto* actual = requireScalar(*array._size);
    const auto& expected = scalarType(ScalarType::Name::Real);
    if( typeMismatch(actual, expected) )
        _context.report(*array._size, std::format("Զանգվածի չափը պետք է լինի REAL, բայց ստացվել է {}։", *actual));
}

void TypeCheckingPass::check(const Let& let)
{
    const auto* target = boundVariable(*let._variable);
    if( target != nullptr )
        cacheType(*let._variable, *target->type);
    if( let._index )
        validateIndex(*let._index);
    const auto* valueType = typeOf(*let._value);

    if( target == nullptr )
        return;

    const auto& declaredType = *target->type;
    const auto arrayTarget = declaredType.isArray();
    const Type& targetType = let._index && arrayTarget ? declaredType.base() : declaredType;
    bool validTarget = true;
    if( let._index && !arrayTarget ) {
        _context.report(*let._variable, std::format("'{}' փոփոխականը զանգված չէ։", let._variable->_name));
        validTarget = false;
    }
    else if( !let._index && arrayTarget ) {
        _context.report(*let._variable, std::format("'{}' զանգվածին ամբողջությամբ արժեք վերագրել չի կարելի։", let._variable->_name));
        validTarget = false;
    }

    if( valueType != nullptr && valueType->isArray() ) {
        _context.report(*let._value, "Զանգվածը չի կարող վերագրվել որպես պարզ արժեք։");
        return;
    }

    if( validTarget && typeMismatch(valueType, targetType) )
        _context.report(*let._value, std::format("'{}' փոփոխականին պետք է վերագրվի {}, բայց ստացվել է {}։", let._variable->_name, targetType, *valueType));
}

void TypeCheckingPass::check(const If& conditional)
{
    for( const auto& branch : conditional._branches )
        check(*branch);
    if( conditional._alternative )
        check(*conditional._alternative);
}

void TypeCheckingPass::check(const IfBranch& branch)
{
    const auto* actual = requireScalar(*branch._condition);
    const auto& expected = scalarType(ScalarType::Name::Bool);
    if( typeMismatch(actual, expected) )
        _context.report(*branch._condition, std::format("Ճյուղավորման պայմանը պետք է լինի BOOL, բայց ստացվել է {}։", *actual));
    check(*branch._body);
}

void TypeCheckingPass::check(const While& loop)
{
    const auto* actual = requireScalar(*loop._condition);
    const auto& expected = scalarType(ScalarType::Name::Bool);
    if( typeMismatch(actual, expected) )
        _context.report(*loop._condition, std::format("WHILE-ի պայմանը պետք է լինի BOOL, բայց ստացվել է {}։", *actual));
    check(*loop._body);
}

void TypeCheckingPass::check(const For& loop)
{
    typeOf(*loop._parameter);

    const auto& expected = scalarType(ScalarType::Name::Real);
    if( const auto* actual = requireScalar(*loop._begin); typeMismatch(actual, expected) )
        _context.report(*loop._begin, std::format("FOR-ի սկզբնական արժեքը պետք է լինի REAL, բայց ստացվել է {}։", *actual));

    if( const auto* actual = requireScalar(*loop._end); typeMismatch(actual, expected) )
        _context.report(*loop._end, std::format("FOR-ի վերջնական արժեքը պետք է լինի REAL, բայց ստացվել է {}։", *actual));

    if( const auto* actual = requireScalar(*loop._step); typeMismatch(actual, expected) )
        _context.report(*loop._step, std::format("FOR-ի քայլը պետք է լինի REAL, բայց ստացվել է {}։", *actual));
    if( loop._step->_value == 0.0 )
        _context.report(*loop._step, "FOR-ի քայլը չի կարող լինել 0։");

    check(*loop._body);
}

void TypeCheckingPass::check(const Call& call)
{
    const auto types = argumentTypes(call._arguments);
    const auto* subroutine = boundSubroutine(call);
    if( subroutine == nullptr )
        return;

    if( subroutine->signature.returnType )
        _context.report(call, "CALL-ով կարելի է կանչել միայն պրոցեդուրա։");
    validateArguments(call, *subroutine, call._arguments, types);
}

void TypeCheckingPass::check(const Return& statement)
{
    const auto* valueType = typeOf(*statement._value);
    const auto* scalarValue = requireScalar(*statement._value);

    if( _currentReturnType == nullptr ) {
        _context.report(statement, "RETURN հրամանը թույլատրելի է միայն ֆունկցիայում։");
        return;
    }

    if( scalarValue != nullptr && typeMismatch(valueType, *_currentReturnType) )
        _context.report(*statement._value, std::format("Ֆունկցիայից պետք է վերադարձվի {}, բայց ստացվել է {}։", static_cast<const Type&>(*_currentReturnType), *valueType));
}

const Type* TypeCheckingPass::typeOf(const Expression& expression)
{
    if( const auto* cached = _context.model.type(expression.id()) )
        return cached;

    switch( expression.kind ) {
        case NodeKind::Boolean:
            return typeOf(static_cast<const Boolean&>(expression));
        case NodeKind::Number:
            return typeOf(static_cast<const Number&>(expression));
        case NodeKind::Text:
            return typeOf(static_cast<const Text&>(expression));
        case NodeKind::Variable:
            return typeOf(static_cast<const Variable&>(expression));
        case NodeKind::Unary:
            return typeOf(static_cast<const Unary&>(expression));
        case NodeKind::Binary:
            return typeOf(static_cast<const Binary&>(expression));
        case NodeKind::Apply:
            return typeOf(static_cast<const Apply&>(expression));
        default:
            std::unreachable();
    }
}

const Type* TypeCheckingPass::typeOf(const Boolean& boolean)
{
    return cacheType(boolean, scalarType(ScalarType::Name::Bool));
}

const Type* TypeCheckingPass::typeOf(const Number& number)
{
    return cacheType(number, scalarType(ScalarType::Name::Real));
}

const Type* TypeCheckingPass::typeOf(const Text& text)
{
    return cacheType(text, scalarType(ScalarType::Name::Text));
}

const Type* TypeCheckingPass::typeOf(const Variable& variable)
{
    const auto* symbol = boundVariable(variable);
    return symbol == nullptr ? nullptr : cacheType(variable, *symbol->type);
}

const Type* TypeCheckingPass::typeOf(const Unary& unary)
{
    const auto* operandType = requireScalar(*unary._operand);

    switch( unary._operation ) {
        case Operation::Not: {
            const auto& result = scalarType(ScalarType::Name::Bool);
            if( typeMismatch(operandType, result) )
                _context.report(unary, std::format("'{}' գործողության օպերանդը պետք է լինի BOOL, բայց ստացվել է {}։", unary._operation, *operandType));
            return cacheType(unary, result);
        }
        case Operation::Add:
        case Operation::Sub: {
            const auto& result = scalarType(ScalarType::Name::Real);
            if( typeMismatch(operandType, result) )
                _context.report(unary, std::format("Ունար '{}' գործողության օպերանդը պետք է լինի REAL, բայց ստացվել է {}։", unary._operation, *operandType));
            return cacheType(unary, result);
        }
        default:
            std::unreachable();
    }
}

const Type* TypeCheckingPass::typeOf(const Binary& binary)
{
    switch( binary._operation ) {
        case Operation::Add:
        case Operation::Sub:
        case Operation::Mul:
        case Operation::Div:
        case Operation::Quot:
        case Operation::Mod:
        case Operation::Pow:
            return cacheType(binary, typeOfArithmetic(binary));
        case Operation::Eq:
        case Operation::Ne:
            return cacheType(binary, typeOfEquality(binary));
        case Operation::Gt:
        case Operation::Ge:
        case Operation::Lt:
        case Operation::Le:
            return cacheType(binary, typeOfComparison(binary));
        case Operation::And:
        case Operation::Or:
            return cacheType(binary, typeOfLogical(binary));
        case Operation::Conc:
            return cacheType(binary, typeOfConcatenation(binary));
        case Operation::Index:
            if( const auto* result = typeOfIndex(binary) )
                return cacheType(binary, *result);
            return nullptr;
        case Operation::Not:
        case Operation::None:
            std::unreachable();
    }
}

const Type& TypeCheckingPass::typeOfArithmetic(const Binary& binary)
{
    const auto& result = scalarType(ScalarType::Name::Real);
    validateScalarOperands(binary, result);
    return result;
}

const Type& TypeCheckingPass::typeOfEquality(const Binary& binary)
{
    const auto* leftType = typeOf(*binary._left);
    const auto* rightType = typeOf(*binary._right);
    const auto* leftScalar = requireScalar(*binary._left);
    const auto* rightScalar = requireScalar(*binary._right);
    const auto typesKnown = leftType != nullptr && rightType != nullptr;
    const auto typesMatch = typesKnown && *leftType == *rightType;
    if( leftScalar != nullptr && rightScalar != nullptr && typesKnown && !typesMatch )
        _context.report(binary, std::format("'{}' գործողության օպերանդները պետք է լինեն նույն տիպի։", binary._operation));
    return scalarType(ScalarType::Name::Bool);
}

const Type& TypeCheckingPass::typeOfComparison(const Binary& binary)
{
    const auto* leftType = typeOf(*binary._left);
    const auto* rightType = typeOf(*binary._right);
    const auto* leftScalar = requireScalar(*binary._left);
    const auto* rightScalar = requireScalar(*binary._right);
    const auto typesKnown = leftType != nullptr && rightType != nullptr;
    const auto bothReal = hasScalarType(leftType, ScalarType::Name::Real) && hasScalarType(rightType, ScalarType::Name::Real);
    const auto bothText = hasScalarType(leftType, ScalarType::Name::Text) && hasScalarType(rightType, ScalarType::Name::Text);
    if( leftScalar != nullptr && rightScalar != nullptr && typesKnown && !bothReal && !bothText )
        _context.report(binary, std::format("'{}' գործողության օպերանդները պետք է լինեն երկու REAL կամ երկու TEXT արժեք։", binary._operation));
    return scalarType(ScalarType::Name::Bool);
}

const Type& TypeCheckingPass::typeOfLogical(const Binary& binary)
{
    const auto& result = scalarType(ScalarType::Name::Bool);
    validateScalarOperands(binary, result);
    return result;
}

const Type& TypeCheckingPass::typeOfConcatenation(const Binary& binary)
{
    const auto& result = scalarType(ScalarType::Name::Text);
    validateScalarOperands(binary, result);
    return result;
}

const Type* TypeCheckingPass::typeOfIndex(const Binary& binary)
{
    const auto* leftType = typeOf(*binary._left);
    const auto array = leftType != nullptr && leftType->isArray();
    if( leftType != nullptr && !array )
        _context.report(*binary._left, "Ինդեքսավորվող արտահայտությունը զանգված չէ։");
    validateIndex(*binary._right);
    return array ? &leftType->base() : nullptr;
}

const Type* TypeCheckingPass::typeOf(const Apply& apply)
{
    const auto types = argumentTypes(apply._arguments);
    const auto* subroutine = boundSubroutine(apply);
    if( subroutine == nullptr )
        return nullptr;

    const Type* result = nullptr;
    if( !subroutine->signature.returnType )
        _context.report(apply, std::format("'{}' ենթածրագիրը արժեք չի վերադարձնում։", apply._callee));
    else
        result = cacheType(apply, *subroutine->signature.returnType);

    validateArguments(apply, *subroutine, apply._arguments, types);
    return result;
}

const Type* TypeCheckingPass::cacheType(const Expression& expression, const Type& type)
{
    _context.model.setType(expression.id(), type);
    return &type;
}

const VariableSymbol* TypeCheckingPass::boundVariable(const Variable& variable) const
{
    const auto id = _context.model.symbol(variable.id());
    return id ? _context.symbols.variable(*id) : nullptr;
}

const SubroutineSymbol* TypeCheckingPass::boundSubroutine(const Node& node) const
{
    const auto id = _context.model.symbol(node.id());
    return id ? _context.symbols.subroutine(*id) : nullptr;
}

std::vector<const Type*> TypeCheckingPass::argumentTypes(const std::vector<Expression::Ptr>& arguments)
{
    std::vector<const Type*> types;
    types.reserve(arguments.size());
    for( const auto& argument : arguments )
        types.push_back(typeOf(*argument));
    return types;
}

void TypeCheckingPass::validateArguments(const Node& node, const SubroutineSymbol& subroutine, const std::vector<Expression::Ptr>& arguments, const std::vector<const Type*>& argumentTypes)
{
    const auto& name = subroutine.name;
    const auto& signature = subroutine.signature;
    const auto expectedCount = signature.parameters.size();
    const auto actualCount = arguments.size();
    if( actualCount != expectedCount )
        _context.report(node, std::format("'{}' ենթածրագիրն ունի {} պարամետր, բայց ստացել է {} արգումենտ։", name, expectedCount, actualCount));

    const auto commonCount = std::min(actualCount, expectedCount);
    for( std::size_t index = 0; index < commonCount; ++index ) {
        const auto& argument = arguments[index];
        const auto* argumentType = argumentTypes[index];
        if( argumentType == nullptr )
            continue;

        if( name == "LEN" ) {
            const auto validArgument = argumentType->isArray() || argumentType->base()._name == ScalarType::Name::Text;
            if( !validArgument )
                _context.report(*argument, "LEN-ի արգումենտը պետք է լինի TEXT կամ զանգված։");
            continue;
        }

        if( name == "STR" ) {
            const auto scalarArgument = !argumentType->isArray();
            const auto scalarName = argumentType->base()._name;
            const auto validArgument = scalarArgument && (scalarName == ScalarType::Name::Real || scalarName == ScalarType::Name::Bool);
            if( !validArgument )
                _context.report(*argument, "STR-ի արգումենտը պետք է լինի REAL կամ BOOL։");
            continue;
        }

        const auto* parameterType = signature.parameters[index];
        if( parameterType == nullptr ) {
            requireScalar(*argument);
            continue;
        }

        if( parameterType->isArray() ) {
            if( !argumentType->isArray() ) {
                _context.report(*argument, "Սպասվում է զանգվածային արգումենտ։");
                continue;
            }
        }
        else if( requireScalar(*argument) == nullptr )
            continue;

        if( typeMismatch(argumentType, *parameterType) )
            _context.report(*argument, std::format("'{}' ենթածրագրի թիվ {} արգումենտը պետք է լինի {}, բայց ստացվել է {}։", name, index + 1, *parameterType, *argumentType));
    }
}

const Type* TypeCheckingPass::requireScalar(const Expression& expression)
{
    const auto* type = typeOf(expression);
    if( type == nullptr || !type->isArray() )
        return type;

    _context.report(expression, "Զանգվածը չի կարող օգտագործվել որպես պարզ արժեք։");
    return nullptr;
}

void TypeCheckingPass::validateScalarOperands(const Binary& binary, const Type& expected)
{
    const auto* left = requireScalar(*binary._left);
    const auto* right = requireScalar(*binary._right);
    if( typeMismatch(left, expected) )
        _context.report(*binary._left, std::format("'{}' գործողության ձախ օպերանդը պետք է լինի {}, բայց ստացվել է {}։", binary._operation, expected, *left));
    if( typeMismatch(right, expected) )
        _context.report(*binary._right, std::format("'{}' գործողության աջ օպերանդը պետք է լինի {}, բայց ստացվել է {}։", binary._operation, expected, *right));
}

void TypeCheckingPass::validateIndex(const Expression& index)
{
    const auto* actual = requireScalar(index);
    const auto& expected = scalarType(ScalarType::Name::Real);
    if( typeMismatch(actual, expected) )
        _context.report(index, std::format("Զանգվածի ինդեքսը պետք է լինի REAL, բայց ստացվել է {}։", *actual));
}

bool TypeCheckingPass::definitelyReturns(const Sequence& sequence) const
{
    return std::ranges::any_of(sequence._items,
        [this](const auto& statement) {
            return definitelyReturns(*statement);
        });
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
        [this](const auto& branch) {
            return definitelyReturns(*branch->_body);
        });
}

} // namespace

void checkTypes(const Program& program, SemanticContext& context)
{
    TypeCheckingPass{context}.check(program);
}

} // namespace avium
