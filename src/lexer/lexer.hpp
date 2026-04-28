#ifndef LEXER_H
#define LEXER_H

#include "token.hpp"
#include <fstream>
#include <string>
#include <map>
#include <queue>

enum DFAState {
    S0,              

    S_IDENT,       

    S_MINUS,         
    S_INT,            
    S_INT_DOT,        // sudah baca ddd.
    S_REAL,           
    S_INT_NEG,        
    S_INT_NEG_DOT,    // sudah baca -ddd.
    S_REAL_NEG,       // membaca digit desimal negatif

    
    S_STR_OPEN,       // baru baca petik pertama 
    S_STR_BODY,       // di dalam isi string
    S_STR_QUOTE,      // baru baca '\'' di dalam string , bisa escape or penutup

    S_COLON,         
    S_LT,             
    S_GT,             
    S_EQL,            

    S_LPAREN,         // baru baca '('
    S_CMT_BRACE,      // di dalam komentar '{'
    S_CMT_BRACE_STAR, // di dalam '{', baru baca '*' , menunggu ')'

    S_CMT_PAREN,      // di dalam komentar '(*'
    S_CMT_PAREN_STAR, // di dalam '(*', baru baca '*' , menunggu ')'

    S_DEAD            // dead state / error
};

class Lexer {
public:
    explicit Lexer(const std::string& filename);
    ~Lexer();

    Token nextToken();

    bool isEOF() const;

    static std::string formatToken(const Token& tok);

private:
    std::ifstream inputFile;
    int           currentLine;
    int           currentCol;
    DFAState      state;

    bool hasPending;
    char pendingChar;

    std::queue<Token> tokenQueue;

    char readChar();
    void setPending(char c);

    static std::map<std::string, TokenType> keywords;
    static std::map<std::string, TokenType> initKeywords();

    static bool isLetter(char c);
    static bool isDigit(char c);
    static bool isWhitespace(char c);
    std::string readUntilLineEndOrEOF();
    std::string trim(const std::string& s);
    Token consumeUnknown(std::string& buffer, int tokLine, int tokCol);

    static TokenType classifyIdent(const std::string& buf);
};

#endif 
