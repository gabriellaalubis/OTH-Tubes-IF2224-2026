#ifndef TOKEN_H
#define TOKEN_H

#include <string>

enum TokenType {
    INTCON, REALCON, CHARCON, STRING, 
    NOTSY, PLUS, MINUS, TIMES, IDIV, RDIV, IMOD,
    ANDSY, ORSY, EQL, NEQ, GTR, GEQ, LSS, LEQ, 
    LPARENT, RPARENT, LBRACK, RBRACK, COMMA, SEMICOLON, PERIOD, COLON,
    BECOMES, CONSTSY, TYPESY, VARSY, FUNCTIONSY, PROCEDURESY,
    ARRAYSY, RECORDSY, PROGRAMSY, IDENT, BEGINSY, 
    IFSY, CASESY, REPREATSY, WHILESY, FORSY, ENDSY, ELSESY, UNTILSY,
    OFSY, DOSY, TOSY, DOWNTOSY, THENSY, COMMENT,
    UNKNOWN, EOF_TOKEN
};

struct Token{
    TokenType type;
    std::string value;
    int line;
    int col;
};

std::string tokenTypetoString(TokenType type);

#endif