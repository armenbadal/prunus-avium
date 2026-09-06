#pragma once

#include "ast.hxx"
#include "astvisitor.hxx"
#include "diagnostics.hxx"
#include "symbols.hxx"

#include <optional>
#include <unordered_map>

namespace avium {

// AST հանգույցների semantic կապերը՝ առանց AST-ը փոփոխելու։
class SemanticModel {
public:
    void bind(NodeId node, SymbolId symbol);
    void setType(NodeId node, TypeName type);

    std::optional<SymbolId> symbol(NodeId node) const;
    std::optional<TypeName> type(NodeId node) const;

private:
    std::unordered_map<NodeId, SymbolId> _symbols;
    std::unordered_map<NodeId, TypeName> _types;
};

class SemanticAnalyzer : public ASTVisitor<SemanticAnalyzer> {
public:
    SemanticAnalyzer(SymbolTable& symbols, SemanticModel& model, Diagnostics& diagnostics);

    bool analyze(Program& program);

    using ASTVisitor<SemanticAnalyzer>::visit;

    void visit(Program& node);
    void visit(Subroutine& node);
    void visit(Sequence& node);
    void visit(Dim& node);
    void visit(Let& node);
    void visit(If& node);
    void visit(IfBranch& node);
    void visit(While& node);
    void visit(For& node);
    void visit(Call& node);

    void visit(Boolean& node);
    void visit(Number& node);
    void visit(Text& node);
    void visit(Variable& node);
    void visit(Unary& node);
    void visit(Binary& node);
    void visit(Apply& node);

private:
    void declareBuiltins();
    void declareSubroutines(const Program& program);
    void analyzeSubroutine(Subroutine& subroutine);
    void declareParameters(const Subroutine& subroutine);
    void declareReturnValue(const Subroutine& subroutine);
    void declareLocals(const Sequence& sequence);
    void declareDim(const Dim& dim);
    void declareForVariable(const For& loop);

    std::optional<SymbolId> resolveVariable(const Variable& variable);
    std::optional<SymbolId> resolveSubroutine(const Node& node, std::string_view name);
    void validateArguments(const Node& node, std::string_view name, const std::vector<Expression::Ptr>& arguments, const SubroutineSignature& signature);
    TypeName expressionType(const Expression::Ptr& expression);
    bool isArrayExpression(const Expression& expression) const;
    bool requireScalar(const Expression& expression);
    void validateIndex(const Expression::Ptr& index);
    ParameterInfo parameterInfo(const Dim& parameter) const;

    void requireType(const Node& node, TypeName actual, TypeName expected);
    void report(const Node& node, std::string_view message);

    SymbolTable& _symbols;
    SemanticModel& _model;
    Diagnostics& _diagnostics;
};

} // namespace avium
