#pragma once

#include <cstddef>
#include <ostream>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

namespace avium {

using Error = std::tuple<unsigned int, std::string>;

std::ostream& operator<<(std::ostream& output, const Error& error);

class Diagnostics {
public:
    static constexpr std::size_t MaxErrors = 8;

    void mark(unsigned int line, std::string_view message);
    void advance() noexcept { _advanced = true; }
    const std::vector<Error>& errors() const noexcept { return _errors; }
    std::size_t count() const noexcept { return _count; }

private:
    std::vector<Error> _errors;
    std::size_t _count{0};
    bool _advanced{true};
};

} // namespace avium
