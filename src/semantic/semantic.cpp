#include "semantic.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <stdexcept>

SemanticAnalyser::SemanticAnalyser() {}


ASTPtr SemanticAnalyser::analyse(const NodePtr& parseRoot) {
    ast_ = buildAST(parseRoot);

    // Tahap 2: dekorasi AST
    if (ast_) visitProgram(ast_);

    return ast_;
}


void SemanticAnalyser::printResults(std::ostream& out) const {
    out << "\n";
    out << "╔══════════════════════════════════════════════════╗\n";
    out << "║           DECORATED ABSTRACT SYNTAX TREE         ║\n";
    out << "╚══════════════════════════════════════════════════╝\n";
    printAST(ast_, out);

    symtab_.printTab(out);
    symtab_.printBtab(out);
    symtab_.printAtab(out);

    if (!warnings_.empty()) {
        out << "\n===== WARNINGS =====\n";
        for (auto& w : warnings_)
            out << "  [WARN] " << w.message << "\n";
    }
}


const NodePtr& SemanticAnalyser::childAt(const NodePtr& n, size_t idx) const {
    static NodePtr nullNode = nullptr;
    if (!n || idx >= n->children.size()) return nullNode;
    return n->children[idx];
}


std::string SemanticAnalyser::labelOf(const NodePtr& n) const {
    if (!n) return "";
    return n->label;
}


std::string SemanticAnalyser::valueOf(const NodePtr& n) const {
    if (!n) return "";
    const std::string& lbl = n->label;
    auto p = lbl.find('(');
    if (p == std::string::npos) return "";
    auto q = lbl.rfind(')');
    if (q == std::string::npos || q <= p) return "";
    return lbl.substr(p + 1, q - p - 1);
}

bool SemanticAnalyser::labelStartsWith(const NodePtr& n, const std::string& prefix) const {
    if (!n) return false;
    if (n->label.size() < prefix.size()) return false;
    for (size_t i = 0; i < prefix.size(); ++i) {
        unsigned char lc = static_cast<unsigned char>(n->label[i]);
        unsigned char pc = static_cast<unsigned char>(prefix[i]);
        if (std::tolower(lc) != std::tolower(pc)) return false;
    }
    return true;
}

bool SemanticAnalyser::labelIs(const NodePtr& n, const std::string& exact) const {
    if (!n) return false;
    if (n->label.size() != exact.size()) return false;
    for (size_t i = 0; i < exact.size(); ++i) {
        unsigned char lc = static_cast<unsigned char>(n->label[i]);
        unsigned char ec = static_cast<unsigned char>(exact[i]);
        if (std::tolower(lc) != std::tolower(ec)) return false;
    }
    return true;
}


void SemanticAnalyser::semanticError(const std::string& msg) const {
    throw SemanticError("[SEMANTIC ERROR] " + msg);
}

void SemanticAnalyser::warn(const std::string& msg) {
    warnings_.push_back(SemanticWarning(msg));
}


ASTPtr SemanticAnalyser::buildAST(const NodePtr& n) {
    if (!n) return nullptr;
    const std::string& lbl = n->label;

    if (lbl == "<program>")              return buildProgram(n);
    if (lbl == "<declaration-part>")     return buildDeclPart(n);
    if (lbl == "<const-declaration>")    return buildConstDecl(n);
    if (lbl == "<var-declaration>")      return buildVarDecl(n);
    if (lbl == "<type-declaration>")     return buildTypeDecl(n);
    if (lbl == "<subprogram-declaration>") return buildSubprogDecl(n);
    if (lbl == "<procedure-declaration>") return buildProcDecl(n);
    if (lbl == "<function-declaration>") return buildFuncDecl(n);
    if (lbl == "<block>")                return buildBlock(n);
    if (lbl == "<compound-statement>")   return buildCompound(n);
    if (lbl == "<statement-list>")       return buildStmtList(n);
    if (lbl == "<statement>")            return buildStatement(n);
    if (lbl == "<assignment-statement>") return buildAssign(n);
    if (lbl == "<if-statement>")         return buildIfStmt(n);
    if (lbl == "<while-statement>")      return buildWhileStmt(n);
    if (lbl == "<for-statement>")        return buildForStmt(n);
    if (lbl == "<repeat-statement>")     return buildRepeatStmt(n);
    if (lbl == "<case-statement>")       return buildCaseStmt(n);
    if (lbl == "<case-block>")           return buildCaseBlock(n);
    if (lbl == "<procedure/function-call>") return buildProcCall(n);
    if (lbl == "<expression>")           return buildExpression(n);
    if (lbl == "<simple-expression>")    return buildSimpleExpr(n);
    if (lbl == "<term>")                 return buildTerm(n);
    if (lbl == "<factor>")               return buildFactor(n);
    if (lbl == "<variable>")             return buildVariable(n);
    if (lbl == "<constant>")             return buildConstant(n);
    if (lbl == "<type>")                 return buildType(n);
    if (lbl == "<array-type>")           return buildArrayType(n);
    if (lbl == "<record-type>")          return buildRecordType(n);
    if (lbl == "<range>")                return buildRange(n);
    if (lbl == "<enumerated>")           return buildEnumerated(n);
    if (lbl == "<field-list>")           return buildFieldList(n);
    if (lbl == "<field-part>")           return buildFieldPart(n);
    if (lbl == "<identifier-list>")      return buildIdentList(n);
    if (lbl == "<formal-parameter-list>") return buildFormalParams(n);
    if (lbl == "<parameter-group>")      return buildParamGroup(n);

    return nullptr;
}


ASTPtr SemanticAnalyser::buildProgram(const NodePtr& n) {
    // children: <program-header>, <declaration-part>, <compound-statement>, PERIOD
    std::string progName;
    for (auto& ch : n->children) {
        if (ch->label == "<program-header>") {
            for (auto& hch : ch->children) {
                if (labelStartsWith(hch, "IDENT")) {
                    progName = valueOf(hch);
                }
            }
        }
    }

    auto node = makeNode(ASTNodeKind::PROGRAM, progName);

    for (auto& ch : n->children) {
        if (ch->label == "<declaration-part>") {
            ASTPtr decl = buildDeclPart(ch);
            if (decl) node->children.push_back(decl);
        } else if (ch->label == "<compound-statement>") {
            ASTPtr body = buildCompound(ch);
            if (body) node->children.push_back(body);
        }
    }

    return node;
}

// Kumpulkan semua deklarasi
ASTPtr SemanticAnalyser::buildDeclPart(const NodePtr& n) {
    auto node = makeNode(ASTNodeKind::BLOCK, "declarations");
    for (auto& ch : n->children) {
        ASTPtr child = buildAST(ch);
        if (!child) continue;
        if (child->kind == ASTNodeKind::VAR_DECL && child->name.empty()) {
            for (auto& entry : child->children) {
                if (entry) node->children.push_back(entry);
            }
            continue;
        }
        node->children.push_back(child);
    }
    if (node->children.empty()) return nullptr;
    return node;
}


ASTPtr SemanticAnalyser::buildConstDecl(const NodePtr& n) {
    auto node = makeNode(ASTNodeKind::CONST_DECL);
    size_t i = 0;
    const auto& ch = n->children;
    if (i < ch.size() && labelStartsWith(ch[i], "CONSTSY")) i++; 

    while (i < ch.size()) {
        if (!labelStartsWith(ch[i], "IDENT")) { i++; continue; }
        std::string name = valueOf(ch[i]); i++;
        if (i < ch.size() && labelStartsWith(ch[i], "EQL")) i++;
        ASTPtr val = nullptr;
        if (i < ch.size() && ch[i]->label == "<constant>") {
            val = buildConstant(ch[i]); i++;
        }
        if (i < ch.size() && labelStartsWith(ch[i], "SEMICOLON")) i++; 

        auto entry = makeNode(ASTNodeKind::CONST_DECL, name);
        if (val) entry->children.push_back(val);
        node->children.push_back(entry);
    }
    return node;
}

ASTPtr SemanticAnalyser::buildVarDecl(const NodePtr& n) {
    auto node = makeNode(ASTNodeKind::VAR_DECL);
    size_t i = 0;
    const auto& ch = n->children;
    if (i < ch.size() && labelStartsWith(ch[i], "VARSY")) i++; 

    while (i < ch.size()) {
        if (ch[i]->label != "<identifier-list>") { i++; continue; }
        ASTPtr identList = buildIdentList(ch[i]); i++;
        if (i < ch.size() && labelStartsWith(ch[i], "COLON")) i++; 
        ASTPtr typeNode = nullptr;
        if (i < ch.size() && ch[i]->label == "<type>") {
            typeNode = buildType(ch[i]); i++;
        }
        if (i < ch.size() && labelStartsWith(ch[i], "SEMICOLON")) i++; 
        // Flat style: tiap identifier jadi satu VarDecl(name).
        if (identList) {
            for (auto& idNode : identList->children) {
                auto entry = makeNode(ASTNodeKind::VAR_DECL, idNode->name);
                if (typeNode) entry->children.push_back(typeNode);
                node->children.push_back(entry);
            }
        }
    }
    return node;
}


ASTPtr SemanticAnalyser::buildTypeDecl(const NodePtr& n) {
    auto node = makeNode(ASTNodeKind::TYPE_DECL);
    size_t i = 0;
    const auto& ch = n->children;
    if (i < ch.size() && labelStartsWith(ch[i], "TYPESY")) i++;

    while (i < ch.size()) {
        if (!labelStartsWith(ch[i], "IDENT")) { i++; continue; }
        std::string name = valueOf(ch[i]); i++;
        if (i < ch.size() && labelStartsWith(ch[i], "EQL")) i++;
        ASTPtr typeNode = nullptr;
        if (i < ch.size() && ch[i]->label == "<type>") {
            typeNode = buildType(ch[i]); i++;
        }
        if (i < ch.size() && labelStartsWith(ch[i], "SEMICOLON")) i++;

        auto entry = makeNode(ASTNodeKind::TYPE_DECL, name);
        if (typeNode) entry->children.push_back(typeNode);
        node->children.push_back(entry);
    }
    return node;
}


ASTPtr SemanticAnalyser::buildSubprogDecl(const NodePtr& n) {
    for (auto& ch : n->children) {
        if (ch->label == "<procedure-declaration>") return buildProcDecl(ch);
        if (ch->label == "<function-declaration>")  return buildFuncDecl(ch);
    }
    return nullptr;
}


ASTPtr SemanticAnalyser::buildProcDecl(const NodePtr& n) {
    std::string name;
    ASTPtr params = nullptr, body = nullptr;

    size_t i = 0;
    const auto& ch = n->children;
    if (i < ch.size() && labelStartsWith(ch[i], "PROCEDURESY")) i++;
    if (i < ch.size() && labelStartsWith(ch[i], "IDENT")) {
        name = valueOf(ch[i]); i++;
    }
    if (i < ch.size() && ch[i]->label == "<formal-parameter-list>") {
        params = buildFormalParams(ch[i]); i++;
    }
    while (i < ch.size() && labelStartsWith(ch[i], "SEMICOLON")) i++;
    if (i < ch.size() && ch[i]->label == "<block>") {
        body = buildBlock(ch[i]); i++;
    }

    auto node = makeNode(ASTNodeKind::PROC_DECL, name);
    if (params) node->children.push_back(params);
    if (body)   node->children.push_back(body);
    return node;
}


ASTPtr SemanticAnalyser::buildFuncDecl(const NodePtr& n) {
    std::string name, retTypeName;
    ASTPtr params = nullptr, body = nullptr;

    size_t i = 0;
    const auto& ch = n->children;
    if (i < ch.size() && labelStartsWith(ch[i], "FUNCTIONSY")) i++;
    if (i < ch.size() && labelStartsWith(ch[i], "IDENT")) {
        name = valueOf(ch[i]); i++;
    }
    if (i < ch.size() && ch[i]->label == "<formal-parameter-list>") {
        params = buildFormalParams(ch[i]); i++;
    }
    if (i < ch.size() && labelStartsWith(ch[i], "COLON")) i++;
    if (i < ch.size() && labelStartsWith(ch[i], "IDENT")) {
        retTypeName = valueOf(ch[i]); i++;
    }
    while (i < ch.size() && labelStartsWith(ch[i], "SEMICOLON")) i++;
    if (i < ch.size() && ch[i]->label == "<block>") {
        body = buildBlock(ch[i]); i++;
    }

    auto node = makeNode(ASTNodeKind::FUNC_DECL, name);
    // simpan tipe kembalian sebagai child TYPE_IDENT
    auto retType = makeNode(ASTNodeKind::TYPE_IDENT, retTypeName);
    node->children.push_back(retType);
    if (params) node->children.push_back(params);
    if (body)   node->children.push_back(body);
    return node;
}


ASTPtr SemanticAnalyser::buildBlock(const NodePtr& n) {
    auto node = makeNode(ASTNodeKind::BLOCK);
    for (auto& ch : n->children) {
        ASTPtr child = buildAST(ch);
        if (child) node->children.push_back(child);
    }
    return node;
}


ASTPtr SemanticAnalyser::buildFormalParams(const NodePtr& n) {
    auto node = makeNode(ASTNodeKind::IDENT_LIST, "params");
    for (auto& ch : n->children) {
        if (ch->label == "<parameter-group>") {
            ASTPtr pg = buildParamGroup(ch);
            if (pg) node->children.push_back(pg);
        }
    }
    return node;
}


ASTPtr SemanticAnalyser::buildParamGroup(const NodePtr& n) {
    ASTPtr identList = nullptr;
    ASTPtr typeNode  = nullptr;

    for (auto& ch : n->children) {
        if (ch->label == "<identifier-list>") identList = buildIdentList(ch);
        else if (ch->label == "<type>")       typeNode  = buildType(ch);
        else if (ch->label == "<array-type>") typeNode  = buildArrayType(ch);
        else if (labelStartsWith(ch, "IDENT")) {
            if (!identList)
                identList = makeNode(ASTNodeKind::IDENT_LIST);
            else
                typeNode = makeNode(ASTNodeKind::TYPE_IDENT, valueOf(ch));
        }
    }

    auto node = makeNode(ASTNodeKind::PARAM_DECL);
    if (identList) node->children.push_back(identList);
    if (typeNode)  node->children.push_back(typeNode);
    return node;
}


ASTPtr SemanticAnalyser::buildType(const NodePtr& n) {
    // <type> bisa punya 1 child: array-type, enumerated, record-type,
    // range, atau IDENT tunggal
    if (n->children.empty()) return makeNode(ASTNodeKind::TYPE_IDENT, "");

    auto& first = n->children[0];
    if (first->label == "<array-type>")   return buildArrayType(first);
    if (first->label == "<enumerated>")   return buildEnumerated(first);
    if (first->label == "<record-type>")  return buildRecordType(first);
    if (first->label == "<range>") {
        auto subrange = makeNode(ASTNodeKind::TYPE_SUBRANGE);
        subrange->children.push_back(buildRange(first));
        return subrange;
    }
    if (labelStartsWith(first, "IDENT"))  return makeNode(ASTNodeKind::TYPE_IDENT, valueOf(first));

    return buildAST(first);
}


ASTPtr SemanticAnalyser::buildArrayType(const NodePtr& n) {
    auto node = makeNode(ASTNodeKind::TYPE_ARRAY);
    for (auto& ch : n->children) {
        // skip keyword ARRAY, '[', ']', OF
        if (labelStartsWith(ch, "ARRAYSY") ||
            labelStartsWith(ch, "LBRACK")  ||
            labelStartsWith(ch, "RBRACK")  ||
            labelStartsWith(ch, "OFSY"))    continue;

        if (ch->label == "<range>") {
            node->children.push_back(buildRange(ch));
        } else if (ch->label == "<type>") {
            node->children.push_back(buildType(ch));
        } else if (labelStartsWith(ch, "IDENT")) {
            // tipe indeks berupa named type
            node->children.push_back(makeNode(ASTNodeKind::TYPE_IDENT, valueOf(ch)));
        }
    }
    return node;
}


ASTPtr SemanticAnalyser::buildRecordType(const NodePtr& n) {
    auto node = makeNode(ASTNodeKind::TYPE_RECORD);
    for (auto& ch : n->children) {
        if (ch->label == "<field-list>")
            node->children.push_back(buildFieldList(ch));
    }
    return node;
}


ASTPtr SemanticAnalyser::buildRange(const NodePtr& n) {
    auto node = makeNode(ASTNodeKind::RANGE);
    for (auto& ch : n->children) {
        if (ch->label == "<constant>")
            node->children.push_back(buildConstant(ch));
    }
    return node;
}


ASTPtr SemanticAnalyser::buildEnumerated(const NodePtr& n) {
    auto node = makeNode(ASTNodeKind::TYPE_ENUMERATED);
    for (auto& ch : n->children) {
        if (labelStartsWith(ch, "IDENT"))
            node->children.push_back(makeNode(ASTNodeKind::VAR_REF, valueOf(ch)));
    }
    return node;
}

ASTPtr SemanticAnalyser::buildFieldList(const NodePtr& n) {
    auto node = makeNode(ASTNodeKind::BLOCK, "fields");
    for (auto& ch : n->children) {
        if (ch->label == "<field-part>")
            node->children.push_back(buildFieldPart(ch));
    }
    return node;
}

ASTPtr SemanticAnalyser::buildFieldPart(const NodePtr& n) {
    ASTPtr identList = nullptr, typeNode = nullptr;
    for (auto& ch : n->children) {
        if (ch->label == "<identifier-list>") identList = buildIdentList(ch);
        else if (ch->label == "<type>")       typeNode  = buildType(ch);
    }
    auto node = makeNode(ASTNodeKind::FIELD_DECL);
    if (identList) node->children.push_back(identList);
    if (typeNode)  node->children.push_back(typeNode);
    return node;
}


ASTPtr SemanticAnalyser::buildIdentList(const NodePtr& n) {
    auto node = makeNode(ASTNodeKind::IDENT_LIST);
    for (auto& ch : n->children) {
        if (labelStartsWith(ch, "IDENT"))
            node->children.push_back(makeNode(ASTNodeKind::VAR_REF, valueOf(ch)));
    }
    return node;
}


ASTPtr SemanticAnalyser::buildCompound(const NodePtr& n) {
    auto node = makeNode(ASTNodeKind::COMPOUND);
    for (auto& ch : n->children) {
        if (ch->label == "<statement-list>") {
            ASTPtr sl = buildStmtList(ch);
            if (sl) {
                // tambahkan masing-masing statement langsung ke COMPOUND
                for (auto& s : sl->children)
                    node->children.push_back(s);
            }
        }
    }
    return node;
}


ASTPtr SemanticAnalyser::buildStmtList(const NodePtr& n) {
    auto node = makeNode(ASTNodeKind::COMPOUND);
    for (auto& ch : n->children) {
        if (labelStartsWith(ch, "SEMICOLON")) continue;
        if (ch->label == "<statement>") {
            ASTPtr s = buildStatement(ch);
            if (s) node->children.push_back(s);
        }
    }
    return node;
}


ASTPtr SemanticAnalyser::buildStatement(const NodePtr& n) {
    if (!n || n->children.empty())
        return makeNode(ASTNodeKind::EMPTY_STMT);

    auto& first = n->children[0];
    if (!first) return makeNode(ASTNodeKind::EMPTY_STMT);

    const std::string& lbl = first->label;
    if (lbl == "<compound-statement>")       return buildCompound(first);
    if (lbl == "<assignment-statement>")     return buildAssign(first);
    if (lbl == "<if-statement>")             return buildIfStmt(first);
    if (lbl == "<while-statement>")          return buildWhileStmt(first);
    if (lbl == "<for-statement>")            return buildForStmt(first);
    if (lbl == "<repeat-statement>")         return buildRepeatStmt(first);
    if (lbl == "<case-statement>")           return buildCaseStmt(first);
    if (lbl == "<procedure/function-call>")  return buildProcCall(first);

    return makeNode(ASTNodeKind::EMPTY_STMT);
}


ASTPtr SemanticAnalyser::buildAssign(const NodePtr& n) {
    auto node = makeNode(ASTNodeKind::ASSIGN);
    for (auto& ch : n->children) {
        if (ch->label == "<variable>") {
            node->children.push_back(buildVariable(ch));
        } else if (labelStartsWith(ch, "BECOMES")) {
            // skip operator :=
        } else if (ch->label == "<expression>") {
            node->children.push_back(buildExpression(ch));
        }
    }
    return node;
}


ASTPtr SemanticAnalyser::buildIfStmt(const NodePtr& n) {
    auto node = makeNode(ASTNodeKind::IF_STMT);
    bool pastThen = false, pastElse = false;
    for (auto& ch : n->children) {
        if (labelStartsWith(ch, "IFSY"))   continue;
        if (labelStartsWith(ch, "THENSY")) { pastThen = true; continue; }
        if (labelStartsWith(ch, "ELSESY")) { pastElse = true; continue; }

        if (ch->label == "<expression>")
            node->children.push_back(buildExpression(ch));
        else if (ch->label == "<statement>")
            node->children.push_back(buildStatement(ch));
        else if (ch->label == "<compound-statement>")
            node->children.push_back(buildCompound(ch));
        (void)pastThen; (void)pastElse;
    }
    return node;
}


ASTPtr SemanticAnalyser::buildWhileStmt(const NodePtr& n) {
    auto node = makeNode(ASTNodeKind::WHILE_STMT);
    for (auto& ch : n->children) {
        if (labelStartsWith(ch, "WHILESY") ||
            labelStartsWith(ch, "DOSY")    ||
            labelStartsWith(ch, "SEMICOLON")) continue;
        if (ch->label == "<expression>")
            node->children.push_back(buildExpression(ch));
        else if (ch->label == "<compound-statement>")
            node->children.push_back(buildCompound(ch));
    }
    return node;
}


ASTPtr SemanticAnalyser::buildForStmt(const NodePtr& n) {
    auto node = makeNode(ASTNodeKind::FOR_STMT);
    bool hasDown = false;
    std::string varName;
    for (auto& ch : n->children) {
        if (labelStartsWith(ch, "FORSY") ||
            labelStartsWith(ch, "BECOMES") ||
            labelStartsWith(ch, "DOSY") ||
            labelStartsWith(ch, "SEMICOLON")) continue;
        if (labelStartsWith(ch, "DOWNTOSY")) { hasDown = true; continue; }
        if (labelStartsWith(ch, "TOSY"))     { continue; }
        if (labelStartsWith(ch, "IDENT")) {
            varName = valueOf(ch);
            node->children.push_back(makeNode(ASTNodeKind::VAR_REF, varName));
            continue;
        }
        if (ch->label == "<expression>")
            node->children.push_back(buildExpression(ch));
        else if (ch->label == "<compound-statement>")
            node->children.push_back(buildCompound(ch));
    }
    node->name = hasDown ? "downto" : "to";
    return node;
}


ASTPtr SemanticAnalyser::buildRepeatStmt(const NodePtr& n) {
    auto node = makeNode(ASTNodeKind::REPEAT_STMT);
    for (auto& ch : n->children) {
        if (labelStartsWith(ch, "REPEATSY") ||
            labelStartsWith(ch, "UNTILSY"))  continue;
        if (ch->label == "<statement-list>") {
            ASTPtr sl = buildStmtList(ch);
            if (sl) {
                for (auto& s : sl->children) node->children.push_back(s);
            }
        } else if (ch->label == "<expression>") {
            node->children.push_back(buildExpression(ch));
        }
    }
    return node;
}


ASTPtr SemanticAnalyser::buildCaseStmt(const NodePtr& n) {
    auto node = makeNode(ASTNodeKind::CASE_STMT);
    for (auto& ch : n->children) {
        if (labelStartsWith(ch, "CASESY") ||
            labelStartsWith(ch, "OFSY")   ||
            labelStartsWith(ch, "ENDSY"))  continue;
        if (ch->label == "<expression>")
            node->children.push_back(buildExpression(ch));
        else if (ch->label == "<case-block>")
            node->children.push_back(buildCaseBlock(ch));
    }
    return node;
}

ASTPtr SemanticAnalyser::buildCaseBlock(const NodePtr& n) {
    auto node = makeNode(ASTNodeKind::CASE_BLOCK);
    for (auto& ch : n->children) {
        if (labelStartsWith(ch, "COLON") ||
            labelStartsWith(ch, "SEMICOLON") ||
            labelStartsWith(ch, "COMMA"))   continue;
        if (ch->label == "<constant>")
            node->children.push_back(buildConstant(ch));
        else if (ch->label == "<statement>")
            node->children.push_back(buildStatement(ch));
        else if (ch->label == "<case-block>")
            node->children.push_back(buildCaseBlock(ch));
    }
    return node;
}


ASTPtr SemanticAnalyser::buildProcCall(const NodePtr& n) {
    std::string name;
    auto node = makeNode(ASTNodeKind::PROC_CALL);
    for (auto& ch : n->children) {
        // Kasus 1: IDENT langsung (writeln(...))
        if (labelStartsWith(ch, "IDENT") && name.empty()) {
            name = valueOf(ch);
            node->name = name;
        }
        // Kasus 2: nama tersembunyi di dalam <variable> (proc tanpa '()')
        else if (ch->label == "<variable>" && name.empty()) {
            for (auto& vch : ch->children) {
                if (labelStartsWith(vch, "IDENT")) {
                    name = valueOf(vch);
                    node->name = name;
                    break;
                }
            }
        }
        // Parameter list
        else if (ch->label == "<parameter-list>") {
            for (auto& param : ch->children) {
                if (param->label == "<expression>")
                    node->children.push_back(buildExpression(param));
            }
        }
        // Skip LPARENT, RPARENT, COMMA
        else if (labelStartsWith(ch, "LPARENT") ||
                 labelStartsWith(ch, "RPARENT") ||
                 labelStartsWith(ch, "COMMA"))   { /* skip */ }
        // Expression langsung
        else if (ch->label == "<expression>") {
            node->children.push_back(buildExpression(ch));
        }
    }
    return node;
}


ASTPtr SemanticAnalyser::buildExpression(const NodePtr& n) {
    std::vector<ASTPtr> parts;
    std::string op;

    for (auto& ch : n->children) {
        if (ch->label == "<simple-expression>")
            parts.push_back(buildSimpleExpr(ch));
        else if (ch->label == "<relational-operator>") {
            if (!ch->children.empty()) op = ch->children[0]->label;
            std::transform(op.begin(), op.end(), op.begin(), ::tolower);
            // ekstrak operator tunggal dari label seperti "EQL", "NEQ", dsb
            if (op == "eql") op = "=";
            else if (op == "neq") op = "<>";
            else if (op == "lss") op = "<";
            else if (op == "gtr") op = ">";
            else if (op == "leq") op = "<=";
            else if (op == "geq") op = ">=";
        }
    }

    if (parts.size() == 1) return parts[0];
    if (parts.size() == 2) {
        auto binop = makeNode(ASTNodeKind::BINOP, op);
        binop->children.push_back(parts[0]);
        binop->children.push_back(parts[1]);
        return binop;
    }
    if (parts.empty()) return makeNode(ASTNodeKind::EMPTY_STMT);
    return parts[0];
}


ASTPtr SemanticAnalyser::buildSimpleExpr(const NodePtr& n) {
    std::string unarySign;
    std::vector<std::pair<std::string, ASTPtr>> terms; // (op, term)

    std::string pendingOp = "";
    for (auto& ch : n->children) {
        if (labelStartsWith(ch, "PLUS") && terms.empty() && unarySign.empty()) {
            unarySign = "+"; continue;
        }
        if (labelStartsWith(ch, "MINUS") && terms.empty() && unarySign.empty()) {
            unarySign = "-"; continue;
        }
        if (ch->label == "<additive-operator>") {
            if (!ch->children.empty()) {
                std::string raw = ch->children[0]->label;
                std::transform(raw.begin(), raw.end(), raw.begin(), ::tolower);
                if (raw == "plus")  pendingOp = "+";
                else if (raw == "minus") pendingOp = "-";
                else if (raw == "orsy")  pendingOp = "or";
                else pendingOp = raw;
            }
            continue;
        }
        if (ch->label == "<term>") {
            ASTPtr t = buildTerm(ch);
            terms.push_back({pendingOp, t});
            pendingOp = "";
        }
    }

    if (terms.empty()) return makeNode(ASTNodeKind::EMPTY_STMT);

    // Terapkan unary sign pada term pertama
    ASTPtr result = terms[0].second;
    if (unarySign == "-") {
        auto neg = makeNode(ASTNodeKind::UNOP, "-");
        neg->children.push_back(result);
        result = neg;
    }

    // Lipat dari kiri ke kanan
    for (size_t i = 1; i < terms.size(); ++i) {
        auto binop = makeNode(ASTNodeKind::BINOP, terms[i].first);
        binop->children.push_back(result);
        binop->children.push_back(terms[i].second);
        result = binop;
    }
    return result;
}


ASTPtr SemanticAnalyser::buildTerm(const NodePtr& n) {
    std::string pendingOp;
    ASTPtr result = nullptr;

    for (auto& ch : n->children) {
        if (ch->label == "<multiplicative-operator>") {
            if (!ch->children.empty()) {
                std::string raw = ch->children[0]->label;
                std::transform(raw.begin(), raw.end(), raw.begin(), ::tolower);
                if (raw == "times")  pendingOp = "*";
                else if (raw == "rdiv")  pendingOp = "/";
                else if (raw == "idiv")  pendingOp = "div";
                else if (raw == "imod")  pendingOp = "mod";
                else if (raw == "andsy") pendingOp = "and";
                else pendingOp = raw;
            }
            continue;
        }
        if (ch->label == "<factor>") {
            ASTPtr f = buildFactor(ch);
            if (!result) { result = f; }
            else {
                auto binop = makeNode(ASTNodeKind::BINOP, pendingOp);
                binop->children.push_back(result);
                binop->children.push_back(f);
                result = binop;
            }
        }
    }
    return result ? result : makeNode(ASTNodeKind::EMPTY_STMT);
}


ASTPtr SemanticAnalyser::buildFactor(const NodePtr& n) {
    if (n->children.empty()) return makeNode(ASTNodeKind::EMPTY_STMT);

    auto& first = n->children[0];
    const std::string& lbl = first->label;

    // Literal
    if (labelStartsWith(first, "INTCON"))  return makeNode(ASTNodeKind::CONST_INT,    valueOf(first));
    if (labelStartsWith(first, "REALCON")) return makeNode(ASTNodeKind::CONST_REAL,   valueOf(first));
    if (labelStartsWith(first, "CHARCON")) return makeNode(ASTNodeKind::CONST_CHAR,   valueOf(first));
    if (labelStartsWith(first, "STRING"))  return makeNode(ASTNodeKind::CONST_STRING, valueOf(first));

    // Variabel atau function call
    if (lbl == "<variable>")               return buildVariable(first);
    if (lbl == "<procedure/function-call>") {
        // function call dalam ekspresi
        auto fc = buildProcCall(first);
        fc->kind = ASTNodeKind::FUNC_CALL;
        return fc;
    }

    // Ekspresi dalam kurung: ( <expression> )
    if (labelStartsWith(first, "LPARENT")) {
        if (n->children.size() >= 2)
            return buildExpression(n->children[1]);
    }

    // NOT <factor>
    if (labelStartsWith(first, "NOTSY")) {
        auto notNode = makeNode(ASTNodeKind::UNOP, "not");
        if (n->children.size() >= 2)
            notNode->children.push_back(buildFactor(n->children[1]));
        return notNode;
    }

    return makeNode(ASTNodeKind::EMPTY_STMT);
}


ASTPtr SemanticAnalyser::buildVariable(const NodePtr& n) {
    std::string name;
    ASTPtr base = nullptr;

    for (auto& ch : n->children) {
        if (labelStartsWith(ch, "IDENT") && !base) {
            name = valueOf(ch);
            base = makeNode(ASTNodeKind::VAR_REF, name);
        } else if (ch->label == "<component-variable>") {
            if (!base) continue;
            // LBRACK → array access; PERIOD → field access
            bool isArray = false, isField = false;
            std::string fieldName;
            for (auto& cv : ch->children) {
                if (labelStartsWith(cv, "LBRACK"))  isArray = true;
                if (labelStartsWith(cv, "PERIOD"))  isField = true;
                if (labelStartsWith(cv, "IDENT"))   fieldName = valueOf(cv);
            }
            if (isArray) {
                auto acc = makeNode(ASTNodeKind::ARRAY_ACCESS);
                acc->children.push_back(base);
                for (auto& cv : ch->children) {
                    if (cv->label == "<index-list>") {
                        for (auto& idx : cv->children) {
                            if (labelStartsWith(idx, "INTCON"))
                                acc->children.push_back(makeNode(ASTNodeKind::CONST_INT, valueOf(idx)));
                            else if (labelStartsWith(idx, "IDENT"))
                                acc->children.push_back(makeNode(ASTNodeKind::VAR_REF, valueOf(idx)));
                        }
                    }
                }
                base = acc;
            } else if (isField) {
                auto fa = makeNode(ASTNodeKind::FIELD_ACCESS, fieldName);
                fa->children.push_back(base);
                base = fa;
            }
        }
    }
    return base ? base : makeNode(ASTNodeKind::VAR_REF, name);
}


ASTPtr SemanticAnalyser::buildConstant(const NodePtr& n) {
    bool negative = false;
    for (auto& ch : n->children) {
        if (labelStartsWith(ch, "MINUS")) { negative = true; continue; }
        if (labelStartsWith(ch, "PLUS"))  continue;
        if (labelStartsWith(ch, "INTCON")) {
            std::string v = valueOf(ch);
            if (negative) v = "-" + v;
            return makeNode(ASTNodeKind::CONST_INT, v);
        }
        if (labelStartsWith(ch, "REALCON")) {
            std::string v = valueOf(ch);
            if (negative) v = "-" + v;
            return makeNode(ASTNodeKind::CONST_REAL, v);
        }
        if (labelStartsWith(ch, "CHARCON"))
            return makeNode(ASTNodeKind::CONST_CHAR, valueOf(ch));
        if (labelStartsWith(ch, "STRING"))
            return makeNode(ASTNodeKind::CONST_STRING, valueOf(ch));
        if (labelStartsWith(ch, "IDENT"))
            return makeNode(ASTNodeKind::VAR_REF, valueOf(ch));
    }
    return makeNode(ASTNodeKind::EMPTY_STMT);
}


// VISIT FUNCTIONS (Dekorasi AST)


void SemanticAnalyser::visitProgram(const ASTPtr& n) {
    if (!n) return;

    // Masukkan nama program ke tab
    int idx = symtab_.addTab(n->name, ObjClass::PROGRAM, DataType::VOID);
    n->tabIndex  = idx;
    n->lexLevel  = 0;
    n->dataType  = DataType::VOID;

    // 1) deklarasi global tetap di level 0
    for (auto& ch : n->children) {
        if (!ch) continue;
        if (ch->kind == ASTNodeKind::BLOCK && ch->name == "declarations") {
            visitBlock(ch);
        }
    }

    // 2) body program dijalankan dalam blok sendiri (main block)
    int mainBlockIdx = symtab_.enterBlock();
    n->blockIndex = mainBlockIdx;

    for (auto& ch : n->children) {
        if (!ch) continue;
        if (ch->kind == ASTNodeKind::COMPOUND) {
            ch->blockIndex = mainBlockIdx;
            ch->lexLevel = symtab_.currentLevel();
            visitCompound(ch);
        } else if (ch->kind == ASTNodeKind::BLOCK && ch->name != "declarations") {
            ch->blockIndex = mainBlockIdx;
            ch->lexLevel = symtab_.currentLevel();
            visitBlock(ch);
        }
    }

    symtab_.leaveBlock();
}


void SemanticAnalyser::visitBlock(const ASTPtr& n) {
    if (!n) return;
    for (auto& ch : n->children) {
        if (!ch) continue;
        switch (ch->kind) {
            case ASTNodeKind::CONST_DECL:  visitConstDecl(ch); break;
            case ASTNodeKind::VAR_DECL:    visitVarDecl(ch);   break;
            case ASTNodeKind::TYPE_DECL:   visitTypeDecl(ch);  break;
            case ASTNodeKind::PROC_DECL:   visitProcDecl(ch);  break;
            case ASTNodeKind::FUNC_DECL:   visitFuncDecl(ch);  break;
            case ASTNodeKind::COMPOUND:    visitCompound(ch);  break;
            case ASTNodeKind::BLOCK:       visitBlock(ch);     break;
            default: visitStatement(ch); break;
        }
    }
}


void SemanticAnalyser::visitConstDecl(const ASTPtr& n) {
    // n bisa berupa CONST_DECL container atau entry tunggal
    if (n->name.empty()) {
        // container: kunjungi setiap entry
        for (auto& ch : n->children) visitConstDecl(ch);
        return;
    }

    // Cek redeklaasi
    if (symtab_.lookupCurrentBlock(n->name) >= 0)
        semanticError("Identifier '" + n->name + "' sudah dideklarasikan dalam scope ini.");

    DataType dt = DataType::NOTYPE;
    if (!n->children.empty()) {
        auto& val = n->children[0];
        dt = visitExpr(val);
    }

    int idx = symtab_.addTab(n->name, ObjClass::CONSTANT, dt, 0, 1, 0);
    symtab_.tab()[idx].initialized = true;
    n->tabIndex  = idx;
    n->lexLevel  = symtab_.currentLevel();
    n->dataType  = dt;
}


void SemanticAnalyser::visitVarDecl(const ASTPtr& n) {
    // Container: kunjungi setiap entry VarDecl.
    if (n->name.empty()) {
        for (auto& ch : n->children) {
            if (ch && ch->kind == ASTNodeKind::VAR_DECL) visitVarDecl(ch);
        }
        return;
    }

    // Fallback kompatibilitas format lama (group berisi IDENT_LIST).
    ASTPtr legacyIdentList = nullptr;
    ASTPtr legacyTypeNode = nullptr;
    for (auto& ch : n->children) {
        if (!ch) continue;
        if (ch->kind == ASTNodeKind::IDENT_LIST) legacyIdentList = ch;
        else if (!legacyTypeNode) legacyTypeNode = ch;
    }
    if (legacyIdentList) {
        DataType dt = resolveTypeNode(legacyTypeNode);
        int ref = 0;
        int adrOffset = symtab_.btab()[symtab_.blockAt(symtab_.currentLevel())].vsze;
        for (auto& identNode : legacyIdentList->children) {
            std::string name = identNode->name;
            if (symtab_.lookupCurrentBlock(name) >= 0)
                semanticError("Identifier '" + name + "' sudah dideklarasikan dalam scope ini.");
            int varSize = symtab_.sizeOf(dt, ref);
            int tabIdx = symtab_.addTab(name, ObjClass::VARIABLE, dt, ref, 1, adrOffset);
            adrOffset += varSize;
            identNode->tabIndex = tabIdx;
            identNode->lexLevel = symtab_.currentLevel();
            identNode->dataType = dt;
            symtab_.btab()[symtab_.blockAt(symtab_.currentLevel())].vsze = adrOffset;
        }
        n->dataType = dt;
        return;
    }

    ASTPtr typeNode = n->children.empty() ? nullptr : n->children[0];
    if (!typeNode) return;

    DataType dt = resolveTypeNode(typeNode);
    int ref = 0;

    if (typeNode->kind == ASTNodeKind::TYPE_ARRAY) {
        ATabEntry ae;
        if (!typeNode->children.empty()) {
            auto& idx0 = typeNode->children[0];
            if (idx0->kind == ASTNodeKind::RANGE) {
                ae.xtyp = DataType::INTEGER;
                if (idx0->children.size() >= 2) {
                    if (idx0->children[0]->kind == ASTNodeKind::CONST_INT)
                        ae.low = std::stoi(idx0->children[0]->name);
                    if (idx0->children[1]->kind == ASTNodeKind::CONST_INT)
                        ae.high = std::stoi(idx0->children[1]->name);
                }
            }
        }
        if (typeNode->children.size() >= 2) {
            ae.etyp = resolveTypeNode(typeNode->children.back());
        }
        ae.elsz = symtab_.sizeOf(ae.etyp);
        ae.size = (ae.high - ae.low + 1) * ae.elsz;
        ref = symtab_.addAtab(ae);
        typeNode->arrayIndex = ref;
    }

    if (symtab_.lookupCurrentBlock(n->name) >= 0)
        semanticError("Identifier '" + n->name + "' sudah dideklarasikan dalam scope ini.");

    int curBlock = symtab_.blockAt(symtab_.currentLevel());
    int adrOffset = symtab_.btab()[curBlock].vsze;
    int varSize = symtab_.sizeOf(dt, ref);
    int tabIdx = symtab_.addTab(n->name, ObjClass::VARIABLE, dt, ref, 1, adrOffset);
    symtab_.btab()[curBlock].vsze = adrOffset + varSize;

    n->tabIndex = tabIdx;
    n->lexLevel = symtab_.currentLevel();
    n->dataType = dt;
    // Biar output AST ringkas seperti contoh, detail tipe tidak dicetak lagi sebagai child.
    n->children.clear();
}


void SemanticAnalyser::visitTypeDecl(const ASTPtr& n) {
    if (n->name.empty()) {
        for (auto& ch : n->children) visitTypeDecl(ch);
        return;
    }
    if (symtab_.lookupCurrentBlock(n->name) >= 0)
        semanticError("Tipe '" + n->name + "' sudah dideklarasikan.");

    DataType dt = DataType::NOTYPE;
    if (!n->children.empty()) dt = resolveTypeNode(n->children[0]);

    int idx = symtab_.addTab(n->name, ObjClass::TYPE_NAME, dt);
    n->tabIndex = idx;
    n->dataType = dt;
}


void SemanticAnalyser::visitProcDecl(const ASTPtr& n) {
    if (symtab_.lookupCurrentBlock(n->name) >= 0)
        semanticError("Procedure '" + n->name + "' sudah dideklarasikan.");

    // Daftarkan procedure ke tab (sebelum masuk blok agar rekursi boleh)
    int procIdx = symtab_.addTab(n->name, ObjClass::PROCEDURE, DataType::VOID);
    n->tabIndex = procIdx;
    n->dataType = DataType::VOID;

    // Masuk ke blok baru
    int blockIdx = symtab_.enterBlock();
    n->blockIndex = blockIdx;
    n->lexLevel   = symtab_.currentLevel();

    for (auto& ch : n->children) {
        if (ch->kind == ASTNodeKind::IDENT_LIST && ch->name == "params") {
            int adrOffset = 0;
            for (auto& pg : ch->children) {
                if (pg->kind != ASTNodeKind::PARAM_DECL) continue;
                ASTPtr identList = nullptr;
                ASTPtr typeNode  = nullptr;
                for (auto& pgch : pg->children) {
                    if (pgch->kind == ASTNodeKind::IDENT_LIST) identList = pgch;
                    else typeNode = pgch;
                }
                if (!identList) continue;
                DataType dt = resolveTypeNode(typeNode);
                for (auto& idNode : identList->children) {
                    if (symtab_.lookupCurrentBlock(idNode->name) >= 0)
                        semanticError("Parameter '" + idNode->name + "' sudah dideklarasikan.");
                    int varSize = symtab_.sizeOf(dt);
                    int tabIdx = symtab_.addTab(idNode->name, ObjClass::PARAMETER, dt, 0, 1, adrOffset);
                    symtab_.tab()[tabIdx].initialized = true;
                    adrOffset += varSize;
                    idNode->tabIndex = tabIdx;
                    idNode->lexLevel = symtab_.currentLevel();
                    idNode->dataType = dt;
                }
            }
            int curBlock = symtab_.blockAt(symtab_.currentLevel());
            symtab_.btab()[curBlock].lpar = symtab_.btab()[curBlock].last;
            symtab_.btab()[curBlock].psze = adrOffset;
        } else if (ch->kind == ASTNodeKind::BLOCK) {
            visitBlock(ch);
        } else if (ch->kind == ASTNodeKind::COMPOUND) {
            visitCompound(ch);
        }
    }

    symtab_.leaveBlock();
}


void SemanticAnalyser::visitFuncDecl(const ASTPtr& n) {
    if (symtab_.lookupCurrentBlock(n->name) >= 0)
        semanticError("Function '" + n->name + "' sudah dideklarasikan.");

    // Tentukan tipe kembalian dari child[0] (TYPE_IDENT)
    DataType retType = DataType::NOTYPE;
    if (!n->children.empty() && n->children[0]->kind == ASTNodeKind::TYPE_IDENT)
        retType = resolveTypeName(n->children[0]->name);

    int funcIdx = symtab_.addTab(n->name, ObjClass::FUNCTION, retType);
    n->tabIndex = funcIdx;
    n->dataType = retType;

    int blockIdx = symtab_.enterBlock();
    n->blockIndex = blockIdx;
    n->lexLevel   = symtab_.currentLevel();

    for (auto& ch : n->children) {
        if (ch->kind == ASTNodeKind::IDENT_LIST && ch->name == "params") {
            int adrOffset = 0;
            for (auto& pg : ch->children) {
                if (pg->kind != ASTNodeKind::PARAM_DECL) continue;
                ASTPtr identList = nullptr;
                ASTPtr typeNode  = nullptr;
                for (auto& pgch : pg->children) {
                    if (pgch->kind == ASTNodeKind::IDENT_LIST) identList = pgch;
                    else typeNode = pgch;
                }
                if (!identList) continue;
                DataType dt = resolveTypeNode(typeNode);
                for (auto& idNode : identList->children) {
                    if (symtab_.lookupCurrentBlock(idNode->name) >= 0)
                        semanticError("Parameter '" + idNode->name + "' sudah dideklarasikan.");
                    int varSize = symtab_.sizeOf(dt);
                    int tabIdx = symtab_.addTab(idNode->name, ObjClass::PARAMETER, dt, 0, 1, adrOffset);
                    symtab_.tab()[tabIdx].initialized = true;
                    adrOffset += varSize;
                    idNode->tabIndex = tabIdx;
                    idNode->lexLevel = symtab_.currentLevel();
                    idNode->dataType = dt;
                }
            }
            int curBlock = symtab_.blockAt(symtab_.currentLevel());
            symtab_.btab()[curBlock].lpar = symtab_.btab()[curBlock].last;
            symtab_.btab()[curBlock].psze = adrOffset;
        } else if (ch->kind == ASTNodeKind::BLOCK) {
            visitBlock(ch);
        } else if (ch->kind == ASTNodeKind::COMPOUND) {
            visitCompound(ch);
        }
    }

    symtab_.leaveBlock();
}


void SemanticAnalyser::visitStatement(const ASTPtr& n) {
    if (!n) return;
    switch (n->kind) {
        case ASTNodeKind::ASSIGN:      visitAssign(n);      break;
        case ASTNodeKind::IF_STMT:     visitIfStmt(n);      break;
        case ASTNodeKind::WHILE_STMT:  visitWhileStmt(n);   break;
        case ASTNodeKind::FOR_STMT:    visitForStmt(n);     break;
        case ASTNodeKind::REPEAT_STMT: visitRepeatStmt(n);  break;
        case ASTNodeKind::CASE_STMT:   visitCaseStmt(n);    break;
        case ASTNodeKind::PROC_CALL:   visitProcCall(n);    break;
        case ASTNodeKind::COMPOUND:    visitCompound(n);    break;
        case ASTNodeKind::EMPTY_STMT:  break;
        default: break;
    }
}


void SemanticAnalyser::visitCompound(const ASTPtr& n) {
    for (auto& ch : n->children) visitStatement(ch);
}


void SemanticAnalyser::visitAssign(const ASTPtr& n) {
    if (n->children.size() < 2) return;

    auto& target = n->children[0];
    DataType targetType = DataType::NOTYPE;
    if (target->kind == ASTNodeKind::VAR_REF) {
        int idx = symtab_.lookup(target->name);
        if (idx < 0)
            semanticError("Identifier '" + target->name + "' belum dideklarasikan.");
        auto& entry = symtab_.tab()[idx];
        target->tabIndex = idx;
        target->lexLevel = entry.lev;
        target->dataType = entry.type;
        targetType = entry.type;
    } else {
        // target berupa array access atau field access — cek tipe via visitExpr biasa
        targetType = visitExpr(target);
    }

    // value  (children[1])
    DataType valueType  = visitExpr(n->children[1]);

    if (!isAssignmentCompatible(targetType, valueType)) {
        semanticError("Assignment tidak kompatibel: target bertipe '"
            + dataTypeToString(targetType) + "', nilai bertipe '"
            + dataTypeToString(valueType) + "'.");
    }

    if (target->tabIndex >= 0) {
        symtab_.tab()[target->tabIndex].initialized = true;
        target->initialized = true;
    }

    n->dataType = DataType::VOID;
}


void SemanticAnalyser::visitIfStmt(const ASTPtr& n) {
    if (n->children.empty()) return;
    DataType condType = visitExpr(n->children[0]);
    if (condType != DataType::BOOLEAN && condType != DataType::NOTYPE)
        semanticError("Kondisi if-statement harus bertipe Boolean.");
    for (size_t i = 1; i < n->children.size(); ++i)
        visitStatement(n->children[i]);
    n->dataType = DataType::VOID;
}


void SemanticAnalyser::visitWhileStmt(const ASTPtr& n) {
    if (n->children.empty()) return;
    DataType condType = visitExpr(n->children[0]);
    if (condType != DataType::BOOLEAN && condType != DataType::NOTYPE)
        semanticError("Kondisi while-statement harus bertipe Boolean.");
    for (size_t i = 1; i < n->children.size(); ++i)
        visitStatement(n->children[i]);
    n->dataType = DataType::VOID;
}


void SemanticAnalyser::visitForStmt(const ASTPtr& n) {
    // children: VAR_REF(varName), fromExpr, toExpr, body
    if (n->children.size() < 3) return;

    DataType varType  = visitExpr(n->children[0]);
    DataType fromType = visitExpr(n->children[1]);
    DataType toType   = visitExpr(n->children[2]);

    if (!isOrdinal(varType))
        semanticError("Variabel kontrol for-loop '" + n->children[0]->name
                      + "' harus bertipe ordinal.");
    if (!isAssignmentCompatible(varType, fromType))
        semanticError("Ekspresi awal for-loop tidak kompatibel dengan variabel kontrol.");
    if (!isAssignmentCompatible(varType, toType))
        semanticError("Ekspresi akhir for-loop tidak kompatibel dengan variabel kontrol.");

    if (n->children.size() >= 4)
        visitStatement(n->children[3]);
    n->dataType = DataType::VOID;
}


void SemanticAnalyser::visitRepeatStmt(const ASTPtr& n) {
    if (n->children.empty()) return;
    // Semua child kecuali terakhir adalah statement; terakhir adalah kondisi
    for (size_t i = 0; i + 1 < n->children.size(); ++i)
        visitStatement(n->children[i]);
    DataType condType = visitExpr(n->children.back());
    if (condType != DataType::BOOLEAN && condType != DataType::NOTYPE)
        semanticError("Kondisi repeat-until harus bertipe Boolean.");
    n->dataType = DataType::VOID;
}


void SemanticAnalyser::visitCaseStmt(const ASTPtr& n) {
    if (n->children.empty()) return;
    DataType exprType = visitExpr(n->children[0]);
    if (!isOrdinal(exprType))
        semanticError("Ekspresi case harus bertipe ordinal.");
    for (size_t i = 1; i < n->children.size(); ++i)
        visitStatement(n->children[i]);
    n->dataType = DataType::VOID;
}


void SemanticAnalyser::visitProcCall(const ASTPtr& n) {
    int idx = symtab_.lookup(n->name);
    if (idx < 0)
        semanticError("Procedure/function '" + n->name + "' belum dideklarasikan.");

    auto& entry = symtab_.tab()[idx];
    if (entry.obj != ObjClass::PROCEDURE && entry.obj != ObjClass::FUNCTION)
        semanticError("'" + n->name + "' bukan procedure atau function.");

    n->tabIndex = idx;
    n->lexLevel = entry.lev;
    n->dataType = entry.type;

    // Kunjungi argumen
    for (auto& arg : n->children) visitExpr(arg);
}


DataType SemanticAnalyser::visitExpr(const ASTPtr& n) {
    if (!n) return DataType::NOTYPE;
    DataType dt = DataType::NOTYPE;

    switch (n->kind) {
        case ASTNodeKind::BINOP:        dt = visitBinOp(n);      break;
        case ASTNodeKind::UNOP:         dt = visitUnOp(n);       break;
        case ASTNodeKind::VAR_REF:      dt = visitVarRef(n);     break;
        case ASTNodeKind::FUNC_CALL:    dt = visitFuncCall(n);   break;
        case ASTNodeKind::ARRAY_ACCESS: dt = visitArrayAccess(n);break;
        case ASTNodeKind::FIELD_ACCESS: dt = visitFieldAccess(n);break;
        case ASTNodeKind::CONST_INT:
            n->dataType = DataType::INTEGER; return DataType::INTEGER;
        case ASTNodeKind::CONST_REAL:
            n->dataType = DataType::REAL;    return DataType::REAL;
        case ASTNodeKind::CONST_CHAR:
            n->dataType = DataType::CHAR;    return DataType::CHAR;
        case ASTNodeKind::CONST_STRING:
            n->dataType = DataType::STRING;  return DataType::STRING;
        case ASTNodeKind::CONST_BOOL:
            n->dataType = DataType::BOOLEAN; return DataType::BOOLEAN;
        default: break;
    }
    n->dataType = dt;
    return dt;
}


DataType SemanticAnalyser::visitBinOp(const ASTPtr& n) {
    if (n->children.size() < 2) return DataType::NOTYPE;
    DataType lt = visitExpr(n->children[0]);
    DataType rt = visitExpr(n->children[1]);
    DataType result = resultTypeOfBinOp(n->name, lt, rt);
    if (result == DataType::UNKNOWN)
        semanticError("Operator '" + n->name + "' tidak bisa diterapkan pada tipe '"
            + dataTypeToString(lt) + "' dan '" + dataTypeToString(rt) + "'.");
    n->dataType = result;
    return result;
}


DataType SemanticAnalyser::visitUnOp(const ASTPtr& n) {
    if (n->children.empty()) return DataType::NOTYPE;
    DataType dt = visitExpr(n->children[0]);
    if (n->name == "-") {
        if (dt != DataType::INTEGER && dt != DataType::REAL)
            semanticError("Operator unary '-' hanya berlaku untuk tipe numerik.");
        n->dataType = dt;
        return dt;
    }
    if (n->name == "not") {
        if (dt != DataType::BOOLEAN)
            semanticError("Operator 'not' hanya berlaku untuk tipe Boolean.");
        n->dataType = DataType::BOOLEAN;
        return DataType::BOOLEAN;
    }
    n->dataType = dt;
    return dt;
}


DataType SemanticAnalyser::visitVarRef(const ASTPtr& n) {
    int idx = symtab_.lookup(n->name);
    if (idx < 0)
        semanticError("Identifier '" + n->name + "' belum dideklarasikan.");

    auto& entry = symtab_.tab()[idx];
    n->tabIndex  = idx;
    n->lexLevel  = entry.lev;
    n->dataType  = entry.type;

    if (entry.obj == ObjClass::VARIABLE && !entry.initialized)
        semanticError("Variabel tidak bisa dioperasikan jika belum diinisialisasi dengan nilai.");

    return entry.type;
}


DataType SemanticAnalyser::visitFuncCall(const ASTPtr& n) {
    int idx = symtab_.lookup(n->name);
    if (idx < 0)
        semanticError("Function '" + n->name + "' belum dideklarasikan.");

    auto& entry = symtab_.tab()[idx];
    n->tabIndex = idx;
    n->lexLevel = entry.lev;
    n->dataType = entry.type;

    for (auto& arg : n->children) visitExpr(arg);
    return entry.type;
}


DataType SemanticAnalyser::visitArrayAccess(const ASTPtr& n) {
    if (n->children.empty()) return DataType::NOTYPE;
    DataType baseType = visitExpr(n->children[0]);
    if (baseType != DataType::ARRAY)
        semanticError("Variabel bukan array, tidak bisa diakses dengan indeks.");

    // Dapatkan tipe elemen dari atab
    int arrRef = -1;
    if (!n->children.empty() && n->children[0]->tabIndex >= 0) {
        arrRef = symtab_.tab()[n->children[0]->tabIndex].ref;
    }

    DataType elemType = DataType::NOTYPE;
    if (arrRef >= 0 && arrRef < (int)symtab_.atab().size())
        elemType = symtab_.atab()[arrRef].etyp;

    // Cek tipe indeks
    for (size_t i = 1; i < n->children.size(); ++i) {
        DataType idxType = visitExpr(n->children[i]);
        if (!isOrdinal(idxType))
            semanticError("Indeks array harus bertipe ordinal.");
    }

    n->dataType = elemType;
    return elemType;
}


DataType SemanticAnalyser::visitFieldAccess(const ASTPtr& n) {
    if (n->children.empty()) return DataType::NOTYPE;
    DataType baseType = visitExpr(n->children[0]);
    if (baseType != DataType::RECORD)
        semanticError("Variabel bukan record, tidak bisa diakses fieldnya.");

    // Lookup field name di btab record
    int recRef = -1;
    if (!n->children.empty() && n->children[0]->tabIndex >= 0)
        recRef = symtab_.tab()[n->children[0]->tabIndex].ref;

    int fieldIdx = -1;
    if (recRef >= 0 && recRef < (int)symtab_.btab().size()) {
        // cari field dalam blok record
        int cur = symtab_.btab()[recRef].last;
        while (cur > 0) {
            std::string fname = symtab_.tab()[cur].name;
            std::transform(fname.begin(), fname.end(), fname.begin(), ::tolower);
            std::string query = n->name;
            std::transform(query.begin(), query.end(), query.begin(), ::tolower);
            if (fname == query) { fieldIdx = cur; break; }
            cur = symtab_.tab()[cur].link;
        }
    }

    if (fieldIdx < 0)
        semanticError("Field '" + n->name + "' tidak ditemukan dalam record.");

    DataType ft = symtab_.tab()[fieldIdx].type;
    n->tabIndex = fieldIdx;
    n->dataType = ft;
    return ft;
}


DataType SemanticAnalyser::resolveTypeName(const std::string& name) const {
    std::string lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    if (lower == "integer") return DataType::INTEGER;
    if (lower == "real")    return DataType::REAL;
    if (lower == "char")    return DataType::CHAR;
    if (lower == "boolean") return DataType::BOOLEAN;
    if (lower == "string")  return DataType::STRING;

    // Cari di symbol table (named type)
    int idx = symtab_.lookup(name);
    if (idx >= 0 && symtab_.tab()[idx].obj == ObjClass::TYPE_NAME)
        return symtab_.tab()[idx].type;

    return DataType::NOTYPE;
}


DataType SemanticAnalyser::resolveTypeNode(const ASTPtr& n) {
    if (!n) return DataType::NOTYPE;
    switch (n->kind) {
        case ASTNodeKind::TYPE_IDENT:
            return resolveTypeName(n->name);
        case ASTNodeKind::TYPE_ARRAY:
            return DataType::ARRAY;
        case ASTNodeKind::TYPE_RECORD:
            return DataType::RECORD;
        case ASTNodeKind::TYPE_SUBRANGE:
            return DataType::SUBRANGE;
        case ASTNodeKind::TYPE_ENUMERATED:
            return DataType::ENUMERATED;
        default:
            return DataType::NOTYPE;
    }
}


bool SemanticAnalyser::isCompatible(DataType t1, DataType t2) const {
    if (t1 == t2) return true;
    if (t1 == DataType::NOTYPE || t2 == DataType::NOTYPE) return true; // beri keringanan
    if (t1 == DataType::SUBRANGE || t2 == DataType::SUBRANGE) return true;
    return false;
}


bool SemanticAnalyser::isAssignmentCompatible(DataType target, DataType value) const {
    if (target == DataType::NOTYPE || value == DataType::NOTYPE) return true;
    if (target == value) return true;
    if (target == DataType::REAL && value == DataType::INTEGER) return true;
    if (isCompatible(target, value)) return true;
    return false;
}


bool SemanticAnalyser::isNumeric(DataType dt) const {
    return dt == DataType::INTEGER || dt == DataType::REAL;
}


bool SemanticAnalyser::isOrdinal(DataType dt) const {
    return dt == DataType::INTEGER || dt == DataType::CHAR
        || dt == DataType::BOOLEAN || dt == DataType::SUBRANGE
        || dt == DataType::ENUMERATED;
}


DataType SemanticAnalyser::resultTypeOfBinOp(const std::string& op,
                                              DataType l, DataType r) const {
    // Operator relasional → Boolean
    if (op == "=" || op == "<>" || op == "<" || op == ">" ||
        op == "<=" || op == ">=") {
        if (isCompatible(l, r)) return DataType::BOOLEAN;
        return DataType::UNKNOWN;
    }
    // Operator logika → Boolean
    if (op == "and" || op == "or") {
        if (l == DataType::BOOLEAN && r == DataType::BOOLEAN)
            return DataType::BOOLEAN;
        return DataType::UNKNOWN;
    }
    // Aritmatika
    if (op == "+") {
        if (l == DataType::STRING && r == DataType::STRING) return DataType::STRING;
        if (isNumeric(l) && isNumeric(r)) {
            return (l == DataType::REAL || r == DataType::REAL)
                   ? DataType::REAL : DataType::INTEGER;
        }
        return DataType::UNKNOWN;
    }
    if (op == "-" || op == "*") {
        if (isNumeric(l) && isNumeric(r))
            return (l == DataType::REAL || r == DataType::REAL)
                   ? DataType::REAL : DataType::INTEGER;
        return DataType::UNKNOWN;
    }
    if (op == "/") {
        if (isNumeric(l) && isNumeric(r)) return DataType::REAL; // selalu Real
        return DataType::UNKNOWN;
    }
    if (op == "div" || op == "mod") {
        if (l == DataType::INTEGER && r == DataType::INTEGER)
            return DataType::INTEGER;
        return DataType::UNKNOWN;
    }
    // Tipe NOTYPE dibiarkan lewat (toleransi)
    if (l == DataType::NOTYPE || r == DataType::NOTYPE) return DataType::NOTYPE;
    return DataType::UNKNOWN;
}