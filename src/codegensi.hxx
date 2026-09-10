#pragma once

#include "ast.hxx"
#include "semantic.hxx"
#include "symbols.hxx"

#include <filesystem>
#include <sstream>

namespace avium {

class CodeGeneratorSi : public ASTVisitor<CodeGeneratorSi> {
public:
    CodeGeneratorSi();
    ~CodeGeneratorSi();

    bool generate(Program& program, const SymbolTable& symbols, const SemanticModel& model);
    void save(std::filesystem::path p);

private:
    void visit(Program& p);
    void visit(Subroutine& s);

    std::ostringstream _out;
};

} // namespace avium
