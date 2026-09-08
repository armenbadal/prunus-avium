#include "semantic.hxx"
#include "formatters.hxx"

#include <algorithm>
#include <cmath>
#include <format>
#include <utility>

namespace avium {

namespace {

bool typeMismatch(std::optional<TypeName> actual, TypeName expected)
{
    return actual.has_value() && *actual != expected;
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
    static const std::vector<SubroutineSignature> signatures{
        {"Print", {{std::nullopt, false}}, std::nullopt, true},
        {"Input", {}, TypeName::Text, true},
        {"NUM", {{TypeName::Text, false}}, TypeName::Real, true},
        {"SQR", {{TypeName::Real, false}}, TypeName::Real, true},
    };
    return signatures;
}

} // namespace

void SemanticModel::bind(NodeId node, SymbolId symbol)
{
    _symbols.insert_or_assign(node, symbol);
}

void SemanticModel::bindReturnValue(NodeId subroutine, SymbolId symbol)
{
    _returnValues.insert_or_assign(subroutine, symbol);
}

void SemanticModel::setType(NodeId node, TypeName type)
{
    _types.insert_or_assign(node, type);
}

std::optional<SymbolId> SemanticModel::symbol(NodeId node) const
{
    if( const auto entry = _symbols.find(node); entry != _symbols.end() )
        return entry->second;
    return std::nullopt;
}

std::optional<SymbolId> SemanticModel::returnValue(NodeId subroutine) const
{
    if( const auto entry = _returnValues.find(subroutine); entry != _returnValues.end() )
        return entry->second;
    return std::nullopt;
}

std::optional<TypeName> SemanticModel::type(NodeId node) const
{
    if( const auto entry = _types.find(node); entry != _types.end() )
        return entry->second;
    return std::nullopt;
}

SemanticAnalyzer::SemanticAnalyzer(SymbolTable& symbols, SemanticModel& model, Diagnostics& diagnostics)
    : _symbols{symbols}, _model{model}, _diagnostics{diagnostics}
{
}

bool SemanticAnalyzer::analyze(Program& program)
{
    declareBuiltins();
    declareSubroutines(program);
    visit(program);
    return _diagnostics.count() == 0;
}

void SemanticAnalyzer::visit(Program& program)
{
    Subroutine* main = nullptr;
    for( const auto& subroutine : program._subroutines ) {
        if( subroutine->_name != "Main" )
            continue;

        if( main == nullptr )
            main = subroutine.get();
    }

    if( main == nullptr ) {
        report(program, "Ծրագիրը պետք է ունենա ճիշտ մեկ 'Main' ենթածրագիր։");
    }
    else {
        if( !main->_parameters.empty() )
            report(*main, "'Main' ենթածրագիրը պարամետրեր չի կարող ունենալ։");

        if( main->_returnType.has_value() )
            report(*main, "'Main' ենթածրագիրը արժեք չի կարող վերադարձնել։");
    }

    for( const auto& subroutine : program._subroutines )
        visit(*subroutine);
}

void SemanticAnalyzer::visit(Subroutine& subroutine)
{
    analyzeSubroutine(subroutine);
}

void SemanticAnalyzer::visit(Sequence& sequence)
{
    for( const auto& statement : sequence._items )
        visit(*statement);
}

void SemanticAnalyzer::visit(Dim& dim)
{
    if( !dim._isArray )
        return;

    if( !dim._size ) {
        report(dim, "Զանգվածի չափը նշված չէ։");
        return;
    }

    const auto sizeType = expressionType(dim._size);
    const auto scalarSize = requireScalar(*dim._size);
    const auto wrongSizeType = scalarSize && typeMismatch(sizeType, TypeName::Real);
    if( wrongSizeType )
        report(*dim._size, std::format("Զանգվածի չափը պետք է լինի REAL, բայց ստացվել է {}։", *sizeType));

    const auto size = constantReal(*dim._size);
    if( size.has_value() ) {
        const auto positive = *size > 0.0;
        const auto integral = std::trunc(*size) == *size;
        const auto finite = std::isfinite(*size);
        if( !positive || !integral || !finite )
            report(*dim._size, "Զանգվածի հաստատուն չափը պետք է լինի դրական ամբողջ թիվ։");
    }
}

void SemanticAnalyzer::visit(Let& let)
{
    const auto target = resolveVariable(*let._variable);
    if( let._index )
        validateIndex(let._index);
    const auto valueType = expressionType(let._value);

    if( !target.has_value() )
        return;

    const auto& symbol = _symbols.symbol(*target);
    const auto targetType = *symbol.type;
    bool validTarget = true;
    if( let._index && !symbol.isArray ) {
        report(*let._variable, std::format("'{}' փոփոխականը զանգված չէ։", let._variable->_name));
        validTarget = false;
    }
    else if( !let._index && symbol.isArray ) {
        report(*let._variable, std::format("'{}' զանգվածին ամբողջությամբ արժեք վերագրել չի կարելի։", let._variable->_name));
        validTarget = false;
    }

    if( isArrayExpression(*let._value) ) {
        report(*let._value, "Զանգվածը չի կարող վերագրվել որպես պարզ արժեք։");
        return;
    }

    const auto wrongValueType = validTarget && valueType.has_value() && *valueType != targetType;
    if( wrongValueType )
        report(*let._value, std::format("'{}' փոփոխականին պետք է վերագրվի {}, բայց ստացվել է {}։", let._variable->_name, targetType, *valueType));
}

void SemanticAnalyzer::visit(If& conditional)
{
    for( const auto& branch : conditional._branches )
        visit(*branch);
    if( conditional._alternative )
        visit(*conditional._alternative);
}

void SemanticAnalyzer::visit(IfBranch& branch)
{
    const auto conditionType = expressionType(branch._condition);
    const auto scalarCondition = requireScalar(*branch._condition);
    const auto wrongConditionType = scalarCondition && typeMismatch(conditionType, TypeName::Bool);
    if( wrongConditionType )
        report(*branch._condition, std::format("Պայմանական ճյուղի պայմանը պետք է լինի BOOL, բայց ստացվել է {}։", *conditionType));
    visit(*branch._body);
}

void SemanticAnalyzer::visit(While& loop)
{
    const auto conditionType = expressionType(loop._condition);
    const auto scalarCondition = requireScalar(*loop._condition);
    const auto wrongConditionType = scalarCondition && typeMismatch(conditionType, TypeName::Bool);
    if( wrongConditionType )
        report(*loop._condition, std::format("WHILE-ի պայմանը պետք է լինի BOOL, բայց ստացվել է {}։", *conditionType));
    visit(*loop._body);
}

void SemanticAnalyzer::visit(For& loop)
{
    visit(*loop._parameter);

    const auto beginType = expressionType(loop._begin);
    const auto scalarBegin = requireScalar(*loop._begin);
    const auto wrongBeginType = scalarBegin && typeMismatch(beginType, TypeName::Real);
    if( wrongBeginType )
        report(*loop._begin, std::format("FOR-ի սկզբնական արժեքը պետք է լինի REAL, բայց ստացվել է {}։", *beginType));

    const auto endType = expressionType(loop._end);
    const auto scalarEnd = requireScalar(*loop._end);
    const auto wrongEndType = scalarEnd && typeMismatch(endType, TypeName::Real);
    if( wrongEndType )
        report(*loop._end, std::format("FOR-ի վերջնական արժեքը պետք է լինի REAL, բայց ստացվել է {}։", *endType));

    const auto stepType = expressionType(loop._step);
    const auto scalarStep = requireScalar(*loop._step);
    const auto wrongStepType = scalarStep && typeMismatch(stepType, TypeName::Real);
    if( wrongStepType )
        report(*loop._step, std::format("FOR-ի քայլը պետք է լինի REAL, բայց ստացվել է {}։", *stepType));
    if( loop._step->_value == 0.0 )
        report(*loop._step, "FOR-ի քայլը չի կարող լինել 0։");

    visit(*loop._body);
}

void SemanticAnalyzer::visit(Call& call)
{
    for( const auto& argument : call._arguments )
        expressionType(argument);

    const auto id = resolveSubroutine(call, call._callee);
    if( !id.has_value() )
        return;

    const auto& signature = *_symbols.symbol(*id).subroutine;
    if( signature.returnType.has_value() )
        report(call, "CALL-ով կարելի է կանչել միայն պրոցեդուրա։");
    validateArguments(call, call._callee, call._arguments, signature);
}

void SemanticAnalyzer::visit(Boolean& boolean)
{
    _model.setType(boolean.id(), TypeName::Bool);
}

void SemanticAnalyzer::visit(Number& number)
{
    _model.setType(number.id(), TypeName::Real);
}

void SemanticAnalyzer::visit(Text& text)
{
    _model.setType(text.id(), TypeName::Text);
}

void SemanticAnalyzer::visit(Variable& variable)
{
    resolveVariable(variable);
}

void SemanticAnalyzer::visit(Unary& unary)
{
    const auto operandType = expressionType(unary._operand);
    const auto scalar = requireScalar(*unary._operand);

    switch( unary._operation ) {
        case Operation::Not: {
            const auto wrongOperandType = scalar && typeMismatch(operandType, TypeName::Bool);
            if( wrongOperandType )
                report(unary, std::format("'{}' գործողության օպերանդը պետք է լինի BOOL, բայց ստացվել է {}։", unary._operation, *operandType));
            _model.setType(unary.id(), TypeName::Bool);
            break;
        }
        case Operation::Add:
        case Operation::Sub: {
            const auto wrongOperandType = scalar && typeMismatch(operandType, TypeName::Real);
            if( wrongOperandType )
                report(unary, std::format("Ունար '{}' գործողության օպերանդը պետք է լինի REAL, բայց ստացվել է {}։", unary._operation, *operandType));
            _model.setType(unary.id(), TypeName::Real);
            break;
        }
        default:
            std::unreachable();
    }
}

void SemanticAnalyzer::visit(Binary& binary)
{
    const auto leftType = expressionType(binary._left);
    const auto rightType = expressionType(binary._right);

    std::optional<TypeName> type;
    switch( binary._operation ) {
        case Operation::Add:
        case Operation::Sub:
        case Operation::Mul:
        case Operation::Div:
        case Operation::Quot:
        case Operation::Mod:
        case Operation::Pow:  {
            const auto leftScalar = requireScalar(*binary._left);
            const auto rightScalar = requireScalar(*binary._right);
            const auto wrongLeftType = leftScalar && typeMismatch(leftType, TypeName::Real);
            const auto wrongRightType = rightScalar && typeMismatch(rightType, TypeName::Real);
            if( wrongLeftType )
                report(*binary._left, std::format("'{}' գործողության ձախ օպերանդը պետք է լինի REAL, բայց ստացվել է {}։", binary._operation, *leftType));
            if( wrongRightType )
                report(*binary._right, std::format("'{}' գործողության աջ օպերանդը պետք է լինի REAL, բայց ստացվել է {}։", binary._operation, *rightType));
            type = TypeName::Real;
            break;
        }
        case Operation::Eq:
        case Operation::Ne: {
            const auto leftScalar = requireScalar(*binary._left);
            const auto rightScalar = requireScalar(*binary._right);
            const auto typesKnown = leftType.has_value() && rightType.has_value();
            const auto typesMatch = leftType == rightType;
            if( leftScalar && rightScalar && typesKnown && !typesMatch )
                report(binary, std::format("'{}' գործողության օպերանդները պետք է լինեն նույն տիպի։", binary._operation));
            type = TypeName::Bool;
            break;
        }
        case Operation::Gt:
        case Operation::Ge:
        case Operation::Lt:
        case Operation::Le: {
            const auto leftScalar = requireScalar(*binary._left);
            const auto rightScalar = requireScalar(*binary._right);
            const auto typesKnown = leftType.has_value() && rightType.has_value();
            const auto bothReal = leftType == TypeName::Real && rightType == TypeName::Real;
            const auto bothText = leftType == TypeName::Text && rightType == TypeName::Text;
            const auto comparable = bothReal || bothText;
            if( leftScalar && rightScalar && typesKnown && !comparable )
                report(binary, std::format("'{}' գործողության օպերանդները պետք է լինեն երկու REAL կամ երկու TEXT արժեք։", binary._operation));
            type = TypeName::Bool;
            break;
        }
        case Operation::And:
        case Operation::Or:  {
            const auto leftScalar = requireScalar(*binary._left);
            const auto rightScalar = requireScalar(*binary._right);
            const auto wrongLeftType = leftScalar && typeMismatch(leftType, TypeName::Bool);
            const auto wrongRightType = rightScalar && typeMismatch(rightType, TypeName::Bool);
            if( wrongLeftType )
                report(*binary._left, std::format("'{}' գործողության ձախ օպերանդը պետք է լինի BOOL, բայց ստացվել է {}։", binary._operation, *leftType));
            if( wrongRightType )
                report(*binary._right, std::format("'{}' գործողության աջ օպերանդը պետք է լինի BOOL, բայց ստացվել է {}։", binary._operation, *rightType));
            type = TypeName::Bool;
            break;
        }
        case Operation::Conc: {
            const auto leftScalar = requireScalar(*binary._left);
            const auto rightScalar = requireScalar(*binary._right);
            const auto wrongLeftType = leftScalar && typeMismatch(leftType, TypeName::Text);
            const auto wrongRightType = rightScalar && typeMismatch(rightType, TypeName::Text);
            if( wrongLeftType )
                report(*binary._left, std::format("'{}' գործողության ձախ օպերանդը պետք է լինի TEXT, բայց ստացվել է {}։", binary._operation, *leftType));
            if( wrongRightType )
                report(*binary._right, std::format("'{}' գործողության աջ օպերանդը պետք է լինի TEXT, բայց ստացվել է {}։", binary._operation, *rightType));
            type = TypeName::Text;
            break;
        }
        case Operation::Index:
            if( leftType.has_value() && !isArrayExpression(*binary._left) )
                report(*binary._left, "Ինդեքսավորվող արտահայտությունը զանգված չէ։");
            validateIndex(binary._right);
            type = leftType;
            break;
        case Operation::Not:
        case Operation::None:
            std::unreachable();
    }
    if( type.has_value() )
        _model.setType(binary.id(), *type);
}

void SemanticAnalyzer::visit(Apply& apply)
{
    for( const auto& argument : apply._arguments )
        expressionType(argument);

    const auto id = resolveSubroutine(apply, apply._callee);
    if( !id.has_value() )
        return;

    const auto& signature = *_symbols.symbol(*id).subroutine;
    if( !signature.returnType.has_value() )
        report(apply, std::format("'{}' ենթածրագիրը արժեք չի վերադարձնում։", apply._callee));
    else
        _model.setType(apply.id(), *signature.returnType);
    validateArguments(apply, apply._callee, apply._arguments, signature);
}

void SemanticAnalyzer::declareBuiltins()
{
    for( const auto& signature : builtinSignatures() )
        _symbols.declareSubroutine(signature);
}

void SemanticAnalyzer::declareSubroutines(const Program& program)
{
    for( const auto& subroutine : program._subroutines ) {
        std::vector<ParameterInfo> parameters;
        parameters.reserve(subroutine->_parameters.size());
        for( const auto& parameter : subroutine->_parameters )
            parameters.push_back(parameterInfo(*parameter));

        const auto existing = _symbols.lookupSubroutine(subroutine->_name);
        if( existing.has_value() ) {
            const auto& symbol = _symbols.symbol(*existing);
            if( symbol.subroutine->builtin )
                report(*subroutine, std::format("'{}' անունը պատկանում է ներդրված ենթածրագրի։", subroutine->_name));
            else
                report(*subroutine, std::format("'{}' ենթածրագիրն արդեն սահմանված է։", subroutine->_name));
            continue;
        }

        const auto& name = subroutine->_name;
        const auto returnType = subroutine->_returnType;
        const auto builtin = false;
        SubroutineSignature signature{name, std::move(parameters), returnType, builtin};
        const auto id = _symbols.declareSubroutine(std::move(signature));
        _model.bind(subroutine->id(), id);
    }
}

void SemanticAnalyzer::analyzeSubroutine(Subroutine& subroutine)
{
    _symbols.openScope();
    declareParameters(subroutine);
    declareReturnValue(subroutine);
    declareLocals(*subroutine._body);
    visit(*subroutine._body);
    _symbols.closeScope();
}

void SemanticAnalyzer::declareParameters(const Subroutine& subroutine)
{
    for( const auto& parameter : subroutine._parameters ) {
        const auto& name = parameter->_name;
        const auto type = parameter->_type;
        const auto isArray = parameter->_isArray;
        const auto storage = VariableStorage::Parameter;
        const auto id = _symbols.declareVariable(name, type, isArray, storage);
        if( id == UnknownSymbol )
            report(*parameter, std::format("'{}' անունն արդեն սահմանված է այս ենթածրագրում։", parameter->_name));
        else
            _model.bind(parameter->id(), id);
    }
}

void SemanticAnalyzer::declareReturnValue(const Subroutine& subroutine)
{
    if( !subroutine._returnType.has_value() )
        return;

    const auto& name = subroutine._name;
    const auto type = *subroutine._returnType;
    const auto storage = VariableStorage::ReturnValue;
    const auto id = _symbols.declareVariable(name, type, false, storage);
    if( id == UnknownSymbol )
        report(subroutine, std::format("'{}' անունն արդեն սահմանված է այս ենթածրագրում։", subroutine._name));
    else
        _model.bindReturnValue(subroutine.id(), id);
}

void SemanticAnalyzer::declareLocals(const Sequence& sequence)
{
    for( const auto& statement : sequence._items ) {
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
}

void SemanticAnalyzer::declareDim(const Dim& dim)
{
    const auto storage = VariableStorage::Local;
    const auto id = _symbols.declareVariable(dim._name, dim._type, dim._isArray, storage);
    if( id == UnknownSymbol )
        report(dim, std::format("'{}' անունն արդեն սահմանված է այս ենթածրագրում։", dim._name));
    else
        _model.bind(dim.id(), id);
}

void SemanticAnalyzer::declareForVariable(const For& loop)
{
    const auto& name = loop._parameter->_name;
    if( _symbols.declaredInCurrentScope(name) ) {
        const auto id = *_symbols.lookup(name);
        const auto& symbol = _symbols.symbol(id);
        _model.bind(loop._parameter->id(), id);
        if( symbol.kind != SymbolKind::Variable || symbol.isArray || symbol.type != TypeName::Real )
            report(loop, std::format("FOR-ի '{}' հաշվիչը պետք է լինի պարզ REAL փոփոխական։", name));
        return;
    }

    const auto storage = VariableStorage::ForVariable;
    const auto id = _symbols.declareVariable(name, TypeName::Real, false, storage);
    _model.bind(loop._parameter->id(), id);
}

std::optional<SymbolId> SemanticAnalyzer::resolveVariable(const Variable& variable)
{
    const auto id = _symbols.lookup(variable._name);
    if( !id.has_value() ) {
        report(variable, std::format("'{}' անունով փոփոխական սահմանված չէ։", variable._name));
        return std::nullopt;
    }

    const auto& symbol = _symbols.symbol(*id);
    if( symbol.kind != SymbolKind::Variable ) {
        report(variable, std::format("'{}' անունը փոփոխական չէ։", variable._name));
        return std::nullopt;
    }

    _model.bind(variable.id(), *id);
    _model.setType(variable.id(), *symbol.type);
    return id;
}

std::optional<SymbolId> SemanticAnalyzer::resolveSubroutine(const Node& node,
    std::string_view name)
{
    const auto id = _symbols.lookupSubroutine(name);
    if( !id.has_value() ) {
        report(node, std::format("'{}' անունով ենթածրագիր սահմանված չէ։", name));
        return std::nullopt;
    }

    _model.bind(node.id(), *id);
    return id;
}

void SemanticAnalyzer::validateArguments(const Node& node, std::string_view name,
    const std::vector<Expression::Ptr>& arguments,
    const SubroutineSignature& signature)
{
    const auto expectedCount = signature.parameters.size();
    const auto actualCount = arguments.size();
    if( actualCount != expectedCount )
        report(node, std::format("'{}' ենթածրագիրն ունի {} պարամետր, բայց ստացել է {} արգումենտ։", name, expectedCount, actualCount));

    const auto commonCount = std::min(actualCount, expectedCount);
    for( std::size_t index = 0; index < commonCount; ++index ) {
        const auto& argument = arguments[index];
        const auto argumentType = expressionType(argument);
        const auto& parameter = signature.parameters[index];

        if( !argumentType.has_value() )
            continue;

        if( parameter.isArray ) {
            if( !isArrayExpression(*argument) ) {
                report(*argument, "Սպասվում է զանգվածային արգումենտ։");
                continue;
            }
        }
        else if( !requireScalar(*argument) ) {
            continue;
        }

        if( !parameter.type.has_value() )
            continue;

        const auto expectedType = *parameter.type;
        const auto wrongArgumentType = typeMismatch(argumentType, expectedType);
        if( wrongArgumentType )
            report(*argument, std::format("'{}' ենթածրագրի թիվ {} արգումենտը պետք է լինի {}, բայց ստացվել է {}։", name, index + 1, expectedType, *argumentType));
    }
}

std::optional<TypeName> SemanticAnalyzer::expressionType(const Expression::Ptr& expression)
{
    if( const auto type = _model.type(expression->id()) )
        return type;
    visit(*expression);
    return _model.type(expression->id());
}

bool SemanticAnalyzer::isArrayExpression(const Expression& expression) const
{
    if( expression.kind != NodeKind::Variable )
        return false;
    const auto id = _model.symbol(expression.id());
    return id.has_value() && _symbols.symbol(*id).isArray;
}

bool SemanticAnalyzer::requireScalar(const Expression& expression)
{
    if( !isArrayExpression(expression) )
        return true;
    report(expression, "Զանգվածը չի կարող օգտագործվել որպես պարզ արժեք։");
    return false;
}

void SemanticAnalyzer::validateIndex(const Expression::Ptr& index)
{
    const auto indexType = expressionType(index);
    const auto scalarIndex = requireScalar(*index);
    const auto wrongIndexType = scalarIndex && typeMismatch(indexType, TypeName::Real);
    if( wrongIndexType )
        report(*index, std::format("Զանգվածի ինդեքսը պետք է լինի REAL, բայց ստացվել է {}։", *indexType));
}

ParameterInfo SemanticAnalyzer::parameterInfo(const Dim& parameter) const
{
    return {parameter._type, parameter._isArray};
}

void SemanticAnalyzer::report(const Node& node, std::string_view message)
{
    _diagnostics.advance();
    _diagnostics.mark(node.line, message);
}

} // namespace avium
