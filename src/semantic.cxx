#include "semantic.hxx"

#include <algorithm>
#include <cmath>
#include <format>
#include <utility>

namespace avium {

namespace {

std::string_view typeName(TypeName type)
{
    switch( type ) {
        case TypeName::Bool:
            return "BOOL";
        case TypeName::Real:
            return "REAL";
        case TypeName::Text:
            return "TEXT";
        case TypeName::Unknown:
            return "UNKNOWN";
    }
    return "UNKNOWN";
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
    if( scalarSize )
        requireType(*dim._size, sizeType, TypeName::Real);

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

    if( validTarget )
        requireType(*let._value, valueType, symbol.type);
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
    if( scalarCondition )
        requireType(*branch._condition, conditionType, TypeName::Bool);
    visit(*branch._body);
}

void SemanticAnalyzer::visit(While& loop)
{
    const auto conditionType = expressionType(loop._condition);
    const auto scalarCondition = requireScalar(*loop._condition);
    if( scalarCondition )
        requireType(*loop._condition, conditionType, TypeName::Bool);
    visit(*loop._body);
}

void SemanticAnalyzer::visit(For& loop)
{
    visit(*loop._parameter);

    const auto beginType = expressionType(loop._begin);
    const auto scalarBegin = requireScalar(*loop._begin);
    if( scalarBegin )
        requireType(*loop._begin, beginType, TypeName::Real);

    const auto endType = expressionType(loop._end);
    const auto scalarEnd = requireScalar(*loop._end);
    if( scalarEnd )
        requireType(*loop._end, endType, TypeName::Real);

    const auto stepType = expressionType(loop._step);
    const auto scalarStep = requireScalar(*loop._step);
    if( scalarStep )
        requireType(*loop._step, stepType, TypeName::Real);
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
        case Operation::Not:
            if( scalar )
                requireType(*unary._operand, operandType, TypeName::Bool);
            _model.setType(unary.id(), TypeName::Bool);
            break;
        case Operation::Add:
        case Operation::Sub:
            if( scalar )
                requireType(*unary._operand, operandType, TypeName::Real);
            _model.setType(unary.id(), TypeName::Real);
            break;
        default:
            _model.setType(unary.id(), TypeName::Unknown);
            break;
    }
}

void SemanticAnalyzer::visit(Binary& binary)
{
    const auto leftType = expressionType(binary._left);
    const auto rightType = expressionType(binary._right);

    TypeName type = TypeName::Unknown;
    switch( binary._operation ) {
        case Operation::Add:
        case Operation::Sub:
        case Operation::Mul:
        case Operation::Div:
        case Operation::Quot:
        case Operation::Mod:
        case Operation::Pow:
            if( requireScalar(*binary._left) )
                requireType(*binary._left, leftType, TypeName::Real);
            if( requireScalar(*binary._right) )
                requireType(*binary._right, rightType, TypeName::Real);
            type = TypeName::Real;
            break;
        case Operation::Eq:
        case Operation::Ne: {
            const auto leftScalar = requireScalar(*binary._left);
            const auto rightScalar = requireScalar(*binary._right);
            const auto typesKnown = leftType != TypeName::Unknown && rightType != TypeName::Unknown;
            const auto typesMatch = leftType == rightType;
            if( leftScalar && rightScalar && typesKnown && !typesMatch )
                report(binary, "Հավասարության օպերանդները պետք է նույն տիպի լինեն։");
            type = TypeName::Bool;
            break;
        }
        case Operation::Gt:
        case Operation::Ge:
        case Operation::Lt:
        case Operation::Le: {
            const auto leftScalar = requireScalar(*binary._left);
            const auto rightScalar = requireScalar(*binary._right);
            const auto typesKnown = leftType != TypeName::Unknown && rightType != TypeName::Unknown;
            const auto bothReal = leftType == TypeName::Real && rightType == TypeName::Real;
            const auto bothText = leftType == TypeName::Text && rightType == TypeName::Text;
            const auto comparable = bothReal || bothText;
            if( leftScalar && rightScalar && typesKnown && !comparable )
                report(binary, "Համեմատության օպերանդները պետք է լինեն երկու REAL կամ երկու TEXT արժեք։");
            type = TypeName::Bool;
            break;
        }
        case Operation::And:
        case Operation::Or:
            if( requireScalar(*binary._left) )
                requireType(*binary._left, leftType, TypeName::Bool);
            if( requireScalar(*binary._right) )
                requireType(*binary._right, rightType, TypeName::Bool);
            type = TypeName::Bool;
            break;
        case Operation::Conc:
            if( requireScalar(*binary._left) )
                requireType(*binary._left, leftType, TypeName::Text);
            if( requireScalar(*binary._right) )
                requireType(*binary._right, rightType, TypeName::Text);
            type = TypeName::Text;
            break;
        case Operation::Index:
            if( leftType != TypeName::Unknown && !isArrayExpression(*binary._left) )
                report(*binary._left, "Ինդեքսավորվող արտահայտությունը զանգված չէ։");
            validateIndex(binary._right);
            type = leftType;
            break;
        case Operation::Not:
        case Operation::None:
            break;
    }
    _model.setType(binary.id(), type);
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
        report(apply, std::format("'{}' պրոցեդուրան արժեք չի վերադարձնում։", apply._callee));
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
        for( const auto& parameter : subroutine->_parameters ) {
            parameters.push_back(parameterInfo(*parameter));
            if( parameter->_type == TypeName::Unknown )
                report(*parameter, std::format("'{}' պարամետրի տիպը հայտնի չէ։", parameter->_name));
        }

        if( subroutine->_returnType.has_value() && *subroutine->_returnType == TypeName::Unknown )
            report(*subroutine, std::format("'{}' ենթածրագրի վերադարձվող տիպը հայտնի չէ։", subroutine->_name));

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
    _model.setType(variable.id(), symbol.type);
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
        report(node, std::format("'{}' ենթածրագիրը սպասում է {} արգումենտ, բայց ստացել է {}։", name, expectedCount, actualCount));

    const auto commonCount = std::min(actualCount, expectedCount);
    for( std::size_t index = 0; index < commonCount; ++index ) {
        const auto& argument = arguments[index];
        const auto argumentType = expressionType(argument);
        const auto& parameter = signature.parameters[index];

        if( argumentType == TypeName::Unknown )
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

        if( parameter.type.has_value() )
            requireType(*argument, argumentType, *parameter.type);
    }
}

TypeName SemanticAnalyzer::expressionType(const Expression::Ptr& expression)
{
    if( const auto type = _model.type(expression->id()) )
        return *type;
    visit(*expression);
    return _model.type(expression->id()).value_or(TypeName::Unknown);
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
    if( scalarIndex )
        requireType(*index, indexType, TypeName::Real);
}

ParameterInfo SemanticAnalyzer::parameterInfo(const Dim& parameter) const
{
    return {parameter._type, parameter._isArray};
}

void SemanticAnalyzer::requireType(const Node& node, TypeName actual, TypeName expected)
{
    if( actual == TypeName::Unknown || expected == TypeName::Unknown
        || actual == expected )
        return;
    report(node, std::format("Սպասվում է {}, բայց ստացվել է {}։", typeName(expected), typeName(actual)));
}

void SemanticAnalyzer::report(const Node& node, std::string_view message)
{
    _diagnostics.advance();
    _diagnostics.mark(node.line, message);
}

} // namespace avium
