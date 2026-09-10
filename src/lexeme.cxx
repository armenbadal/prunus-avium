#include "lexeme.hxx"
#include "formatters.hxx"

#include <format>

namespace avium {

std::string Lexeme::toString() const
{
    return std::format("<{}, {}, {}>", kind, value, line);
}

} // namespace avium
