#include "parser.hxx"
#include "formatters.hxx"

#include <exception>
#include <format>
#include <set>
#include <utility>

namespace avium {

namespace {

const std::set<Token> firstStatement{Token::Let, Token::Dim, Token::If,
    Token::While, Token::For, Token::Call, Token::Return};
const std::set<Token> firstExpression{Token::BoolLit, Token::RealLit,
    Token::TextLit, Token::Identifier, Token::Add, Token::Sub, Token::Not,
    Token::LeftPar};
const std::set<Token> sequenceEnd{Token::End, Token::ElseIf, Token::Else,
    Token::Subroutine, Token::Eof};
const std::set<Token> statementSync{Token::NewLine, Token::Let, Token::Dim,
    Token::If, Token::While, Token::For, Token::Call, Token::Return, Token::End,
    Token::ElseIf, Token::Else, Token::Subroutine, Token::Eof};
const std::set<Token> subroutineSync{Token::Subroutine, Token::Eof};
const std::set<Token> expressionSync{Token::BoolLit, Token::RealLit,
    Token::TextLit, Token::Identifier, Token::Add, Token::Sub, Token::Not,
    Token::LeftPar, Token::RightPar, Token::RightBrack, Token::Comma,
    Token::NewLine, Token::End, Token::ElseIf, Token::Else,
    Token::Subroutine, Token::Eof};

Operation operation(Token token)
{
    switch( token ) {
        case Token::Add:
            return Operation::Add;
        case Token::Sub:
            return Operation::Sub;
        case Token::Amp:
            return Operation::Conc;
        case Token::Mul:
            return Operation::Mul;
        case Token::Div:
            return Operation::Div;
        case Token::Mod:
            return Operation::Mod;
        case Token::Quot:
            return Operation::Quot;
        case Token::Pow:
            return Operation::Pow;
        case Token::Not:
            return Operation::Not;
        case Token::Eq:
            return Operation::Eq;
        case Token::Ne:
            return Operation::Ne;
        case Token::Gt:
            return Operation::Gt;
        case Token::Ge:
            return Operation::Ge;
        case Token::Lt:
            return Operation::Lt;
        case Token::Le:
            return Operation::Le;
        case Token::And:
            return Operation::And;
        case Token::Or:
            return Operation::Or;
        default:
            return Operation::None;
    }
}

ScalarType::Name typeName(Token token)
{
    switch( token ) {
        case Token::Bool:
            return ScalarType::Name::Bool;
        case Token::Real:
            return ScalarType::Name::Real;
        case Token::Text:
            return ScalarType::Name::Text;
        default:
            std::unreachable();
    }
}

} // namespace

Parser::Parser(Scanner& scanner, Diagnostics& diagnostics) : _scanner{scanner}, _diagnostics{diagnostics}
{
}

void Parser::advance()
{
    _lookahead = _scanner.scan();
    _diagnostics.advance();
}

std::string Parser::match(Token expected)
{
    if( !_lookahead.is(expected) ) {
        _diagnostics.mark(_lookahead.line, std::format("Սպասվում է '{}', բայց հանդիպել է {}։", expected, _lookahead));
        return {};
    }

    auto value = _lookahead.value;
    advance();
    return value;
}

void Parser::synchronize(const std::set<Token>& stops, std::string_view message)
{
    if( stops.contains(_lookahead.kind) )
        return;

    _diagnostics.mark(_lookahead.line, message);
    while( !stops.contains(_lookahead.kind) )
        advance();
}

Program::Ptr Parser::parse()
{
    return parseProgram();
}

Program::Ptr Parser::parseProgram()
{
    advance();
    const auto line = _lookahead.line;
    while( _lookahead.is(Token::NewLine) )
        advance();

    std::vector<Subroutine::Ptr> subroutines;
    while( !_lookahead.is(Token::Eof) ) {
        synchronize(subroutineSync, std::format("Սպասվում է 'SUB', բայց հանդիպել է {}։", _lookahead));
        if( _lookahead.is(Token::Subroutine) )
            subroutines.push_back(parseSubroutine());
        parseNewLines();
    }

    return node<Program>(std::move(subroutines), line);
}

Subroutine::Ptr Parser::parseSubroutine()
{
    const auto line = _lookahead.line;
    match(Token::Subroutine);
    const auto name = match(Token::Identifier);

    std::vector<Parameter::Ptr> parameters;
    if( _lookahead.is(Token::LeftPar) ) {
        match(Token::LeftPar);
        if( _lookahead.is(Token::Identifier) ) {
            if( auto parameter = parseParameter() )
                parameters.push_back(std::move(parameter));
            while( _lookahead.is(Token::Comma) ) {
                match(Token::Comma);
                if( auto parameter = parseParameter() )
                    parameters.push_back(std::move(parameter));
            }
        }
        match(Token::RightPar);
    }

    ScalarType::Ptr returnType;
    if( _lookahead.is(Token::As) )
        returnType = parseType();

    auto body = parseSequence();
    parseBlockEnd(Token::Subroutine);
    return node<Subroutine>(name, std::move(parameters), std::move(returnType), std::move(body), line);
}

Parameter::Ptr Parser::parseParameter()
{
    const auto line = _lookahead.line;
    const auto name = match(Token::Identifier);
    bool isArray = false;
    if( _lookahead.is(Token::LeftBrack) ) {
        match(Token::LeftBrack);
        match(Token::RightBrack);
        isArray = true;
    }

    auto base = parseType();
    if( !base )
        return {};

    Type::Ptr parameterType;
    if( isArray )
        parameterType = node<ArrayType>(std::move(base), nullptr, line);
    else
        parameterType = std::move(base);
    return node<Parameter>(name, std::move(parameterType), line);
}

ScalarType::Ptr Parser::parseType()
{
    match(Token::As);
    if( !_lookahead.is(Token::Real, Token::Text, Token::Bool) ) {
        _diagnostics.mark(_lookahead.line, std::format("Սպասվում է տիպ (REAL | TEXT | BOOL), բայց հանդիպել է {}։", _lookahead));
        return nullptr;
    }

    const auto token = _lookahead.kind;
    const auto line = _lookahead.line;
    match(token);
    return node<ScalarType>(typeName(token), line);
}

Sequence::Ptr Parser::parseSequence()
{
    const auto line = _lookahead.line;
    parseNewLines();
    std::vector<Statement::Ptr> statements;

    while( !sequenceEnd.contains(_lookahead.kind) ) {
        if( _lookahead.is(Token::NewLine) ) {
            parseNewLines();
            continue;
        }
        synchronize(statementSync, std::format("Սպասվում է հրաման, բայց հանդիպել է {}։", _lookahead));
        if( firstStatement.contains(_lookahead.kind) ) {
            if( auto statement = parseStatement() )
                statements.push_back(std::move(statement));
            parseNewLines();
        }
    }
    return node<Sequence>(std::move(statements), line);
}

Statement::Ptr Parser::parseStatement()
{
    if( _lookahead.is(Token::Let) )
        return parseLet();
    if( _lookahead.is(Token::Dim) )
        return parseDim();
    if( _lookahead.is(Token::If) )
        return parseIf();
    if( _lookahead.is(Token::While) )
        return parseWhile();
    if( _lookahead.is(Token::For) )
        return parseFor();
    if( _lookahead.is(Token::Call) )
        return parseCall();
    if( _lookahead.is(Token::Return) )
        return parseReturn();
    return {};
}

Let::Ptr Parser::parseLet()
{
    const auto line = _lookahead.line;
    match(Token::Let);
    const auto name = match(Token::Identifier);
    Expression::Ptr index;
    if( _lookahead.is(Token::LeftBrack) ) {
        match(Token::LeftBrack);
        index = parseExpression();
        match(Token::RightBrack);
    }
    match(Token::Eq);
    return node<Let>(node<Variable>(name, line), std::move(index), parseExpression(), line);
}

Dim::Ptr Parser::parseDim()
{
    match(Token::Dim);
    return parseDeclaration(true);
}

Dim::Ptr Parser::parseDeclaration(bool sizeRequired)
{
    const auto line = _lookahead.line;
    const auto name = match(Token::Identifier);
    Expression::Ptr size;
    bool isArray = false;
    if( _lookahead.is(Token::LeftBrack) ) {
        match(Token::LeftBrack);
        if( firstExpression.contains(_lookahead.kind) )
            size = parseExpression();
        else if( sizeRequired )
            _diagnostics.mark(_lookahead.line, std::format("Սպասվում է չափը, բայց հանդիպել է {}։", _lookahead));
        match(Token::RightBrack);
        isArray = true;
    }

    auto base = parseType();
    if( !base )
        return {};

    Type::Ptr declarationType;
    if( isArray )
        declarationType = node<ArrayType>(std::move(base), std::move(size), line);
    else
        declarationType = std::move(base);
    return node<Dim>(name, std::move(declarationType), line);
}

If::Ptr Parser::parseIf()
{
    const auto line = _lookahead.line;
    std::vector<IfBranch::Ptr> branches;
    branches.push_back(parseIfBranch(Token::If));
    while( _lookahead.is(Token::ElseIf) )
        branches.push_back(parseIfBranch(Token::ElseIf));

    Sequence::Ptr alternative;
    if( _lookahead.is(Token::Else) ) {
        match(Token::Else);
        alternative = parseSequence();
    }
    parseBlockEnd(Token::If);
    return node<If>(std::move(branches), std::move(alternative), line);
}

IfBranch::Ptr Parser::parseIfBranch(Token keyword)
{
    const auto line = _lookahead.line;
    match(keyword);
    auto condition = parseExpression();
    match(Token::Then);
    return node<IfBranch>(std::move(condition), parseSequence(), line);
}

While::Ptr Parser::parseWhile()
{
    const auto line = _lookahead.line;
    match(Token::While);
    auto condition = parseExpression();
    auto body = parseSequence();
    parseBlockEnd(Token::While);
    return node<While>(std::move(condition), std::move(body), line);
}

For::Ptr Parser::parseFor()
{
    const auto line = _lookahead.line;
    match(Token::For);
    const auto name = match(Token::Identifier);
    auto parameter = node<Variable>(name, line);
    match(Token::Eq);
    auto begin = parseExpression();
    match(Token::To);
    auto end = parseExpression();

    auto step = node<Number>(1.0, line);
    if( _lookahead.is(Token::Step) ) {
        match(Token::Step);
        bool negative = false;
        if( _lookahead.is(Token::Add, Token::Sub) ) {
            negative = _lookahead.is(Token::Sub);
            advance();
        }
        step = parseNumber();
        if( negative )
            step = node<Number>(-step->_value, step->line);
    }

    auto body = parseSequence();
    parseBlockEnd(Token::For);
    return node<For>(std::move(parameter), std::move(begin), std::move(end),
        std::move(step), std::move(body), line);
}

Call::Ptr Parser::parseCall()
{
    const auto line = _lookahead.line;
    match(Token::Call);
    const auto name = match(Token::Identifier);
    return node<Call>(name, parseExpressionList(), line);
}

Return::Ptr Parser::parseReturn()
{
    const auto line = _lookahead.line;
    match(Token::Return);
    return node<Return>(parseExpression(), line);
}

std::vector<Expression::Ptr> Parser::parseExpressionList()
{
    std::vector<Expression::Ptr> result;
    if( !firstExpression.contains(_lookahead.kind) )
        return result;
    result.push_back(parseExpression());
    while( _lookahead.is(Token::Comma) ) {
        match(Token::Comma);
        result.push_back(parseExpression());
    }
    return result;
}

Expression::Ptr Parser::parseExpression()
{
    return parseDisjunction();
}

Expression::Ptr Parser::parseDisjunction()
{
    auto result = parseConjunction();
    while( _lookahead.is(Token::Or) ) {
        const auto line = _lookahead.line;
        match(Token::Or);
        result = node<Binary>(Operation::Or, std::move(result), parseConjunction(), line);
    }
    return result;
}

Expression::Ptr Parser::parseConjunction()
{
    auto result = parseNegation();
    while( _lookahead.is(Token::And) ) {
        const auto line = _lookahead.line;
        match(Token::And);
        result = node<Binary>(Operation::And, std::move(result), parseNegation(), line);
    }
    return result;
}

Expression::Ptr Parser::parseNegation()
{
    if( _lookahead.is(Token::Not) ) {
        const auto line = _lookahead.line;
        match(Token::Not);
        return node<Unary>(Operation::Not, parseNegation(), line);
    }
    return parseEquality();
}

Expression::Ptr Parser::parseEquality()
{
    auto result = parseComparison();
    if( _lookahead.is(Token::Eq, Token::Ne) ) {
        const auto token = _lookahead.kind;
        const auto line = _lookahead.line;
        match(token);
        result = node<Binary>(operation(token), std::move(result), parseComparison(), line);
    }
    return result;
}

Expression::Ptr Parser::parseComparison()
{
    auto result = parseAddition();
    if( _lookahead.is(Token::Gt, Token::Ge, Token::Lt, Token::Le) ) {
        const auto token = _lookahead.kind;
        const auto line = _lookahead.line;
        match(token);
        result = node<Binary>(operation(token), std::move(result), parseAddition(), line);
    }
    return result;
}

Expression::Ptr Parser::parseAddition()
{
    auto result = parseMultiplication();
    while( _lookahead.is(Token::Add, Token::Sub, Token::Amp) ) {
        const auto token = _lookahead.kind;
        const auto line = _lookahead.line;
        match(token);
        result = node<Binary>(operation(token), std::move(result), parseMultiplication(), line);
    }
    return result;
}

Expression::Ptr Parser::parseMultiplication()
{
    auto result = parsePower();
    while( _lookahead.is(Token::Mul, Token::Div, Token::Quot, Token::Mod) ) {
        const auto token = _lookahead.kind;
        const auto line = _lookahead.line;
        match(token);
        result = node<Binary>(operation(token), std::move(result), parsePower(), line);
    }
    return result;
}

Expression::Ptr Parser::parsePower()
{
    auto result = parseUnary();
    if( _lookahead.is(Token::Pow) ) {
        const auto line = _lookahead.line;
        match(Token::Pow);
        result = node<Binary>(Operation::Pow, std::move(result), parsePower(), line);
    }
    return result;
}

Expression::Ptr Parser::parseUnary()
{
    const auto line = _lookahead.line;
    std::vector<Operation> operators;
    while( _lookahead.is(Token::Add, Token::Sub) ) {
        operators.push_back(operation(_lookahead.kind));
        advance();
    }

    auto result = parseSubscript();
    for( auto iterator = operators.rbegin(); iterator != operators.rend(); ++iterator )
        result = node<Unary>(*iterator, std::move(result), line);
    return result;
}

Expression::Ptr Parser::parseSubscript()
{
    auto result = parseFactor();
    if( _lookahead.is(Token::LeftBrack) ) {
        const auto line = _lookahead.line;
        match(Token::LeftBrack);
        result = node<Binary>(Operation::Index, std::move(result), parseExpression(), line);
        match(Token::RightBrack);
    }
    return result;
}

Expression::Ptr Parser::parseFactor()
{
    if( !firstExpression.contains(_lookahead.kind) ) {
        _diagnostics.mark(_lookahead.line, std::format("Սպասվում է արտահայտություն, բայց հանդիպել է {}։", _lookahead));
        while( !expressionSync.contains(_lookahead.kind) )
            advance();
        return node<Number>(0.0, _lookahead.line);
    }

    if( _lookahead.is(Token::BoolLit) ) {
        const auto line = _lookahead.line;
        const auto value = match(Token::BoolLit);
        return node<Boolean>(value == "TRUE", line);
    }

    if( _lookahead.is(Token::RealLit) )
        return parseNumber();

    if( _lookahead.is(Token::TextLit) ) {
        const auto line = _lookahead.line;
        return node<Text>(match(Token::TextLit), line);
    }

    if( _lookahead.is(Token::Identifier) )
        return parseIdentifier();

    match(Token::LeftPar);
    auto result = parseExpression();
    match(Token::RightPar);

    return result;
}

Number::Ptr Parser::parseNumber()
{
    const auto line = _lookahead.line;
    const auto value = match(Token::RealLit);
    try {
        return node<Number>(std::stod(value), line);
    }
    catch( const std::exception& ) {
        _diagnostics.mark(line, std::format("Սխալ թվային հաստատուն՝ '{}'։", value));
        return node<Number>(0.0, line);
    }
}

Expression::Ptr Parser::parseIdentifier()
{
    const auto line = _lookahead.line;
    const auto name = match(Token::Identifier);
    if( !_lookahead.is(Token::LeftPar) )
        return node<Variable>(name, line);

    match(Token::LeftPar);
    auto arguments = parseExpressionList();
    match(Token::RightPar);
    return node<Apply>(name, std::move(arguments), line);
}

void Parser::parseNewLines()
{
    if( !_lookahead.is(Token::NewLine, Token::Eof) )
        _diagnostics.mark(_lookahead.line, std::format("Սպասվում է տողի ավարտ, բայց հանդիպել է {}։", _lookahead));
    while( _lookahead.is(Token::NewLine) )
        advance();
}

void Parser::parseBlockEnd(Token keyword)
{
    if( !_lookahead.is(Token::End) ) {
        _diagnostics.mark(_lookahead.line, std::format("Սպասվում է 'END {}', բայց հանդիպել է {}։", keyword, _lookahead));
        return;
    }
    advance();
    match(keyword);
}

} // namespace avium
