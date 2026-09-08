#pragma once

#include "ast.hxx"

#include <format>
#include <string_view>
#include <utility>

namespace std {

template<>
struct formatter<avium::TypeName> : formatter<string_view> {
    format_context::iterator format(avium::TypeName type, format_context& context) const
    {
        const auto value = [type] {
            switch( type ) {
                case avium::TypeName::Bool:
                    return string_view{"BOOL"};
                case avium::TypeName::Real:
                    return string_view{"REAL"};
                case avium::TypeName::Text:
                    return string_view{"TEXT"};
            }
            unreachable();
        }();
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
