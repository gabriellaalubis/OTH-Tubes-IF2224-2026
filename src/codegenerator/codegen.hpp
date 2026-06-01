#ifndef CODEGEN_HPP
#define CODEGEN_HPP

#include "../semantic/ast.hpp"
#include "../semantic/symbol_table.hpp"
#include <string>
#include <vector>
#include <ostream>
#include <stdexcept>

enum class OpCode {
    LIT,  
    LOD, 
    STO,  
    CAL,  
    INT, 
    JMP, 
    JPC, 
    OPR,  
    RET,  
};

std::string opCodeToString(OpCode op);

struct Instruction {
    int line;      
    OpCode op;      
    int level;    
    int operand;   

    Instruction(int ln, OpCode o, int lv, int oper) : line(ln), op(o), level(lv), operand(oper) {}
};

class CodeGenError : public std::runtime_error {
public:
    explicit CodeGenError(const std::string& msg) : std::runtime_error("[CODEGEN ERROR] " + msg) {}
};

class CodeGenerator {
public:
    CodeGenerator(const ASTPtr& ast, const SymbolTable& symtab);
    void generate();
    const std::vector<Instruction>& getInstructions() const { return code_; }
    const std::vector<std::string>& getStringTable() const { return stringTable_; }
    void printCode(std::ostream& out) const;

private:
    const ASTPtr& ast_;     
    const SymbolTable& symtab_;  
    std::vector<Instruction> code_; 
    std::vector<std::string> stringTable_;
    int curLev_; 

    int  emit(OpCode op, int level, int operand);
    void patch(int instrIndex, int newOperand);
    int nextLine() const { return static_cast<int>(code_.size()); }
    int varAddress(int tabIdx) const;
    int levelDiff(int tabIdx) const;
    int frameSize(int btabIdx) const;
    void genProgram(const ASTPtr& node);
    void genStatement(const ASTPtr& node);
    void genCompound(const ASTPtr& node);
    void genAssign(const ASTPtr& node);
    void genIfStmt (const ASTPtr& node);
    void genWhileStmt (const ASTPtr& node);
    void genForStmt(const ASTPtr& node);
    void genRepeatStmt(const ASTPtr& node);
    void genCaseStmt(const ASTPtr& node);
    void genProcCall(const ASTPtr& node);
    void genExpr(const ASTPtr& node);
    void genBinOp(const ASTPtr& node);
    void genUnOp(const ASTPtr& node);
    void genVarRef(const ASTPtr& node);
    void genLiteral (const ASTPtr& node);
    void genFuncCall(const ASTPtr& node);
    void genProcDecl(const ASTPtr& node);
    void genFuncDecl(const ASTPtr& node);
    int  opToOprNum(const std::string& op, DataType leftType) const;
};

#endif