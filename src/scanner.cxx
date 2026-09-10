#include "scanner.hxx"

#include <cctype>
#include <concepts>
#include <map>
#include <string>
#include <string_view>

namespace avium {

namespace {

const std::map<std::string_view, Token> keywords{
    {"SUB", Token::Subroutine},
    {"LET", Token::Let},
    {"DIM", Token::Dim},
    {"AS", Token::As},
    {"REAL", Token::Real},
    {"TEXT", Token::Text},
    {"BOOL", Token::Bool},
    {"IF", Token::If},
    {"THEN", Token::Then},
    {"ELSEIF", Token::ElseIf},
    {"ELSE", Token::Else},
    {"WHILE", Token::While},
    {"FOR", Token::For},
    {"TO", Token::To},
    {"STEP", Token::Step},
    {"CALL", Token::Call},
    {"RETURN", Token::Return},
    {"END", Token::End},
    {"MOD", Token::Mod},
    {"AND", Token::And},
    {"OR", Token::Or},
    {"NOT", Token::Not},
};

bool isDigit(char c)
{
    return c >= '0' && c <= '9';
}

bool isIdentifierStart(char c)
{
    return std::isalpha(static_cast<unsigned char>(c)) != 0;
}

bool isIdentifierPart(char c)
{
    return isIdentifierStart(c) || isDigit(c);
}

bool isSpace(char c)
{
    return c == ' ' || c == '\t' || c == '\r';
}

template<typename Predicate>
    requires std::predicate<Predicate, char>
std::string readWhile(std::istream& input, Predicate predicate)
{
    std::string result;
    char value{};
    while( input.get(value) ) {
        if( !predicate(value) ) {
            input.unget();
            break;
        }
        result += value;
    }
    return result;
}

Token singleCharacterToken(char value)
{
    switch( value ) {
        case '(':
            return Token::LeftPar;
        case ')':
            return Token::RightPar;
        case '[':
            return Token::LeftBrack;
        case ']':
            return Token::RightBrack;
        case ',':
            return Token::Comma;
        case '+':
            return Token::Add;
        case '-':
            return Token::Sub;
        case '*':
            return Token::Mul;
        case '/':
            return Token::Div;
        case '\\':
            return Token::Quot;
        case '^':
            return Token::Pow;
        case '&':
            return Token::Amp;
        case '=':
            return Token::Eq;
        default:
            return Token::None;
    }
}

} // namespace

Scanner::Scanner(std::istream& source)
    : _source{source}
{
    _source.unsetf(std::ios_base::skipws);
}

Lexeme Scanner::scan()
{
    while( true ) {
        readWhile(_source, isSpace);

        if( _source.peek() != '\'' )
            break;

        char value{};
        while( _source.get(value) ) {
            if( value == '\n' ) {
                _source.unget();
                break;
            }
        }
    }

    const auto next = _source.peek();
    if( next == std::char_traits<char>::eof() )
        return {Token::Eof, "EOF", _line};

    const auto value = static_cast<char>(next);

    if( isDigit(value) )
        return scanNumber();

    if( value == '"' )
        return scanText();

    if( isIdentifierStart(value) )
        return scanIdentifier();

    if( value == '\n' ) {
        _source.get();
        const auto line = _line++;
        return {Token::NewLine, "\n", line};
    }

    if( value == '<' ) {
        _source.get();
        Lexeme result{Token::Lt, "<", _line};
        if( _source.peek() == '>' ) {
            _source.get();
            result.kind = Token::Ne;
            result.value = "<>";
        }
        else if( _source.peek() == '=' ) {
            _source.get();
            result.kind = Token::Le;
            result.value = "<=";
        }
        return result;
    }

    if( value == '>' ) {
        _source.get();
        Lexeme result{Token::Gt, ">", _line};
        if( _source.peek() == '=' ) {
            _source.get();
            result.kind = Token::Ge;
            result.value = ">=";
        }
        return result;
    }

    _source.get();
    return {singleCharacterToken(value), std::string{value}, _line};
}

Lexeme Scanner::scanNumber()
{
    const auto line = _line;
    auto value = readWhile(_source, isDigit);

    if( _source.peek() == '.' ) {
        _source.get();
        value += '.';

        const auto next = _source.peek();
        if( next == std::char_traits<char>::eof() || !isDigit(static_cast<char>(next)) )
            return {Token::None, value, line};

        value += readWhile(_source, isDigit);
    }

    return {Token::RealLit, value, line};
}

Lexeme Scanner::scanText()
{
    const auto line = _line;
    _source.get(); // բացող չակերտը

    std::string value;
    char current{};
    while( _source.get(current) ) {
        if( current == '"' )
            return {Token::TextLit, value, line};

        if( current == '\n' || current == '\r' ) {
            _source.unget();
            return {Token::None, value, line};
        }

        value += current;
    }

    return {Token::None, value, line};
}

Lexeme Scanner::scanIdentifier()
{
    const auto line = _line;
    const auto value = readWhile(_source, isIdentifierPart);

    if( value == "TRUE" || value == "FALSE" )
        return {Token::BoolLit, value, line};

    const auto keyword = keywords.find(value);
    const auto kind = keyword == keywords.end() ? Token::Identifier : keyword->second;
    return {kind, value, line};
}

} // namespace avium
