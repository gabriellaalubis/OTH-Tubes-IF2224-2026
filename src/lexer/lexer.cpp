#include "lexer.hpp"
#include <iostream>
#include <algorithm>
#include <cctype>
#include <stdexcept>

std::map<std::string, TokenType> Lexer::keywords = Lexer::initKeywords();

std::map<std::string, TokenType> Lexer::initKeywords() {
    std::map<std::string, TokenType> kw;

    kw["not"]       = NOTSY;
    kw["div"]       = IDIV;
    kw["mod"]       = IMOD;
    kw["and"]       = ANDSY;
    kw["or"]        = ORSY;
    kw["const"]     = CONSTSY;
    kw["type"]      = TYPESY;
    kw["var"]       = VARSY;
    kw["function"]  = FUNCTIONSY;
    kw["procedure"] = PROCEDURESY;
    kw["array"]     = ARRAYSY;
    kw["record"]    = RECORDSY;
    kw["program"]   = PROGRAMSY;
    kw["begin"]     = BEGINSY;
    kw["if"]        = IFSY;
    kw["case"]      = CASESY;
    kw["repeat"]    = REPEATSY;
    kw["while"]     = WHILESY;
    kw["for"]       = FORSY;
    kw["end"]       = ENDSY;
    kw["else"]      = ELSESY;
    kw["until"]     = UNTILSY;
    kw["of"]        = OFSY;
    kw["do"]        = DOSY;
    kw["to"]        = TOSY;
    kw["downto"]    = DOWNTOSY;
    kw["then"]      = THENSY;

    return kw;
}

Lexer::Lexer(const std::string& filename)
    : currentLine(1), currentCol(0),
    hasPending(false), pendingChar('\0'),
    state(S0)
{
    inputFile.open(filename);
    if (!inputFile.is_open()) {
        throw std::runtime_error("Cannot open input file: " + filename);
    }
}

Lexer::~Lexer() {
    if (inputFile.is_open()) inputFile.close();
}

bool Lexer::isEOF() const {
    // EOF kalo ga ada pending char dan stream habis
    return !hasPending && (!inputFile.good() || inputFile.eof());
}

vector<Token> Lexer::tokenizeAll() {
    vector<Token> tokens;
    while (true) {
        Token tok = nextToken();
        if (tok.type == EOF_TOKEN) break;
        if (tok.type == COMMENT) continue;
        tokens.push_back(tok);
    }
    return tokens;
}

char Lexer::readChar() {
    char c;

    if (hasPending) {
        c = pendingChar;
        hasPending = false;
    } else {
        if (!inputFile.get(c)) {
            return (char)EOF;
        }
    }

    if (c == '\n') {
        currentLine++;
        currentCol = 0;
    } else {
        currentCol++;
    }
    return c;
}

void Lexer::setPending(char c) {
    if (c == (char)EOF) return;
    hasPending  = true;
    pendingChar = c;
    if (c == '\n') {
        currentLine--;
    } else {
        currentCol--;
    }
}

bool Lexer::isLetter(char c)     { return std::isalpha((unsigned char)c) != 0; }
bool Lexer::isDigit(char c)      { return std::isdigit((unsigned char)c) != 0; }
bool Lexer::isWhitespace(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }
std::string Lexer::readUntilLineEndOrEOF() {
    std::string extra = "";
    while (true) {
        char nc = readChar();
        if (nc == (char)EOF || nc == '\n') {
            if (nc == '\n') setPending(nc);
            break;
        }
        extra += nc;
    }
    return extra;
}

std::string Lexer::trim(const std::string& s) {
    size_t l = 0, r = s.size();
    while (l < r && std::isspace((unsigned char)s[l])) l++;
    while (r > l && std::isspace((unsigned char)s[r - 1])) r--;
    return s.substr(l, r - l);
}

Token Lexer::consumeUnknown(std::string& buffer, int tokLine, int tokCol) {
    while (true) {
        char nc = readChar();
        if (nc == (char)EOF || isWhitespace(nc)) {
            if (isWhitespace(nc)) setPending(nc);
            break;
        }
        if (nc == ';' || nc == ',' || nc == '(' || nc == ')' ||
            nc == '[' || nc == ']' || nc == '\'' || nc == '{') {
            setPending(nc);
            break;
        }
        buffer += nc;
    }
    return {UNKNOWN, buffer, tokLine, tokCol};
}

std::string Lexer::formatToken(const Token& tok) {
    std::string name = tokenTypetoString(tok.type);
    bool isLiteral = (tok.type == STRING || tok.type == CHARCON ||
                        tok.type == INTCON || tok.type == REALCON ||
                        tok.type == IDENT   || tok.type == UNKNOWN ||
                        tok.type == COMMENT);
    if (!tok.value.empty() || isLiteral) {
        return name + "(" + tok.value + ")";
    }
    return name;
}


// cek apakah buffer adalah keyword. kalau iya return TokenType keyword, kalau ngga return IDENT
TokenType Lexer::classifyIdent(const std::string& buf) {
    std::string lower = buf;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    auto it = keywords.find(lower);
    if (it != keywords.end()) return it->second;
    return IDENT;
}


Token Lexer::nextToken() {
    if (!tokenQueue.empty()) {
        Token t = tokenQueue.front();
        tokenQueue.pop();
        return t;
    }

    std::string buffer = "";
    state = S0;

    int tokLine = currentLine;
    int tokCol  = currentCol + 1;

    while (true) {
        char c = readChar();

        if (state == S0 && !isWhitespace(c) && c != (char)EOF) {
            tokLine = currentLine;
            tokCol  = currentCol;
        }

        switch (state) {

        case S0: {
            if (c == (char)EOF) {
                return {EOF_TOKEN, "", tokLine, tokCol};
            }

            if (isWhitespace(c)) {
                break; 
            }

            if (isDigit(c)) {
                buffer += c;
                state = S_INT;
                break;
            }

            if (isLetter(c)) {
                buffer += c;
                state = S_IDENT;
                break;
            }

            if (c == '-') {
                buffer += c;
                state = S_MINUS;
                break;
            }

            if (c == '+') {
                return {PLUS, "", tokLine, tokCol};
            }

            if (c == '\'') {
                state = S_STR_OPEN;
                break;
            }

            if (c == '{') {
                state = S_CMT_BRACE;
                break;
            }

            if (c == '(') {
                state = S_LPAREN;
                break;
            }

            if (c == ':') {
                state = S_COLON;
                break;
            }

            if (c == '<') {
                state = S_LT;
                break;
            }

            if (c == '>') {
                state = S_GT;
                break;
            }

            if (c == '=') {
                state = S_EQL;
                break;
            }

            // single char
            if (c == '*') return {TIMES,     "", tokLine, tokCol};
            if (c == '/') return {RDIV,      "", tokLine, tokCol};
            if (c == ')') return {RPARENT,   "", tokLine, tokCol};
            if (c == '[') return {LBRACK,    "", tokLine, tokCol};
            if (c == ']') return {RBRACK,    "", tokLine, tokCol};
            if (c == ',') return {COMMA,     "", tokLine, tokCol};
            if (c == ';') return {SEMICOLON, "", tokLine, tokCol};
            if (c == '.') {
                char nc = readChar();
                if (nc == (char)EOF || isWhitespace(nc)) {
                    if (isWhitespace(nc)) setPending(nc);
                    return {PERIOD, "", tokLine, tokCol};
                }
                if (nc == '.') {
                    setPending(nc);
                    return {PERIOD, "", tokLine, tokCol};
                }
                if (isLetter(nc) || isDigit(nc)) {
                    setPending(nc);
                    return {PERIOD, "", tokLine, tokCol};
                }
                std::string buf = ".";
                buf += nc;
                return consumeUnknown(buf, tokLine, tokCol);
            }

            // Karakter ga dikenal
            return {UNKNOWN, std::string(1, c), tokLine, tokCol};
        }

        case S_MINUS: {
            if (isDigit(c)) {
                buffer += c;
                state = S_INT_NEG;
            } else {
                setPending(c);
                return {MINUS, "", tokLine, tokCol};
            }
            break;
        }

        case S_INT_NEG: {
            if (isDigit(c)) {
                buffer += c;
            } else if (c == '.') {
                state = S_INT_NEG_DOT;
            } else {
                setPending(c);
                return {INTCON, buffer, tokLine, tokCol};
            }
            break;
        }

        case S_INT_NEG_DOT: {
            if (isDigit(c)) {
                buffer += '.';
                buffer += c;
                state = S_REAL_NEG;
            } else {
                setPending(c);
                tokenQueue.push({PERIOD, "", tokLine, tokCol});
                return {INTCON, buffer, tokLine, tokCol};
            }
            break;
        }

        case S_REAL_NEG: {
            if (isDigit(c)) {
                buffer += c;
            } else {
                setPending(c);
                return {REALCON, buffer, tokLine, tokCol};
            }
            break;
        }

        case S_INT: {
            if (isDigit(c)) {
                buffer += c;
            } else if (c == '.') {
                state = S_INT_DOT;
            } else {

                setPending(c);
                return {INTCON, buffer, tokLine, tokCol};
            }
            break;
        }

        case S_INT_DOT: {
            if (isDigit(c)) {
                buffer += '.';
                buffer += c;
                state = S_REAL;
            } else if (c == '.') {
                tokenQueue.push({PERIOD, "", tokLine, tokCol});
                tokenQueue.push({PERIOD, "", tokLine, tokCol});
                return {INTCON, buffer, tokLine, tokCol};
            } else {
                setPending(c);
                tokenQueue.push({PERIOD, "", tokLine, tokCol});
                return {INTCON, buffer, tokLine, tokCol};
            }
            break;
        }

        case S_REAL: {
            if (isDigit(c)) {
                buffer += c;
            } else {
                setPending(c);
                return {REALCON, buffer, tokLine, tokCol};
            }
            break;
        }

        case S_IDENT: {
            if (isLetter(c)) {
                buffer += c;
            } else if (isDigit(c)) {
                setPending(c);
                TokenType tt = classifyIdent(buffer);
                if (tt == IDENT) return {IDENT, buffer, tokLine, tokCol};
                else {
                    return {tt, "", tokLine, tokCol};
                }
            } else {
                setPending(c);
                TokenType tt = classifyIdent(buffer);
                if (tt == IDENT) return {IDENT, buffer, tokLine, tokCol};
                else {
                    return {tt, "", tokLine, tokCol};
                }
            }
            break;
        }

        // baca kutip pertama
        case S_STR_OPEN: {
            if (c == (char)EOF || c == '\n') {
                return {UNKNOWN, "'", tokLine, tokCol};
            }

            if (c == '\'') {
                // bisa '' (string kosong) atau '''' (charcon kutip)
                state = S_STR_QUOTE;
                break;
            }

            buffer += c;
            state = S_STR_BODY;
            break;
        }


        case S_STR_BODY: {
            if (c == (char)EOF || c == '\n') {
                return {UNKNOWN, "'" + buffer, tokLine, tokCol};
            }

            if (c == '\'') {
                state = S_STR_QUOTE;
            } else {
                buffer += c;
            }
            break;
        }


        case S_STR_QUOTE: {
            if (c == '\'') {
                buffer += '\'';
                state = S_STR_BODY;
            } else {
                setPending(c);
                if (buffer.empty()) {
                    return {STRING, "", tokLine, tokCol};  // '' -> string kosong
                } else if (buffer.size() == 1) {
                    return {CHARCON, buffer, tokLine, tokCol};
                } else {
                    return {STRING, buffer, tokLine, tokCol};
                }
            }
            break;
        }

        case S_COLON: {
            if (c == '=') {
                return {BECOMES, "", tokLine, tokCol};
            } else {
                setPending(c);
                return {COLON, "", tokLine, tokCol};
            }
        }

        case S_LT: {
            if (c == '=') return {LEQ, "", tokLine, tokCol};
            if (c == '>') return {NEQ, "", tokLine, tokCol};
            setPending(c);
            return {LSS, "", tokLine, tokCol};
        }


        case S_GT: {
            if (c == '=') return {GEQ, "", tokLine, tokCol};
            setPending(c);
            return {GTR, "", tokLine, tokCol};
        }

        case S_EQL: {
            if (c == '=') return {EQL, "", tokLine, tokCol};
            setPending(c);
            return {UNKNOWN, "=", tokLine, tokCol};
        }

        case S_LPAREN: {
            if (c == '*') {
                state = S_CMT_PAREN; 
            } else {
                setPending(c);
                return {LPARENT, "", tokLine, tokCol};
            }
            break;
        }

        //comment
        case S_CMT_BRACE: {
            if (c == (char)EOF) {
                return {UNKNOWN, "{" + buffer, tokLine, tokCol};
            }

            if (c == '}') {
                return {COMMENT, trim(buffer), tokLine, tokCol};
            }

            if (c == '*') {
                state = S_CMT_BRACE_STAR;
            } else {
                buffer += c;
            }
            break;
        }

        case S_CMT_BRACE_STAR: {
            if (c == (char)EOF) {
                return {UNKNOWN, "{" + buffer + "*", tokLine, tokCol};
            }

            if (c == ')') {
                // komentar yang dibuka dengan { boleh ditutup dengan *)
                return {COMMENT, trim(buffer), tokLine, tokCol};
            }

            if (c == '}') {
                buffer += '*';
                return {COMMENT, trim(buffer), tokLine, tokCol};
            }

            if (c == '*') {
                buffer += '*';
            } else {
                buffer += '*';
                buffer += c;
                state = S_CMT_BRACE;
            }
            break;
        }

        case S_CMT_PAREN: {
            if (c == (char)EOF) {
                return {UNKNOWN, "(*" + buffer, tokLine, tokCol};
            }

            if (c == '}') {
                return {COMMENT, trim(buffer), tokLine, tokCol};
            }

            if (c == '*') {
                state = S_CMT_PAREN_STAR;
            } else {
                buffer += c;
            }
            break;
        }

        case S_CMT_PAREN_STAR: {
            if (c == (char)EOF) {
                return {UNKNOWN, "(*" + buffer + "*", tokLine, tokCol};
            }

            if (c == ')') {
                return {COMMENT, trim(buffer), tokLine, tokCol};
            }

            if (c == '}') {
                buffer += '*';
                return {COMMENT, trim(buffer), tokLine, tokCol};
            }

            if (c == '*') {
                buffer += '*';
            } else {
                buffer += '*';
                buffer += c;
                state = S_CMT_PAREN;
            }
            break;
        }

        case S_DEAD:
        default:
            return {UNKNOWN, buffer, tokLine, tokCol};

        } 
    } 
}