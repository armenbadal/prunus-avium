#include "lexeme.hxx"

#include <format>
#include <map>

namespace avium {

std::string toString(Token token)
{
    static std::map<Token, std::string> names{
        {Token::None, "None"},
        {Token::Identifier, "IDENT"},
        {Token::RealLit, "RealLit"},
        {Token::TextLit, "TextLit"},
        {Token::BoolLit, "BoolLit"},
        {Token::Subroutine, "SUB"},
        {Token::Dim, "DIM"},
        {Token::As, "AS"},
        {Token::Let, "LET"},
        {Token::If, "IF"},
        {Token::Then, "THEN"},
        {Token::ElseIf, "ELSEIF"},
        {Token::Else, "ELSE"},
        {Token::While, "WHILE"},
        {Token::For, "FOR"},
        {Token::To, "TO"},
        {Token::Step, "STEP"},
        {Token::Call, "CALL"},
        {Token::End, "END"},
        {Token::Real, "REAL"},
        {Token::Text, "TEXT"},
        {Token::Bool, "BOOL"},
        {Token::NewLine, "New Line"},
        {Token::Eq, "="},
        {Token::Ne, "<>"},
        {Token::Lt, "<"},
        {Token::Le, "<="},
        {Token::Gt, ">"},
        {Token::Ge, ">="},
        {Token::LeftPar, "("},
        {Token::RightPar, ")"},
        {Token::LeftBrack, "["},
        {Token::RightBrack, "]"},
        {Token::Comma, ","},
        {Token::Add, "+"},
        {Token::Sub, "-"},
        {Token::Amp, "&"},
        {Token::Or, "OR"},
        {Token::Mul, "*"},
        {Token::Div, "/"},
        {Token::Mod, "MOD"},
        {Token::Quot, "\\"},
        {Token::And, "AND"},
        {Token::Pow, "^"},
        {Token::Not, "NOT"},
        {Token::Eof, "Eof"},
    };

    return names[token];
}

std::string Lexeme::toString() const
{
    return std::format("<{}, {}, {}>", avium::toString(kind), value, line);
}

} // namespace avium
