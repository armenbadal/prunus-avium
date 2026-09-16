#include <catch2/catch_test_macros.hpp>

#include "codegensi.hxx"
#include "diagnostics.hxx"
#include "parser.hxx"
#include "semantic.hxx"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <string_view>

using namespace avium;

namespace {

std::string generate(std::string_view source)
{
    std::istringstream input{std::string{source}};
    Scanner scanner{input};
    Diagnostics diagnostics;
    Parser parser{scanner, diagnostics};
    auto program = parser.parse();
    REQUIRE(diagnostics.count() == 0);

    SymbolTable symbols;
    SemanticModel model;
    SemanticAnalyzer analyzer{symbols, model, diagnostics};
    REQUIRE(analyzer.analyze(*program));

    static unsigned index = 0;
    const auto path = std::filesystem::temp_directory_path() / ("prunus-codegensi-test-" + std::to_string(++index) + ".c");
    CodeGeneratorSi generator{*program, model};
    REQUIRE(generator.generate(path));

    std::ifstream generated{path};
    REQUIRE(generated);
    const std::string result{std::istreambuf_iterator<char>{generated}, {}};
    generated.close();
    std::error_code error;
    std::filesystem::remove(path, error);
    return result;
}

} // namespace

TEST_CASE("C generator initializes scalar declarations", "[codegensi]")
{
    const auto output = generate(
        "SUB Main\n"
        "DIM number AS REAL\n"
        "DIM message AS TEXT\n"
        "DIM ready AS BOOL\n"
        "LET message = \"\"\n"
        "END SUB\n");

    CHECK(output.contains("#include \"arrays.h\""));
    CHECK(output.contains("#include \"io.h\""));
    CHECK(output.contains("#include \"texts.h\""));
    CHECK(output.contains("double number = 0.0;"));
    CHECK(output.contains(
        "static const avium_text avium_empty_text = {\"\", 0, false};"));
    CHECK(output.contains("avium_text message = avium_empty_text;"));
    CHECK(output.contains("bool ready = false;"));
    CHECK(output.contains("message = avium_empty_text;"));
}

TEST_CASE("C generator emits reusable escaped text values", "[codegensi]")
{
    const auto output = generate(
        "SUB Main\n"
        "DIM values[2] AS TEXT\n"
        "LET values[0] = \"a\\n\"\n"
        "LET values[1] = \"a\\n\"\n"
        "END SUB\n");

    CHECK(output.find("avium_text_create") == std::string::npos);
    const std::string declarationPrefix = "static const avium_text ";
    const std::string initializer = " = {\"a\\\\n\", 3, false};";
    const auto initializerPosition = output.find(initializer);
    REQUIRE(initializerPosition != std::string::npos);
    const auto declaration = output.rfind(declarationPrefix, initializerPosition);
    REQUIRE(declaration != std::string::npos);
    const auto namePosition = declaration + declarationPrefix.size();
    const auto literal = output.substr(namePosition, initializerPosition - namePosition);
    CHECK(literal.starts_with("avium_temp_text_"));
    const auto firstUse = output.find(literal, initializerPosition + initializer.size());
    REQUIRE(firstUse != std::string::npos);
    CHECK(output.find(literal, firstUse + literal.size()) != std::string::npos);
}

TEST_CASE("C generator preserves UTF-8 text literals", "[codegensi]")
{
    const auto output = generate(
        "SUB Main\n"
        "DIM message AS TEXT\n"
        "LET message = \"տեքստ\"\n"
        "END SUB\n");

    CHECK(output.contains(" = {\"տեքստ\", 10, false};"));
}

TEST_CASE("C generator mangles user function calls", "[codegensi]")
{
    const auto output = generate(
        "SUB Value AS REAL\n"
        "RETURN 1\n"
        "END SUB\n"
        "SUB Main\n"
        "DIM result AS REAL\n"
        "LET result = Value()\n"
        "END SUB\n");

    CHECK(output.contains("double avium_Value("));
    CHECK(output.contains("result = avium_Value();"));
}
