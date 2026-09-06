#pragma once

#include "ast.hxx"
#include "diagnostics.hxx"
#include "scanner.hxx"

#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace avium {

class Parser {
public:
    Parser(Scanner& scanner, Diagnostics& diagnostics);

    Program::Ptr parse();

private:
    Program::Ptr parseProgram();
    Subroutine::Ptr parseSubroutine();
    Sequence::Ptr parseSequence();
    Statement::Ptr parseStatement();
    Let::Ptr parseLet();
    Dim::Ptr parseDim();
    Dim::Ptr parseDeclaration(bool sizeRequired);
    Parameter::Ptr parseParameter();
    If::Ptr parseIf();
    IfBranch::Ptr parseIfBranch(Token keyword);
    While::Ptr parseWhile();
    For::Ptr parseFor();
    Call::Ptr parseCall();

    std::vector<Expression::Ptr> parseExpressionList();
    Expression::Ptr parseExpression();
    Expression::Ptr parseDisjunction();
    Expression::Ptr parseConjunction();
    Expression::Ptr parseNegation();
    Expression::Ptr parseEquality();
    Expression::Ptr parseComparison();
    Expression::Ptr parseAddition();
    Expression::Ptr parseMultiplication();
    Expression::Ptr parsePower();
    Expression::Ptr parseUnary();
    Expression::Ptr parseSubscript();
    Expression::Ptr parseFactor();
    Number::Ptr parseNumber();
    Expression::Ptr parseIdentifier();

    void advance();
    std::string match(Token expected);
    void parseNewLines();
    void parseBlockEnd(Token keyword);
    void synchronize(const std::set<Token>& stops, std::string_view message);

    Scanner& _scanner;
    Diagnostics& _diagnostics;
    Lexeme _lookahead;
};

} // namespace avium
