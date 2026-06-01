#include "codegen.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <stdexcept>


std::string opCodeToString(OpCode op) {
    switch (op) {
        case OpCode::LIT: return "LIT";
        case OpCode::LOD: return "LOD";
        case OpCode::STO: return "STO";
        case OpCode::CAL: return "CAL";
        case OpCode::INT: return "INT";
        case OpCode::JMP: return "JMP";
        case OpCode::JPC: return "JPC";
        case OpCode::OPR: return "OPR";
        case OpCode::RET: return "RET";
        default: return "???";
    }
}


CodeGenerator::CodeGenerator(const ASTPtr& ast, const SymbolTable& symtab) : ast_(ast), symtab_(symtab), curLev_(0){}


void CodeGenerator::generate() {
    code_.clear();
    if (!ast_) throw CodeGenError("AST kosong, tidak bisa generate kode.");
    if (ast_->kind != ASTNodeKind::PROGRAM)
        throw CodeGenError("Root AST bukan PROGRAM node.");
    genProgram(ast_);
}

void CodeGenerator::printCode(std::ostream& out) const {
    out << "\n";
    out << "INTERMEDIATE CODE (STACK MACHINE)\n";
    for (const auto& instr : code_) {
        out << std::right << std::setw(4) << instr.line
            << "  " << std::left  << std::setw(5) << opCodeToString(instr.op)
            << std::right << std::setw(2) << instr.level
            << "  " << std::setw(5) << instr.operand
            << "\n";
    }
}


int CodeGenerator::emit(OpCode op, int level, int operand) {
    int ln = nextLine();
    code_.emplace_back(ln, op, level, operand);
    return ln;
}

void CodeGenerator::patch(int instrIndex, int newOperand) {
    if (instrIndex < 0 || instrIndex >= (int)code_.size())
        throw CodeGenError("patch: index instruksi di luar batas (" + std::to_string(instrIndex) + ")");
    code_[instrIndex].operand = newOperand;
}


int CodeGenerator::varAddress(int tabIdx) const {
    if (tabIdx < 0 || tabIdx >= (int)symtab_.tab().size())
        throw CodeGenError("varAddress: tabIdx tidak valid (" + std::to_string(tabIdx) + ")");

    const TabEntry& entry = symtab_.tab()[tabIdx];

    if (entry.obj == ObjClass::PARAMETER) {
        return 3 + entry.adr;
    }

    int psze = 0;
    for (int b = 0; b < (int)symtab_.btab().size(); ++b) {
        int cur = symtab_.btab()[b].last;
        bool found = false;
        while (cur > 0) {
            if (cur == tabIdx) {
                found = true; 
                break; 
            }
            cur = symtab_.tab()[cur].link;
        }
        if (found) {
            psze = symtab_.btab()[b].psze;
            break;
        }
    }
    return 3 + psze + entry.adr;
}

int CodeGenerator::levelDiff(int tabIdx) const {
    if (tabIdx < 0 || tabIdx >= (int)symtab_.tab().size()) return 0;
    return curLev_ - symtab_.tab()[tabIdx].lev;
}

int CodeGenerator::frameSize(int btabIdx) const {
    if (btabIdx < 0 || btabIdx >= (int)symtab_.btab().size())
        throw CodeGenError("frameSize: btabIdx tidak valid (" +
                           std::to_string(btabIdx) + ")");
    const auto& b = symtab_.btab()[btabIdx];
    return 3 + b.psze + b.vsze;
}



int CodeGenerator::opToOprNum(const std::string& op, DataType /*leftType*/) const {
    if (op == "+")   return 2; 
    if (op == "-")   return 3; 
    if (op == "*")   return 4;  
    if (op == "/")   return 5;  
    if (op == "div") return 5;  
    if (op == "mod") return 6;  

    if (op == "=")   return 7;  
    if (op == "<>")  return 8;  
    if (op == "<")   return 9;  
    if (op == ">=")  return 10; 
    if (op == ">")   return 11; 
    if (op == "<=")  return 12; 

    if (op == "and") return 4;  
    if (op == "or")  return 2;  
    throw CodeGenError("opToOprNum: operator tidak dikenal '" + op + "'");
}


void CodeGenerator::genProgram(const ASTPtr& node) {
    curLev_ = 0;
    int fsize = frameSize(0); 
    emit(OpCode::INT, 0, fsize);
    ASTPtr declBlock = nullptr;
    ASTPtr bodyBlock = nullptr;

    for (const auto& child : node->children) {
        if (child->kind == ASTNodeKind::BLOCK && child->name == "declarations") {
            declBlock = child;
        } else if (child->kind == ASTNodeKind::BLOCK || child->kind == ASTNodeKind::COMPOUND) {
            bodyBlock = child;
        }
    }

    bool hasSubprog = false;
    if (declBlock) {
        for (const auto& d : declBlock->children) {
            if (d->kind == ASTNodeKind::PROC_DECL || d->kind == ASTNodeKind::FUNC_DECL) {
                hasSubprog = true;
                break;
            }
        }
    }

    // JMP ke body 
    int jmpToBody = -1;
    if (hasSubprog) {
        jmpToBody = emit(OpCode::JMP, 0, 0); // target di-patch nanti
    }

    // generate deklarasi subprogram (proc/func)
    if (declBlock) {
        for (const auto& d : declBlock->children) {
            if (d->kind == ASTNodeKind::PROC_DECL) genProcDecl(d);
            else if (d->kind == ASTNodeKind::FUNC_DECL) genFuncDecl(d);
            // VarDecl tidak menghasilkan instruksi (sudah di-handle oleh INT)
        }
    }

    // JMP ke posisi body sekarang
    if (jmpToBody >= 0) {
        patch(jmpToBody, nextLine());
    }

    //generate body program utama
    if (bodyBlock) {
        genCompound(bodyBlock);
    }
    emit(OpCode::RET, 0, 0);
}

void CodeGenerator::genCompound(const ASTPtr& node) {
    for (const auto& child : node->children) {
        genStatement(child);
    }
}

void CodeGenerator::genStatement(const ASTPtr& node) {
    if (!node) return;

    switch (node->kind) {
        case ASTNodeKind::ASSIGN:
            genAssign(node);
            break;

        case ASTNodeKind::IF_STMT:
            genIfStmt(node);
            break;

        case ASTNodeKind::WHILE_STMT:
            genWhileStmt(node);
            break;

        case ASTNodeKind::FOR_STMT:
            genForStmt(node);
            break;

        case ASTNodeKind::REPEAT_STMT:
            genRepeatStmt(node);
            break;

        case ASTNodeKind::PROC_CALL:
            genProcCall(node);
            break;

        case ASTNodeKind::CASE_STMT:
            genCaseStmt(node);
            break;

        case ASTNodeKind::COMPOUND:
        case ASTNodeKind::BLOCK:
            genCompound(node);
            break;

        case ASTNodeKind::EMPTY_STMT:
            break;

        default:
            break;
    }
}

void CodeGenerator::genAssign(const ASTPtr& node) {
    if (node->children.size() < 2)
        throw CodeGenError("genAssign: node ASSIGN tidak memiliki 2 anak.");

    const ASTPtr& target = node->children[0]; // VAR_REF target
    const ASTPtr& rhs    = node->children[1]; // ekspresi kanan
    genExpr(rhs);

    if (target->kind == ASTNodeKind::VAR_REF) {
        int tabIdx = target->tabIndex;
        if (tabIdx < 0)
            throw CodeGenError("genAssign: tabIndex variabel '" + target->name + "' tidak valid.");
        const TabEntry& entry = symtab_.tab()[tabIdx];
        if (entry.obj == ObjClass::FUNCTION) {
            emit(OpCode::STO, 0, 0);
        } else {
            int addr = varAddress(tabIdx);
            int lev  = levelDiff(tabIdx);
            emit(OpCode::STO, lev, addr);
        }
    } else if (target->kind == ASTNodeKind::FIELD_ACCESS) {
        if (!target->children.empty() &&
            target->children[0]->kind == ASTNodeKind::VAR_REF) {
            int recTabIdx   = target->children[0]->tabIndex;
            int fieldTabIdx = target->tabIndex;
            int baseAddr = varAddress(recTabIdx);
            int fieldOff = (fieldTabIdx >= 0) ? symtab_.tab()[fieldTabIdx].adr : 0;
            int lev = levelDiff(recTabIdx);
            emit(OpCode::STO, lev, baseAddr + fieldOff);
        }
    } else if (target->kind == ASTNodeKind::ARRAY_ACCESS) {
        if (!target->children.empty() &&
            target->children[0]->kind == ASTNodeKind::VAR_REF) {
            int tabIdx = target->children[0]->tabIndex;
            int addr = varAddress(tabIdx);
            int lev = levelDiff(tabIdx);
            emit(OpCode::STO, lev, addr);
        }
    } else {
        throw CodeGenError("genAssign: target assignment bukan VAR_REF.");
    }
}

void CodeGenerator::genIfStmt(const ASTPtr& node) {
    if (node->children.empty())
        throw CodeGenError("genIfStmt: node IF_STMT tidak memiliki anak.");

    const ASTPtr& cond = node->children[0];
    ASTPtr thenBlock = (node->children.size() > 1) ? node->children[1] : nullptr;
    ASTPtr elseBlock = (node->children.size() > 2) ? node->children[2] : nullptr;

    genExpr(cond);
    int jpcIdx = emit(OpCode::JPC, 0, 0); 
    if (thenBlock) genStatement(thenBlock);

    if (elseBlock) {
        int jmpIdx = emit(OpCode::JMP, 0, 0); 
        patch(jpcIdx, nextLine());
        genStatement(elseBlock);
        patch(jmpIdx, nextLine());
    } else {
        patch(jpcIdx, nextLine());
    }
}


void CodeGenerator::genWhileStmt(const ASTPtr& node) {
    if (node->children.size() < 2)
        throw CodeGenError("genWhileStmt: node WHILE_STMT butuh 2 anak.");

    const ASTPtr& cond = node->children[0];
    const ASTPtr& body = node->children[1];
    int loopStart = nextLine();
    genExpr(cond);
    int jpcIdx = emit(OpCode::JPC, 0, 0); 
    genStatement(body);
    emit(OpCode::JMP, 0, loopStart);
    patch(jpcIdx, nextLine());
}

void CodeGenerator::genForStmt(const ASTPtr& node) {
    if (node->children.size() < 4)
        throw CodeGenError("genForStmt: node FOR_STMT butuh 4 anak.");

    const ASTPtr& varRef = node->children[0]; 
    const ASTPtr& startExpr= node->children[1]; 
    const ASTPtr& endExpr  = node->children[2]; 
    const ASTPtr& body = node->children[3]; 
    bool isDownto = (node->name == "downto");

    if (varRef->kind != ASTNodeKind::VAR_REF || varRef->tabIndex < 0)
        throw CodeGenError("genForStmt: variabel kontrol FOR tidak valid.");

    int addr = varAddress(varRef->tabIndex);
    int lev  = levelDiff(varRef->tabIndex);

    genExpr(startExpr);
    emit(OpCode::STO, lev, addr);
    int loopStart = nextLine();
    emit(OpCode::LOD, lev, addr);   
    genExpr(endExpr);               
    emit(OpCode::OPR, 0, isDownto ? 10 : 12);
    int jpcIdx = emit(OpCode::JPC, 0, 0);
    genStatement(body);

    emit(OpCode::LOD, lev, addr);
    emit(OpCode::LIT, 0, 1);
    emit(OpCode::OPR, 0, isDownto ? 3 : 2); 
    emit(OpCode::STO, lev, addr);

    emit(OpCode::JMP, 0, loopStart);
    patch(jpcIdx, nextLine());
}

static bool isStatementKind(ASTNodeKind k) {
    switch (k) {
        case ASTNodeKind::ASSIGN:
        case ASTNodeKind::IF_STMT:
        case ASTNodeKind::WHILE_STMT:
        case ASTNodeKind::FOR_STMT:
        case ASTNodeKind::REPEAT_STMT:
        case ASTNodeKind::CASE_STMT:
        case ASTNodeKind::PROC_CALL:
        case ASTNodeKind::COMPOUND:
        case ASTNodeKind::BLOCK:
        case ASTNodeKind::EMPTY_STMT:
            return true;
        default:
            return false;
    }
}

void CodeGenerator::genRepeatStmt(const ASTPtr& node) {
    if (node->children.empty())
        throw CodeGenError("genRepeatStmt: node REPEAT_STMT tidak memiliki anak.");
    size_t condIdx = node->children.size() - 1;
    int loopStart = nextLine();
    for (size_t i = 0; i < condIdx; ++i) {
        genStatement(node->children[i]);
    }
    genExpr(node->children[condIdx]);
    emit(OpCode::JPC, 0, loopStart);
}

void CodeGenerator::genCaseStmt(const ASTPtr& node) {
    if (node->children.size() < 2)
        throw CodeGenError("genCaseStmt: CASE_STMT butuh selector dan blok.");

    const ASTPtr& selector = node->children[0];

    std::vector<int> jmpEndIdxs;

    const ASTPtr* caseBlock = &node->children[1];
    while (caseBlock && *caseBlock &&
           (*caseBlock)->kind == ASTNodeKind::CASE_BLOCK) {
        const ASTPtr& blk = *caseBlock;

        genExpr(selector);
        genExpr(blk->children[0]);
        emit(OpCode::OPR, 0, 7);
        int jpcIdx = emit(OpCode::JPC, 0, 0);

        if (blk->children.size() >= 2)
            genStatement(blk->children[1]);

        jmpEndIdxs.push_back(emit(OpCode::JMP, 0, 0));
        patch(jpcIdx, nextLine());

        if (blk->children.size() >= 3)
            caseBlock = &blk->children[2];
        else
            break;
    }

    int endAddr = nextLine();
    for (int idx : jmpEndIdxs)
        patch(idx, endAddr);
}

void CodeGenerator::genProcCall(const ASTPtr& node) {
    std::string name = node->name;
    if (name == "writeln") {
        if (node->children.empty()) {
            int idx = (int)stringTable_.size();
            stringTable_.push_back("");
            emit(OpCode::LIT, 1, idx);
            emit(OpCode::OPR, 0, 14);
        } else {
            for (size_t i = 0; i < node->children.size(); ++i) {
                genExpr(node->children[i]);
                if (i == node->children.size() - 1)
                    emit(OpCode::OPR, 0, 14); 
                else
                    emit(OpCode::OPR, 0, 13); 
            }
        }
        return;
    }

    if (name == "write") {
        for (const auto& arg : node->children) {
            genExpr(arg);
            emit(OpCode::OPR, 0, 13); // WRT tanpa newline
        }
        return;
    }

    if (name == "readln" || name == "read") {
        return;
    }

    emit(OpCode::LIT, 0, 0);
    emit(OpCode::LIT, 0, 0);
    emit(OpCode::LIT, 0, 0);

    for (const auto& arg : node->children) {
        genExpr(arg);
    }

    int tabIdx = node->tabIndex;
    if (tabIdx < 0)
        throw CodeGenError("genProcCall: tabIndex prosedur '" + name + "' tidak valid.");

    int procAddr = symtab_.tab()[tabIdx].adr;
    int lev      = levelDiff(tabIdx);
    emit(OpCode::CAL, lev, procAddr);
}

void CodeGenerator::genExpr(const ASTPtr& node) {
    if (!node) return;

    switch (node->kind) {
        case ASTNodeKind::BINOP:
            genBinOp(node);
            break;

        case ASTNodeKind::UNOP:
            genUnOp(node);
            break;

        case ASTNodeKind::VAR_REF:
            genVarRef(node);
            break;

        case ASTNodeKind::CONST_INT:
        case ASTNodeKind::CONST_REAL:
        case ASTNodeKind::CONST_CHAR:
        case ASTNodeKind::CONST_STRING:
        case ASTNodeKind::CONST_BOOL:
            genLiteral(node);
            break;

        case ASTNodeKind::FUNC_CALL:
            genFuncCall(node);
            break;

        case ASTNodeKind::ARRAY_ACCESS:
            if (!node->children.empty() &&
                node->children[0]->kind == ASTNodeKind::VAR_REF) {
                int tabIdx = node->children[0]->tabIndex;
                int addr   = varAddress(tabIdx);
                int lev    = levelDiff(tabIdx);
                emit(OpCode::LOD, lev, addr);
            }
            break;

        case ASTNodeKind::FIELD_ACCESS:
            if (!node->children.empty() &&
                node->children[0]->kind == ASTNodeKind::VAR_REF) {
                int recTabIdx   = node->children[0]->tabIndex;
                int fieldTabIdx = node->tabIndex;
                int baseAddr = varAddress(recTabIdx);
                int fieldOff = (fieldTabIdx >= 0) ? symtab_.tab()[fieldTabIdx].adr : 0;
                int lev = levelDiff(recTabIdx);
                emit(OpCode::LOD, lev, baseAddr + fieldOff);
            }
            break;

        default:
            throw CodeGenError("genExpr: tipe node tidak bisa di-generate sebagai ekspresi: " + astKindToString(node->kind));
    }
}


void CodeGenerator::genBinOp(const ASTPtr& node) {
    if (node->children.size() < 2)
        throw CodeGenError("genBinOp: BINOP butuh 2 anak.");

    const ASTPtr& left  = node->children[0];
    const ASTPtr& right = node->children[1];
    const std::string& op = node->name;

    genExpr(left);
    genExpr(right);

    int oprNum = opToOprNum(op, left->dataType);
    emit(OpCode::OPR, 0, oprNum);

    if (op == "or") {
        emit(OpCode::LIT, 0, 0);
        emit(OpCode::OPR, 0, 11); 
    }
}

void CodeGenerator::genUnOp(const ASTPtr& node) {
    if (node->children.empty())
        throw CodeGenError("genUnOp: UNOP tidak memiliki anak.");

    const std::string& op = node->name;

    genExpr(node->children[0]);

    if (op == "-") {
        emit(OpCode::OPR, 0, 1); 
    } else if (op == "not") {
        emit(OpCode::OPR, 0, 1); 
        emit(OpCode::LIT, 0, 1); 
        emit(OpCode::OPR, 0, 2);
    } else {
        throw CodeGenError("genUnOp: operator unary tidak dikenal '" + op + "'");
    }
}

void CodeGenerator::genVarRef(const ASTPtr& node) {
    int tabIdx = node->tabIndex;
    if (tabIdx == 6) { 
        emit(OpCode::LIT, 0, 1);
        return;
    }
    if (tabIdx == 7) { 
        emit(OpCode::LIT, 0, 0);
        return;
    }

    if (tabIdx < 0)
        throw CodeGenError("genVarRef: tabIndex variabel '" + node->name + "' tidak valid (-1).");

    const TabEntry& entry = symtab_.tab()[tabIdx];
    if (entry.obj == ObjClass::CONSTANT) {
        emit(OpCode::LIT, 0, entry.adr);
        return;
    }

    if (entry.obj == ObjClass::FUNCTION) {
        emit(OpCode::LIT, 0, 0);
        emit(OpCode::LIT, 0, 0);
        emit(OpCode::LIT, 0, 0);
        int lev = levelDiff(tabIdx);
        emit(OpCode::CAL, lev, entry.adr);
        return;
    }

    int addr = varAddress(tabIdx);
    int lev  = levelDiff(tabIdx);
    emit(OpCode::LOD, lev, addr);
}

void CodeGenerator::genLiteral(const ASTPtr& node) {
    switch (node->kind) {
        case ASTNodeKind::CONST_INT: {
            int val = 0;
            try { val = std::stoi(node->name); }
            catch (...) { throw CodeGenError("genLiteral: nilai integer tidak valid '" + node->name + "'"); }
            emit(OpCode::LIT, 0, val);
            break;
        }

        case ASTNodeKind::CONST_BOOL: {
            std::string lower = node->name;
            for (auto& c : lower) c = (char)tolower((unsigned char)c);
            int val = (lower == "true") ? 1 : 0;
            emit(OpCode::LIT, 0, val);
            break;
        }

        case ASTNodeKind::CONST_CHAR: {
            if (!node->name.empty()) {
                emit(OpCode::LIT, 0, (int)(unsigned char)node->name[0]);
            } else {
                emit(OpCode::LIT, 0, 0);
            }
            break;
        }

        case ASTNodeKind::CONST_REAL: {
            double val = 0.0;
            try { val = std::stod(node->name); }
            catch (...) {}
            emit(OpCode::LIT, 0, static_cast<int>(val));
            break;
        }

        case ASTNodeKind::CONST_STRING: {
            int idx = (int)stringTable_.size();
            stringTable_.push_back(node->name);
            emit(OpCode::LIT, 1, idx);
            break;
        }

        default:
            throw CodeGenError("genLiteral: jenis literal tidak dikenal.");
    }
}

void CodeGenerator::genFuncCall(const ASTPtr& node) {
    emit(OpCode::LIT, 0, 0);
    emit(OpCode::LIT, 0, 0);
    emit(OpCode::LIT, 0, 0);

    for (const auto& arg : node->children) {
        genExpr(arg);
    }

    int tabIdx = node->tabIndex;
    if (tabIdx < 0)
        throw CodeGenError("genFuncCall: tabIndex fungsi '" + node->name + "' tidak valid.");

    int funcAddr = symtab_.tab()[tabIdx].adr;
    int lev      = levelDiff(tabIdx);
    emit(OpCode::CAL, lev, funcAddr);
}

void CodeGenerator::genProcDecl(const ASTPtr& node) {
    int tabIdx = node->tabIndex;
    if (tabIdx < 0)
        throw CodeGenError("genProcDecl: tabIndex prosedur '" + node->name + "' tidak valid.");

    int procStart = nextLine();
    const_cast<SymbolTable&>(symtab_).tab()[tabIdx].adr = procStart;
    curLev_++;

    int btabIdx = node->blockIndex;
    if (btabIdx < 0) btabIdx = 1; 

    int fsize = frameSize(btabIdx);
    emit(OpCode::INT, 0, fsize);

    for (const auto& child : node->children) {
        if (child->kind == ASTNodeKind::BLOCK ||
            child->kind == ASTNodeKind::COMPOUND) {
            genCompound(child);
        }
    }

    emit(OpCode::RET, 0, 0);
    curLev_--;
}


void CodeGenerator::genFuncDecl(const ASTPtr& node) {
    int tabIdx = node->tabIndex;
    if (tabIdx < 0)
        throw CodeGenError("genFuncDecl: tabIndex fungsi '" + node->name + "' tidak valid.");

    int funcStart = nextLine();
    const_cast<SymbolTable&>(symtab_).tab()[tabIdx].adr = funcStart;
    curLev_++;

    int btabIdx = node->blockIndex;
    if (btabIdx < 0) btabIdx = 1;

    int fsize = frameSize(btabIdx);
    emit(OpCode::INT, 0, fsize);

    for (const auto& child : node->children) {
        if (child->kind == ASTNodeKind::BLOCK ||
            child->kind == ASTNodeKind::COMPOUND) {
            genCompound(child);
        }
    }

    emit(OpCode::RET, 0, 1);
    curLev_--;
}