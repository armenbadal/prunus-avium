#include "diagnostics.hxx"

namespace avium {

std::ostream& operator<<(std::ostream& output, const Error& error)
{
    return output << std::get<0>(error) << ": " << std::get<1>(error);
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
