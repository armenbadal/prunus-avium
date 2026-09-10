#include "astlisp.hxx"
#include "parser.hxx"
#include "semantic.hxx"
#include "codegensi.hxx"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace {

void printDiagnostics(const std::filesystem::path& source,
    const avium::Diagnostics& diagnostics)
{
    const auto sourceName = source.string();
    for( const auto& [line, message] : diagnostics.errors() )
        std::cerr << sourceName << ':' << line << ": " << message << '\n';

    const auto storedCount = diagnostics.errors().size();
    const auto omittedCount = diagnostics.count() - storedCount;
    if( omittedCount != 0 )
        std::cerr << sourceName << ": ... և ևս " << omittedCount << " սխալ։\n";
}

} // namespace

int main(int argc, char* argv[])
{
    if( argc != 2 ) {
        const auto executable = argc == 0 ? "prunus" : argv[0];
        std::cerr << "Օգտագործում՝ " << executable << " <ֆայլ>\n";
        return EXIT_FAILURE;
    }

    const std::filesystem::path source{argv[1]};
    std::ifstream input{source};
    if( !input ) {
        std::cerr << source.string() << ": ֆայլը բացել չհաջողվեց։\n";
        return EXIT_FAILURE;
    }

    avium::Scanner scanner{input};
    avium::Diagnostics diagnostics;
    avium::Parser parser{scanner, diagnostics};
    auto program = parser.parse();

    if( diagnostics.count() != 0 ) {
        printDiagnostics(source, diagnostics);
        return EXIT_FAILURE;
    }

    avium::SymbolTable symbols;
    avium::SemanticModel model;
    avium::SemanticAnalyzer analyzer{symbols, model, diagnostics};
    if( !analyzer.analyze(*program) ) {
        printDiagnostics(source, diagnostics);
        return EXIT_FAILURE;
    }

    ///avium::AstLisp{}.emit(*program, std::cout);
    avium::CodeGeneratorSi codegen;
    codegen.generate(*program, symbols, model);
    codegen.save("test-output.c");

    return EXIT_SUCCESS;
}
