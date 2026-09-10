#pragma once

#include "ast.hxx"

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

template<typename Declaration = Dim>
std::unique_ptr<Declaration> scalarDeclaration(
    std::string_view name, ScalarType::Name type, Position line)
{
    auto declarationType = node<ScalarType>(type, line);
    return node<Declaration>(name, std::move(declarationType), line);
}

template<typename Declaration = Dim>
std::unique_ptr<Declaration> arrayDeclaration(std::string_view name,
    Expression::Ptr size, ScalarType::Name type, Position line)
{
    auto base = node<ScalarType>(type, line);
    auto declarationType = node<ArrayType>(std::move(base), std::move(size), line);
    return node<Declaration>(name, std::move(declarationType), line);
}

} // namespace avium::test
