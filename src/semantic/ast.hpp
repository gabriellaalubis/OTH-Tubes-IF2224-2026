#ifndef AST_HPP
#define AST_HPP

#include <string>
#include <vector>
#include <memory>

// Tipe data yang diketahui semantic analyser

enum class DataType {
    UNKNOWN,
    INTEGER,
    REAL,
    CHAR,
    BOOLEAN,
    STRING,
    ARRAY,
    RECORD,
    SUBRANGE,
    ENUMERATED,
    VOID,       
    NOTYPE      
};

std::string dataTypeToString(DataType dt);

// Jenis-jenis node AST

enum class ASTNodeKind {
    // Program
    PROGRAM,

    // Deklarasi
    CONST_DECL,
    VAR_DECL,
    TYPE_DECL,
    PARAM_DECL,
    PROC_DECL,
    FUNC_DECL,

    // Tipe
    TYPE_IDENT,      
    TYPE_ARRAY,
    TYPE_RECORD,
    TYPE_SUBRANGE,
    TYPE_ENUMERATED,

    // Statement
    ASSIGN,
    IF_STMT,
    WHILE_STMT,
    FOR_STMT,
    REPEAT_STMT,
    CASE_STMT,
    CASE_BLOCK,
    PROC_CALL,
    COMPOUND,
    EMPTY_STMT,

    // Ekspresi
    BINOP,           // operator biner: +, -, *, /, div, mod, and, or, =, <>, <, >, <=, >=
    UNOP,            // operator unary: -, not
    VAR_REF,         // referensi variabel / identifier
    FUNC_CALL,       // pemanggilan fungsi (menghasilkan nilai)
    ARRAY_ACCESS,    // a[i]
    FIELD_ACCESS,    // r.field
    CONST_INT,
    CONST_REAL,
    CONST_CHAR,
    CONST_STRING,
    CONST_BOOL,

    // Range & enum untuk deklarasi tipe
    RANGE,
    ENUM_LIST,

    // Field record
    FIELD_DECL,

    // Block untuk procedure/function body
    BLOCK,

    // Identifier list dipakai saat deklarasi multi-var
    IDENT_LIST,
};

// Struct utama Node AST

struct ASTNode {
    ASTNodeKind kind;
    std::string name;
    std::vector<std::shared_ptr<ASTNode>> children;

    DataType dataType   = DataType::NOTYPE;   // tipe hasil ekspresi / deklarasi
    int tabIndex   = -1;                      // indeks di tab
    int blockIndex = -1;                      // indeks di btab
    int arrayIndex = -1;                      // indeks di atab
    int lexLevel   = -1;                      // lexical level
    bool initialized = false;                

    explicit ASTNode(ASTNodeKind k, const std::string& n = "")
        : kind(k), name(n) {}
};

using ASTPtr = std::shared_ptr<ASTNode>;

inline ASTPtr makeNode(ASTNodeKind k, const std::string& n = "") {
    return std::make_shared<ASTNode>(k, n);
}

void printAST(const ASTPtr& node, std::ostream& out,
              const std::string& prefix = "", bool isLast = true);

std::string astKindToString(ASTNodeKind k);

#endif 