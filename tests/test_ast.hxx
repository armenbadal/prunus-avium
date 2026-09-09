#pragma once

#include <concepts>
#include <memory>
#include <utility>
#include <vector>

namespace avium::test {

template<typename Base>
class NodeList {
public:
    NodeList() = default;

    template<typename... Derived>
        requires(sizeof...(Derived) > 0 && (std::derived_from<Derived, Base> && ...))
    NodeList(std::unique_ptr<Derived>... values)
    {
        _values.reserve(sizeof...(values));
        (_values.push_back(std::move(values)), ...);
    }

    operator std::vector<std::unique_ptr<Base>>() &&
    {
        return std::move(_values);
    }

private:
    std::vector<std::unique_ptr<Base>> _values;
};

} // namespace avium::test
