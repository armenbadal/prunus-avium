#include <catch2/catch_test_macros.hpp>

#include "diagnostics.hxx"
#include "parser.hxx"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string_view>

using namespace avium;

namespace {

struct Parsed {
    Program::Ptr program;
    Diagnostics diagnostics;
};

Parsed parse(std::string_view source)
{
    std::istringstream input{std::string{source}};
    Scanner scanner{input};
    Parsed result;
    Parser parser{scanner, result.diagnostics};
    result.program = parser.parse();
    return result;
}

} // namespace

TEST_CASE("Parser parses a subroutine and its parameters", "[parser]")
{
    auto result = parse("SUB Add(x AS REAL, values[] AS TEXT) AS REAL\n"
                        "RETURN x\n"
                        "END SUB\n");

    REQUIRE(result.diagnostics.count() == 0);
    REQUIRE(result.program->_subroutines.size() == 1);
    const auto& subroutine = result.program->_subroutines.front();
    CHECK(subroutine->_name == "Add");
    REQUIRE(subroutine->_parameters.size() == 2);
    REQUIRE(subroutine->_parameters[0]->_type->kind == NodeKind::ScalarType);
    const auto& scalar = static_cast<const ScalarType&>(
        *subroutine->_parameters[0]->_type);
    CHECK(scalar._name == ScalarType::Name::Real);
    REQUIRE(subroutine->_parameters[1]->_type->kind == NodeKind::ArrayType);
    const auto& array = static_cast<const ArrayType&>(
        *subroutine->_parameters[1]->_type);
    CHECK(array._base->_name == ScalarType::Name::Text);
    CHECK(array.isOpen());
    REQUIRE(subroutine->_returnType);
    CHECK(subroutine->_returnType->_name == ScalarType::Name::Real);
    REQUIRE(subroutine->_body->_items.size() == 1);
    REQUIRE(subroutine->_body->_items.front()->kind == NodeKind::Return);
    const auto& returnStatement =
        static_cast<const Return&>(*subroutine->_body->_items.front());
    CHECK(returnStatement._value->kind == NodeKind::Variable);
}

TEST_CASE("Parser requires a return value expression", "[parser]")
{
    const auto result = parse("SUB Value AS REAL\nRETURN\nEND SUB\n");
    CHECK(result.diagnostics.count() > 0);
}

TEST_CASE("Parser preserves expression precedence", "[parser]")
{
    auto result = parse("SUB Main\n"
                        "LET x = 1 + 2 * 3 OR FALSE\n"
                        "END SUB\n");

    REQUIRE(result.diagnostics.count() == 0);
    const auto& assignment = result.program->_subroutines.front()->_body->_items.front();
    const auto& let = static_cast<const Let&>(*assignment);
    const auto& disjunction = static_cast<const Binary&>(*let._value);
    CHECK(disjunction._operation == Operation::Or);
    const auto& addition = static_cast<const Binary&>(*disjunction._left);
    CHECK(addition._operation == Operation::Add);
    const auto& multiplication = static_cast<const Binary&>(*addition._right);
    CHECK(multiplication._operation == Operation::Mul);
}

TEST_CASE("Parser handles indexed assignments and control statements", "[parser]")
{
    auto result = parse("SUB Main\n"
                        "DIM values[10] AS REAL\n"
                        "LET values[1] = 2\n"
                        "IF TRUE THEN\n"
                        "CALL Print values[1]\n"
                        "ELSE\n"
                        "LET values[1] = 0\n"
                        "END IF\n"
                        "END SUB\n");

    REQUIRE(result.diagnostics.count() == 0);
    const auto& body = result.program->_subroutines.front()->_body->_items;
    REQUIRE(body.size() == 3);
    CHECK(body[0]->kind == NodeKind::Dim);
    CHECK(body[1]->kind == NodeKind::Let);
    CHECK(body[2]->kind == NodeKind::If);
}

TEST_CASE("Parser reports malformed input", "[parser]")
{
    const auto result = parse("SUB Main\nLET x =\nEND SUB\n");
    CHECK(result.diagnostics.count() > 0);
}

TEST_CASE("Parser omits declarations without a type", "[parser]")
{
    const auto result = parse("SUB Main(value AS)\n"
                              "DIM item AS\n"
                              "END SUB\n");

    CHECK(result.diagnostics.count() == 2);
    REQUIRE(result.program->_subroutines.size() == 1);
    const auto& main = result.program->_subroutines.front();
    CHECK(main->_parameters.empty());
    CHECK(main->_body->_items.empty());
}

TEST_CASE("All bundled examples follow the Cherry grammar", "[parser][examples]")
{
    const auto examples = std::filesystem::path{AVIUM_SOURCE_DIR} / "examples";
    REQUIRE(std::filesystem::exists(examples));

    std::size_t count = 0;
    for( const auto& entry : std::filesystem::directory_iterator(examples) ) {
        if( entry.path().extension() != ".bas" )
            continue;

        std::ifstream input{entry.path()};
        REQUIRE(input.good());
        std::ostringstream source;
        source << input.rdbuf();
        const auto result = parse(source.str());
        INFO("Example: " << entry.path().filename().string());
        CHECK(result.diagnostics.count() == 0);
        ++count;
    }
    CHECK(count == 20);
}
