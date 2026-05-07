#ifndef PARSER_H
#define PARSER_H

#include "../lexer/token.hpp"
#include <vector>
#include <string>
#include <memory>
#include <stdexcept>
using namespace std;

struct ParseNode {
    string label;
    vector<shared_ptr<ParseNode>> children;
    explicit ParseNode(const string& lbl) : label(lbl) {}
};
using NodePtr = shared_ptr<ParseNode>;

class SyntaxError : public runtime_error {
public:
    explicit SyntaxError(const string& msg) : runtime_error(msg) {}
};

class Parser {
public:
    explicit Parser(const vector<Token>& tokens);
    NodePtr parse();
    static void printTree(const NodePtr& node, ostream& out,
                          const string& prefix = "", bool isLast = true);

private:
    vector<Token> tokens;
    size_t pos; 

    const Token& current() const;            // token saat ini 
    const Token& peek(int offset = 1) const; // look-ahead 
    bool check(TokenType t) const;
    bool checkAny(initializer_list<TokenType> types) const;
    NodePtr expect(TokenType t);           
    NodePtr consume();                      
    NodePtr parseProgram();
    NodePtr parseProgramHeader();
    NodePtr parseDeclarationPart();

    NodePtr parseConstDeclaration();
    NodePtr parseConstant();
    NodePtr parseVarDeclaration();
    NodePtr parseTypeDeclaration();
    NodePtr parseType();
    NodePtr parseArrayType();
    NodePtr parseRange();        
    NodePtr parseEnumerated();
    NodePtr parseRecordType();
    NodePtr parseFieldList();
    NodePtr parseFieldPart();

    NodePtr parseSubprogramDeclaration();
    NodePtr parseProcedureDeclaration();
    NodePtr parseFunctionDeclaration();
    NodePtr parseBlock();
    NodePtr parseFormalParameterList();
    NodePtr parseParameterGroup();

    NodePtr parseCompoundStatement();
    NodePtr parseStatementList();
    NodePtr parseStatement();
    NodePtr parseAssignmentStatement(NodePtr varNode); 
    NodePtr parseIfStatement();
    NodePtr parseCaseStatement();
    NodePtr parseCaseBlock();
    NodePtr parseWhileStatement();
    NodePtr parseRepeatStatement();
    NodePtr parseForStatement();
    NodePtr parseProcFuncCall();   
    NodePtr parseParameterList();

    NodePtr parseVariable();         
    NodePtr parseComponentVariable(); 
    NodePtr parseIdentifierList();
    NodePtr parseIndexList();

    NodePtr parseExpression();
    NodePtr parseSimpleExpression();
    NodePtr parseTerm();
    NodePtr parseFactor();
    NodePtr parseRelationalOperator();
    NodePtr parseAdditiveOperator();
    NodePtr parseMultiplicativeOperator();

    bool isRelationalOp() const;
    bool isAdditiveOp() const;
    bool isMultiplicativeOp() const;
    bool isConstantStart() const;   
    bool isTypeStart() const;      
};

#endif