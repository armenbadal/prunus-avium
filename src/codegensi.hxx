#pragma once

#include "ast.hxx"
#include "astvisitor.hxx"
#include "semantic.hxx"
#include "symbols.hxx"

#include <filesystem>
#include <sstream>

namespace avium {

class CodeGeneratorSi : public ASTVisitor<CodeGeneratorSi> {
public:
    CodeGeneratorSi();
    ~CodeGeneratorSi();

    using ASTVisitor<CodeGeneratorSi>::visit;

    bool generate(Program& program, const SymbolTable& symbols, const SemanticModel& model);
    void save(std::filesystem::path p);

public:
    void visit(Program& p);
    void visit(Subroutine& s);
    void visit(Sequence& q);
    void visit(Dim& d);
    void visit(Let&);
    void visit(If&);
    void visit(IfBranch&);
    void visit(While&);
    void visit(For&);
    void visit(Call&);
    void visit(Return&);
    void visit(Apply&);
    void visit(ScalarType&);
    void visit(ArrayType&);
    void visit(Binary&);
    void visit(Unary&);
    void visit(Variable&);
    void visit(Text&);
    void visit(Number&);
    void visit(Boolean&);

private:
    std::ostringstream _out;
    const SemanticModel* _model{};
};

} // namespace avium
