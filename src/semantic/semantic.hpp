#ifndef SEMANTIC_HPP
#define SEMANTIC_HPP

#include "../parser/parser.hpp"
#include "ast.hpp"
#include "symbol_table.hpp"
#include <string>
#include <stdexcept>
#include <ostream>

class SemanticError : public std::runtime_error {
public:
    explicit SemanticError(const std::string& msg)
        : std::runtime_error(msg) {}
};

struct SemanticWarning {
    std::string message;
    explicit SemanticWarning(const std::string& msg) : message(msg) {}
};


class SemanticAnalyser {
public:
    SemanticAnalyser();

    // Entry point utama
    ASTPtr analyse(const NodePtr& parseRoot);

    // Akses hasil
    const ASTPtr& getAST() const {return ast_;}
    const SymbolTable& getSymbolTable() const {return symtab_;}
    const std::vector<SemanticWarning>& getWarnings() const {return warnings_;}

    // Print hasil ke ostream
    void printResults(std::ostream& out) const;

private:
    ASTPtr       ast_;
    SymbolTable  symtab_;
    std::vector<SemanticWarning> warnings_;

    // Syntax-Directed Translation
    ASTPtr buildAST         (const NodePtr& n);
    ASTPtr buildProgram     (const NodePtr& n);
    ASTPtr buildDeclPart    (const NodePtr& n);
    ASTPtr buildConstDecl   (const NodePtr& n);
    ASTPtr buildVarDecl     (const NodePtr& n);
    ASTPtr buildTypeDecl    (const NodePtr& n);
    ASTPtr buildSubprogDecl (const NodePtr& n);
    ASTPtr buildProcDecl    (const NodePtr& n);
    ASTPtr buildFuncDecl    (const NodePtr& n);
    ASTPtr buildBlock       (const NodePtr& n);
    ASTPtr buildFormalParams(const NodePtr& n);
    ASTPtr buildParamGroup  (const NodePtr& n);

    ASTPtr buildType        (const NodePtr& n);
    ASTPtr buildArrayType   (const NodePtr& n);
    ASTPtr buildRecordType  (const NodePtr& n);
    ASTPtr buildRange       (const NodePtr& n);
    ASTPtr buildEnumerated  (const NodePtr& n);
    ASTPtr buildFieldList   (const NodePtr& n);
    ASTPtr buildFieldPart   (const NodePtr& n);

    ASTPtr buildCompound    (const NodePtr& n);
    ASTPtr buildStmtList    (const NodePtr& n);
    ASTPtr buildStatement   (const NodePtr& n);
    ASTPtr buildAssign      (const NodePtr& n);
    ASTPtr buildIfStmt      (const NodePtr& n);
    ASTPtr buildWhileStmt   (const NodePtr& n);
    ASTPtr buildForStmt     (const NodePtr& n);
    ASTPtr buildRepeatStmt  (const NodePtr& n);
    ASTPtr buildCaseStmt    (const NodePtr& n);
    ASTPtr buildCaseBlock   (const NodePtr& n);
    ASTPtr buildProcCall    (const NodePtr& n);

    ASTPtr buildExpression  (const NodePtr& n);
    ASTPtr buildSimpleExpr  (const NodePtr& n);
    ASTPtr buildTerm        (const NodePtr& n);
    ASTPtr buildFactor      (const NodePtr& n);
    ASTPtr buildVariable    (const NodePtr& n);
    ASTPtr buildConstant    (const NodePtr& n);
    ASTPtr buildIdentList   (const NodePtr& n);

    const NodePtr& childAt(const NodePtr& n, size_t idx) const;
    std::string    labelOf(const NodePtr& n) const;
    std::string    valueOf(const NodePtr& n) const; 
    bool           labelStartsWith(const NodePtr& n, const std::string& prefix) const;
    bool           labelIs(const NodePtr& n, const std::string& exact) const;

    // Visit functions
    void visitProgram   (const ASTPtr& n);
    void visitBlock     (const ASTPtr& n);
    void visitConstDecl (const ASTPtr& n);
    void visitVarDecl   (const ASTPtr& n);
    void visitTypeDecl  (const ASTPtr& n);
    void visitProcDecl  (const ASTPtr& n);
    void visitFuncDecl  (const ASTPtr& n);

    void visitStatement (const ASTPtr& n);
    void visitCompound  (const ASTPtr& n);
    void visitAssign    (const ASTPtr& n);
    void visitIfStmt    (const ASTPtr& n);
    void visitWhileStmt (const ASTPtr& n);
    void visitForStmt   (const ASTPtr& n);
    void visitRepeatStmt(const ASTPtr& n);
    void visitCaseStmt  (const ASTPtr& n);
    void visitCaseBlock  (const ASTPtr& n);
    void visitProcCall  (const ASTPtr& n);

    DataType visitExpr      (const ASTPtr& n);
    DataType visitBinOp     (const ASTPtr& n);
    DataType visitUnOp      (const ASTPtr& n);
    DataType visitVarRef    (const ASTPtr& n);
    DataType visitFuncCall  (const ASTPtr& n);
    DataType visitArrayAccess(const ASTPtr& n);
    DataType visitFieldAccess(const ASTPtr& n);

    DataType resolveTypeName(const std::string& name) const;
    DataType resolveTypeNode(const ASTPtr& typeNode);
    bool     isCompatible           (DataType t1, DataType t2) const;
    bool     isAssignmentCompatible (DataType target, DataType value) const;
    bool     isNumeric              (DataType dt) const;
    bool     isOrdinal              (DataType dt) const;
    DataType resultTypeOfBinOp      (const std::string& op, DataType l, DataType r) const;

    // Error warning
    void semanticError  (const std::string& msg) const;
    void warn           (const std::string& msg);
};

#endif