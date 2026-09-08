#pragma once

#include <string>
#include <utility>

namespace avium {

// Բառային տարրերի պիտակները։
enum class Token : int {
    None,

    Identifier,
    RealLit,
    TextLit,
    BoolLit,

    Subroutine,
    Dim,
    As,
    Let,
    If,
    Then,
    ElseIf,
    Else,
    While,
    For,
    To,
    Step,
    Call,
    End,

    Real,
    Text,
    Bool,

    NewLine,

    Eq,
    Ne,
    Lt,
    Le,
    Gt,
    Ge,

    LeftPar,
    RightPar,
    LeftBrack,
    RightBrack,
    Comma,

    Add,
    Sub,
    Amp,
    Or,
    Mul,
    Div,
    Mod,
    Quot,
    And,
    Pow,
    Not,

    Eof,
};

// Scanner-ի և parser-ի միջև փոխանցվող մեկ բառային տարր։
class Lexeme {
public:
    Token kind = Token::None;
    std::string value;
    unsigned int line = 0;

    Lexeme() = default;

    Lexeme(Token kind, std::string value, unsigned int line)
        : kind{kind}, value{std::move(value)}, line{line}
    {
    }

    bool is(Token expected) const
    {
        return kind == expected;
    }

    template<typename... Tokens>
    bool is(Token expected, Tokens... alternatives) const
    {
        return is(expected) || is(alternatives...);
    }

    std::string toString() const;
};

} // namespace avium
