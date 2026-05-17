#include "ast.hpp"
#include <iostream>

// Nama tipe buat kebutuhan print/debug
std::string dataTypeToString(DataType dt) {
    switch (dt) {
        case DataType::INTEGER:    return "Integer";
        case DataType::REAL:       return "Real";
        case DataType::CHAR:       return "Char";
        case DataType::BOOLEAN:    return "Boolean";
        case DataType::STRING:     return "String";
        case DataType::ARRAY:      return "Array";
        case DataType::RECORD:     return "Record";
        case DataType::SUBRANGE:   return "Subrange";
        case DataType::ENUMERATED: return "Enumerated";
        case DataType::VOID:       return "Void";
        case DataType::NOTYPE:     return "NoType";
        default:                   return "Unknown";
    }
}

// Nama node AST versi string, supaya output lebih enak dibaca
std::string astKindToString(ASTNodeKind k) {
    switch (k) {
        case ASTNodeKind::PROGRAM:          return "Program";
        case ASTNodeKind::CONST_DECL:       return "ConstDecl";
        case ASTNodeKind::VAR_DECL:         return "VarDecl";
        case ASTNodeKind::TYPE_DECL:        return "TypeDecl";
        case ASTNodeKind::PARAM_DECL:       return "ParamDecl";
        case ASTNodeKind::PROC_DECL:        return "ProcDecl";
        case ASTNodeKind::FUNC_DECL:        return "FuncDecl";
        case ASTNodeKind::TYPE_IDENT:       return "TypeIdent";
        case ASTNodeKind::TYPE_ARRAY:       return "TypeArray";
        case ASTNodeKind::TYPE_RECORD:      return "TypeRecord";
        case ASTNodeKind::TYPE_SUBRANGE:    return "TypeSubrange";
        case ASTNodeKind::TYPE_ENUMERATED:  return "TypeEnumerated";
        case ASTNodeKind::ASSIGN:           return "Assign";
        case ASTNodeKind::IF_STMT:          return "IfStmt";
        case ASTNodeKind::WHILE_STMT:       return "WhileStmt";
        case ASTNodeKind::FOR_STMT:         return "ForStmt";
        case ASTNodeKind::REPEAT_STMT:      return "RepeatStmt";
        case ASTNodeKind::CASE_STMT:        return "CaseStmt";
        case ASTNodeKind::CASE_BLOCK:       return "CaseBlock";
        case ASTNodeKind::PROC_CALL:        return "ProcCall";
        case ASTNodeKind::FUNC_CALL:        return "FuncCall";
        case ASTNodeKind::COMPOUND:         return "Block";
        case ASTNodeKind::EMPTY_STMT:       return "EmptyStmt";
        case ASTNodeKind::BINOP:            return "BinOp";
        case ASTNodeKind::UNOP:             return "UnOp";
        case ASTNodeKind::VAR_REF:          return "VarRef";
        case ASTNodeKind::ARRAY_ACCESS:     return "ArrayAccess";
        case ASTNodeKind::FIELD_ACCESS:     return "FieldAccess";
        case ASTNodeKind::CONST_INT:        return "ConstInt";
        case ASTNodeKind::CONST_REAL:       return "ConstReal";
        case ASTNodeKind::CONST_CHAR:       return "ConstChar";
        case ASTNodeKind::CONST_STRING:     return "ConstString";
        case ASTNodeKind::CONST_BOOL:       return "ConstBool";
        case ASTNodeKind::RANGE:            return "Range";
        case ASTNodeKind::ENUM_LIST:        return "EnumList";
        case ASTNodeKind::FIELD_DECL:       return "FieldDecl";
        case ASTNodeKind::BLOCK:            return "Block";
        case ASTNodeKind::IDENT_LIST:       return "IdentList";
        default:                            return "?";
    }
}

namespace {

std::string exprInline(const ASTPtr& n) {
    if (!n) return "";

    switch (n->kind) {
        case ASTNodeKind::VAR_REF:
            return "'" + n->name + "'";
        case ASTNodeKind::CONST_INT:
        case ASTNodeKind::CONST_REAL:
        case ASTNodeKind::CONST_CHAR:
        case ASTNodeKind::CONST_STRING:
        case ASTNodeKind::CONST_BOOL:
            return n->name;
        case ASTNodeKind::BINOP:
            if (n->children.size() >= 2)
                return exprInline(n->children[0]) + n->name + exprInline(n->children[1]);
            return n->name;
        case ASTNodeKind::UNOP:
            if (!n->children.empty()) return n->name + exprInline(n->children[0]);
            return n->name;
        case ASTNodeKind::PROC_CALL:
        case ASTNodeKind::FUNC_CALL:
            return n->name + "(...)";
        default:
            break;
    }

    if (!n->name.empty()) return n->name;
    if (n->children.size() == 1) return exprInline(n->children[0]);
    return astKindToString(n->kind);
}

std::string nodeLabel(const ASTPtr& node) {
    if (!node) return "";

    if (node->kind == ASTNodeKind::ASSIGN && node->children.size() >= 2) {
        return "Assign(" + exprInline(node->children[0]) + " := " + exprInline(node->children[1]) + ")";
    }

    if (node->kind == ASTNodeKind::PROC_CALL || node->kind == ASTNodeKind::FUNC_CALL) {
        if (!node->name.empty()) return node->name + "(...)";
    }

    std::string label = astKindToString(node->kind);
    if (!node->name.empty()) label += "('" + node->name + "')";
    return label;
}

} // namespace

void printAST(const ASTPtr& node, std::ostream& out,
              const std::string& prefix, bool isLast) {
    if (!node) return;

    out << prefix << (isLast ? "└── " : "├── ");
    out << nodeLabel(node);

    bool hasAnnotation = (node->dataType != DataType::NOTYPE)
                      || (node->tabIndex >= 0)
                      || (node->lexLevel >= 0);

    if (hasAnnotation) {
        out << "  →  ";
        if (node->dataType != DataType::NOTYPE)
            out << "type:" << dataTypeToString(node->dataType) << " ";
        if (node->tabIndex >= 0)
            out << "tab:" << node->tabIndex << " ";
        if (node->blockIndex >= 0)
            out << "btab:" << node->blockIndex << " ";
        if (node->arrayIndex >= 0)
            out << "atab:" << node->arrayIndex << " ";
        if (node->lexLevel >= 0)
            out << "lev:" << node->lexLevel << " ";
    }

    out << "\n";

    std::string childPrefix = prefix + (isLast ? "    " : "│   ");
    for (size_t i = 0; i < node->children.size(); ++i) {
        bool last = (i == node->children.size() - 1);
        printAST(node->children[i], out, childPrefix, last);
    }
}
