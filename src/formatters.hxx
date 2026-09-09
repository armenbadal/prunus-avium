#pragma once

#include "ast.hxx"
#include "lexeme.hxx"

#include <format>
#include <string>
#include <string_view>
#include <utility>

namespace std {

template<>
struct formatter<avium::Token> : formatter<string_view> {
    format_context::iterator format(avium::Token token, format_context& context) const
    {
        const auto value = [token] {
            switch( token ) {
                case avium::Token::None:
                    return string_view{"None"};
                case avium::Token::Identifier:
                    return string_view{"IDENT"};
                case avium::Token::RealLit:
                    return string_view{"RealLit"};
                case avium::Token::TextLit:
                    return string_view{"TextLit"};
                case avium::Token::BoolLit:
                    return string_view{"BoolLit"};
                case avium::Token::Subroutine:
                    return string_view{"SUB"};
                case avium::Token::Dim:
                    return string_view{"DIM"};
                case avium::Token::As:
                    return string_view{"AS"};
                case avium::Token::Let:
                    return string_view{"LET"};
                case avium::Token::If:
                    return string_view{"IF"};
                case avium::Token::Then:
                    return string_view{"THEN"};
                case avium::Token::ElseIf:
                    return string_view{"ELSEIF"};
                case avium::Token::Else:
                    return string_view{"ELSE"};
                case avium::Token::While:
                    return string_view{"WHILE"};
                case avium::Token::For:
                    return string_view{"FOR"};
                case avium::Token::To:
                    return string_view{"TO"};
                case avium::Token::Step:
                    return string_view{"STEP"};
                case avium::Token::Call:
                    return string_view{"CALL"};
                case avium::Token::End:
                    return string_view{"END"};
                case avium::Token::Real:
                    return string_view{"REAL"};
                case avium::Token::Text:
                    return string_view{"TEXT"};
                case avium::Token::Bool:
                    return string_view{"BOOL"};
                case avium::Token::NewLine:
                    return string_view{"New Line"};
                case avium::Token::Eq:
                    return string_view{"="};
                case avium::Token::Ne:
                    return string_view{"<>"};
                case avium::Token::Lt:
                    return string_view{"<"};
                case avium::Token::Le:
                    return string_view{"<="};
                case avium::Token::Gt:
                    return string_view{">"};
                case avium::Token::Ge:
                    return string_view{">="};
                case avium::Token::LeftPar:
                    return string_view{"("};
                case avium::Token::RightPar:
                    return string_view{")"};
                case avium::Token::LeftBrack:
                    return string_view{"["};
                case avium::Token::RightBrack:
                    return string_view{"]"};
                case avium::Token::Comma:
                    return string_view{","};
                case avium::Token::Add:
                    return string_view{"+"};
                case avium::Token::Sub:
                    return string_view{"-"};
                case avium::Token::Amp:
                    return string_view{"&"};
                case avium::Token::Or:
                    return string_view{"OR"};
                case avium::Token::Mul:
                    return string_view{"*"};
                case avium::Token::Div:
                    return string_view{"/"};
                case avium::Token::Mod:
                    return string_view{"MOD"};
                case avium::Token::Quot:
                    return string_view{"\\"};
                case avium::Token::And:
                    return string_view{"AND"};
                case avium::Token::Pow:
                    return string_view{"^"};
                case avium::Token::Not:
                    return string_view{"NOT"};
                case avium::Token::Eof:
                    return string_view{"Eof"};
            }
            unreachable();
        }();
        return formatter<string_view>::format(value, context);
    }
};

template<>
struct formatter<avium::Lexeme> : formatter<string_view> {
    format_context::iterator format(const avium::Lexeme& lexeme, format_context& context) const
    {
        string value;
        switch( lexeme.kind ) {
            case avium::Token::NewLine:
                value = "տողի ավարտ";
                break;
            case avium::Token::Eof:
                value = "ֆայլի ավարտ";
                break;
            case avium::Token::None:
                value = std::format("անհայտ նիշ '{}'", lexeme.value);
                break;
            case avium::Token::RealLit:
            case avium::Token::TextLit:
            case avium::Token::Identifier:
                value = std::format("'{}'", lexeme.value);
                break;
            default:
                value = std::format("'{}'", lexeme.kind);
                break;
        }
        return formatter<string_view>::format(value, context);
    }
};

template<>
struct formatter<avium::ScalarType::Name> : formatter<string_view> {
    format_context::iterator format(avium::ScalarType::Name type, format_context& context) const
    {
        const auto value = [type] {
            switch( type ) {
                case avium::ScalarType::Name::Bool:
                    return string_view{"BOOL"};
                case avium::ScalarType::Name::Real:
                    return string_view{"REAL"};
                case avium::ScalarType::Name::Text:
                    return string_view{"TEXT"};
            }
            unreachable();
        }();
        return formatter<string_view>::format(value, context);
    }
};

template<>
struct formatter<avium::Type> : formatter<string_view> {
    format_context::iterator format(const avium::Type& type, format_context& context) const
    {
        const auto base = std::format("{}", avium::baseType(type)._name);
        const auto value = avium::isArrayType(type) ? base + "[]" : base;
        return formatter<string_view>::format(value, context);
    }
};

template<>
struct formatter<avium::Operation> : formatter<string_view> {
    format_context::iterator format(avium::Operation operation, format_context& context) const
    {
        string_view value{"?"};
        switch( operation ) {
            case avium::Operation::Add:
                value = "+";
                break;
            case avium::Operation::Sub:
                value = "-";
                break;
            case avium::Operation::Mul:
                value = "*";
                break;
            case avium::Operation::Div:
                value = "/";
                break;
            case avium::Operation::Quot:
                value = "\\";
                break;
            case avium::Operation::Mod:
                value = "MOD";
                break;
            case avium::Operation::Pow:
                value = "^";
                break;
            case avium::Operation::Eq:
                value = "=";
                break;
            case avium::Operation::Ne:
                value = "<>";
                break;
            case avium::Operation::Gt:
                value = ">";
                break;
            case avium::Operation::Ge:
                value = ">=";
                break;
            case avium::Operation::Lt:
                value = "<";
                break;
            case avium::Operation::Le:
                value = "<=";
                break;
            case avium::Operation::And:
                value = "AND";
                break;
            case avium::Operation::Or:
                value = "OR";
                break;
            case avium::Operation::Not:
                value = "NOT";
                break;
            case avium::Operation::Conc:
                value = "&";
                break;
            case avium::Operation::Index:
                value = "[]";
                break;
            case avium::Operation::None:
                break;
        }
        return formatter<string_view>::format(value, context);
    }
};

} // namespace std
