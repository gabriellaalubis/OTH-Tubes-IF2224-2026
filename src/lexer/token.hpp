#ifndef TOKEN_H
#define TOKEN_H

#include <string>
using namespace std;

enum TokenType {
    INTCON, REALCON, CHARCON, STRING, 
    TRUESY, FALSESY,
    NOTSY, PLUS, MINUS, TIMES, IDIV, RDIV, IMOD,
    ANDSY, ORSY, EQL, NEQ, GTR, GEQ, LSS, LEQ, 
    LPARENT, RPARENT, LBRACK, RBRACK, COMMA, SEMICOLON, PERIOD, COLON,
    BECOMES, CONSTSY, TYPESY, VARSY, FUNCTIONSY, PROCEDURESY,
    ARRAYSY, RECORDSY, PROGRAMSY, IDENT, BEGINSY, 
    IFSY, CASESY, REPEATSY, WHILESY, FORSY, ENDSY, ELSESY, UNTILSY,
    OFSY, DOSY, TOSY, DOWNTOSY, THENSY, COMMENT,
    UNKNOWN, EOF_TOKEN
};

struct Token{
    TokenType type;
    string value;
    int line;
    int col;
};

string tokenTypetoString(TokenType type);

#endif
