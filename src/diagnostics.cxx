#include "diagnostics.hxx"

#include <format>

namespace avium {

std::ostream& operator<<(std::ostream& output, const Error& error)
{
    return output << std::get<0>(error) << ": " << std::get<1>(error);
}

std::string describe(const Lexeme& lexeme)
{
    switch( lexeme.kind ) {
        case Token::NewLine:
            return "տողի ավարտ";
        case Token::Eof:
            return "ֆայլի ավարտ";
        case Token::None:
            return std::format("անհայտ նիշ '{}'", lexeme.value);
        case Token::RealLit:
        case Token::TextLit:
        case Token::Identifier:
            return std::format("'{}'", lexeme.value);
        default:
            return std::format("'{}'", toString(lexeme.kind));
    }
}

void Diagnostics::mark(unsigned int line, std::string_view message)
{
    if( !_advanced )
        return;

    _advanced = false;
    ++_count;
    if( _count <= MaxErrors )
        _errors.emplace_back(line, message);
}

} // namespace avium
