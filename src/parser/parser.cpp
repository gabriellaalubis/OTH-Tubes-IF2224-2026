#include "parser.hpp"
#include <iostream>
#include <sstream>

Parser::Parser(const std::vector<Token>& tokens) : tokens(tokens), pos(0) {}

const Token& Parser::current() const {
    static Token eof_tok = {EOF_TOKEN, "", 0, 0};
    if (pos < tokens.size()) return tokens[pos];
    return eof_tok;
}

const Token& Parser::peek(int offset) const {
    static Token eof_tok = {EOF_TOKEN, "", 0, 0};
    size_t idx = (size_t)((int)pos + offset);
    if (idx < tokens.size()) return tokens[idx];
    return eof_tok;
}

bool Parser::check(TokenType t) const { return current().type == t; }

bool Parser::checkAny(std::initializer_list<TokenType> types) const {
    for (auto t : types) if (check(t)) return true;
    return false;
}

NodePtr Parser::consume() {
    const Token& tok = current();
    std::string lbl = tokenTypetoString(tok.type);
    if (!tok.value.empty()) lbl += "(" + tok.value + ")";
    auto node = std::make_shared<ParseNode>(lbl);
    pos++;
    return node;
}

NodePtr Parser::expect(TokenType t) {
    if (!check(t)) {
        const Token& cur = current();
        std::string got = tokenTypetoString(cur.type);
        if (!cur.value.empty()) got += "(" + cur.value + ")";
        std::string exp = tokenTypetoString(t);
        std::ostringstream oss;
        oss << "Syntax error at line " << cur.line << " col " << cur.col
            << ": unexpected token '" << got << "', expected '" << exp << "'";
        throw SyntaxError(oss.str());
    }
    return consume();
}


void Parser::printTree(const NodePtr& node, std::ostream& out,
                       const std::string& prefix, bool isLast) {
    out << prefix << (isLast ? "└── " : "├── ") << node->label << "\n";
    std::string childPrefix = prefix + (isLast ? "    " : "│   ");
    for (size_t i = 0; i < node->children.size(); ++i) {
        bool last = (i == node->children.size() - 1);
        printTree(node->children[i], out, childPrefix, last);
    }
}

bool Parser::isRelationalOp() const {
    return checkAny({EQL, NEQ, GTR, GEQ, LSS, LEQ});
}
bool Parser::isAdditiveOp() const {
    return checkAny({PLUS, MINUS, ORSY});
}
bool Parser::isMultiplicativeOp() const {
    return checkAny({TIMES, RDIV, IDIV, IMOD, ANDSY});
}

bool Parser::isConstantStart() const {
    return checkAny({CHARCON, STRING, IDENT, INTCON, REALCON, PLUS, MINUS});
}

bool Parser::isTypeStart() const {
    return checkAny({IDENT, ARRAYSY, LPARENT, RECORDSY, INTCON, CHARCON, REALCON});
}


NodePtr Parser::parse() {
    NodePtr root = parseProgram();
    if (!check(EOF_TOKEN)) {
        const Token& cur = current();
        std::ostringstream oss;
        oss << "Syntax error at line " << cur.line
            << ": unexpected token '" << tokenTypetoString(cur.type)
            << "' after end of program";
        throw SyntaxError(oss.str());
    }
    return root;
}


NodePtr Parser::parseProgram() {
    auto node = std::make_shared<ParseNode>("<program>");
    node->children.push_back(parseProgramHeader());
    node->children.push_back(parseDeclarationPart());
    node->children.push_back(parseCompoundStatement());
    node->children.push_back(expect(PERIOD));
    return node;
}

NodePtr Parser::parseProgramHeader() {
    auto node = std::make_shared<ParseNode>("<program-header>");
    node->children.push_back(expect(PROGRAMSY));
    node->children.push_back(expect(IDENT));
    node->children.push_back(expect(SEMICOLON));
    return node;
}



NodePtr Parser::parseDeclarationPart() {
    auto node = std::make_shared<ParseNode>("<declaration-part>");
    while (check(CONSTSY))
        node->children.push_back(parseConstDeclaration());

    while (check(TYPESY))
        node->children.push_back(parseTypeDeclaration());

    while (check(VARSY))
        node->children.push_back(parseVarDeclaration());

    while (check(PROCEDURESY) || check(FUNCTIONSY))
        node->children.push_back(parseSubprogramDeclaration());

    if (checkAny({CONSTSY, TYPESY, VARSY})) {
        const Token& cur = current();
        std::ostringstream oss;
        oss << "Syntax error at line " << cur.line
            << ": declaration order violation — '"
            << tokenTypetoString(cur.type)
            << "' appears out of order"
            << " (required order: const, type, var, subprogram)";
        throw SyntaxError(oss.str());
    }

    return node;
}


NodePtr Parser::parseConstDeclaration() {
    auto node = std::make_shared<ParseNode>("<const-declaration>");
    node->children.push_back(expect(CONSTSY));
    do {
        node->children.push_back(expect(IDENT));
        node->children.push_back(expect(EQL));
        node->children.push_back(parseConstant());
        node->children.push_back(expect(SEMICOLON));
    } while (check(IDENT));
    return node;
}


NodePtr Parser::parseConstant() {
    auto node = std::make_shared<ParseNode>("<constant>");
    if (check(CHARCON) || check(STRING)) {
        node->children.push_back(consume());
    } else {
        if (check(PLUS) || check(MINUS))
            node->children.push_back(consume());
        if (checkAny({IDENT, INTCON, REALCON}))
            node->children.push_back(consume());
        else {
            const Token& cur = current();
            std::ostringstream oss;
            oss << "Syntax error at line " << cur.line
                << ": expected constant value (ident, intcon, realcon, charcon, or string)"
                << ", got '" << tokenTypetoString(cur.type) << "'";
            throw SyntaxError(oss.str());
        }
    }
    return node;
}



NodePtr Parser::parseTypeDeclaration() {
    auto node = std::make_shared<ParseNode>("<type-declaration>");
    node->children.push_back(expect(TYPESY));
    do {
        node->children.push_back(expect(IDENT));
        node->children.push_back(expect(EQL));
        node->children.push_back(parseType());
        node->children.push_back(expect(SEMICOLON));
    } while (check(IDENT));
    return node;
}



NodePtr Parser::parseVarDeclaration() {
    auto node = std::make_shared<ParseNode>("<var-declaration>");
    node->children.push_back(expect(VARSY));
    do {
        node->children.push_back(parseIdentifierList());
        node->children.push_back(expect(COLON));
        node->children.push_back(parseType());
        node->children.push_back(expect(SEMICOLON));
    } while (check(IDENT));
    return node;
}


NodePtr Parser::parseIdentifierList() {
    auto node = std::make_shared<ParseNode>("<identifier-list>");
    node->children.push_back(expect(IDENT));
    while (check(COMMA)) {
        node->children.push_back(consume()); // comma
        node->children.push_back(expect(IDENT));
    }
    return node;
}



NodePtr Parser::parseType() {
    auto node = std::make_shared<ParseNode>("<type>");

    if (check(ARRAYSY)) {
        node->children.push_back(parseArrayType());
    } else if (check(LPARENT)) {
        node->children.push_back(parseEnumerated());
    } else if (check(RECORDSY)) {
        node->children.push_back(parseRecordType());
    } else if (check(IDENT)) {
        if (peek(1).type == PERIOD) {
            node->children.push_back(parseRange());
        } else {
            node->children.push_back(consume()); // ident tunggal
        }
    } else if (checkAny({INTCON, REALCON, CHARCON, PLUS, MINUS})) {
        node->children.push_back(parseRange());
    } else {
        const Token& cur = current();
        std::ostringstream oss;
        oss << "Syntax error at line " << cur.line
            << ": expected type, got '" << tokenTypetoString(cur.type) << "'";
        throw SyntaxError(oss.str());
    }
    return node;
}

NodePtr Parser::parseArrayType() {
    auto node = std::make_shared<ParseNode>("<array-type>");
    node->children.push_back(expect(ARRAYSY));
    node->children.push_back(expect(LBRACK));


    if (check(IDENT) && peek(1).type == PERIOD) {
        node->children.push_back(parseRange());
    } else if (check(IDENT)) {
        node->children.push_back(consume()); 
    } else if (isConstantStart()) {
        node->children.push_back(parseRange());
    } else {
        const Token& cur = current();
        std::ostringstream oss;
        oss << "Syntax error at line " << cur.line
            << ": expected range or type identifier in array index, got '"
            << tokenTypetoString(cur.type) << "'";
        throw SyntaxError(oss.str());
    }

    node->children.push_back(expect(RBRACK));
    node->children.push_back(expect(OFSY));
    node->children.push_back(parseType());
    return node;
}


NodePtr Parser::parseRange() {
    auto node = std::make_shared<ParseNode>("<range>");
    node->children.push_back(parseConstant());
    node->children.push_back(expect(PERIOD));
    node->children.push_back(expect(PERIOD));
    node->children.push_back(parseConstant());
    return node;
}


NodePtr Parser::parseEnumerated() {
    auto node = std::make_shared<ParseNode>("<enumerated>");
    node->children.push_back(expect(LPARENT));
    node->children.push_back(expect(IDENT));
    while (check(COMMA)) {
        node->children.push_back(consume()); // comma
        node->children.push_back(expect(IDENT));
    }
    node->children.push_back(expect(RPARENT));
    return node;
}


NodePtr Parser::parseRecordType() {
    auto node = std::make_shared<ParseNode>("<record-type>");
    node->children.push_back(expect(RECORDSY));
    node->children.push_back(parseFieldList());
    node->children.push_back(expect(ENDSY));
    return node;
}


NodePtr Parser::parseFieldList() {
    auto node = std::make_shared<ParseNode>("<field-list>");
    node->children.push_back(parseFieldPart());
    while (check(SEMICOLON) && peek(1).type == IDENT) {
        node->children.push_back(consume()); // semicolon
        node->children.push_back(parseFieldPart());
    }
    return node;
}


NodePtr Parser::parseFieldPart() {
    auto node = std::make_shared<ParseNode>("<field-part>");
    node->children.push_back(parseIdentifierList());
    node->children.push_back(expect(COLON));
    node->children.push_back(parseType());
    return node;
}



NodePtr Parser::parseSubprogramDeclaration() {
    auto node = std::make_shared<ParseNode>("<subprogram-declaration>");
    if (check(PROCEDURESY)) node->children.push_back(parseProcedureDeclaration());
    else                    node->children.push_back(parseFunctionDeclaration());
    return node;
}


NodePtr Parser::parseProcedureDeclaration() {
    auto node = std::make_shared<ParseNode>("<procedure-declaration>");
    node->children.push_back(expect(PROCEDURESY));
    node->children.push_back(expect(IDENT));
    if (check(LPARENT)) node->children.push_back(parseFormalParameterList());
    node->children.push_back(expect(SEMICOLON));
    node->children.push_back(parseBlock());
    node->children.push_back(expect(SEMICOLON));
    return node;
}


NodePtr Parser::parseFunctionDeclaration() {
    auto node = std::make_shared<ParseNode>("<function-declaration>");
    node->children.push_back(expect(FUNCTIONSY));
    node->children.push_back(expect(IDENT));
    if (check(LPARENT)) node->children.push_back(parseFormalParameterList());
    node->children.push_back(expect(COLON));
    node->children.push_back(expect(IDENT)); // tipe kembalian
    node->children.push_back(expect(SEMICOLON));
    node->children.push_back(parseBlock());
    node->children.push_back(expect(SEMICOLON));
    return node;
}


NodePtr Parser::parseBlock() {
    auto node = std::make_shared<ParseNode>("<block>");
    node->children.push_back(parseDeclarationPart());
    node->children.push_back(parseCompoundStatement());
    return node;
}



NodePtr Parser::parseFormalParameterList() {
    auto node = std::make_shared<ParseNode>("<formal-parameter-list>");
    node->children.push_back(expect(LPARENT));
    node->children.push_back(parseParameterGroup());


    while (check(SEMICOLON)) {
        if (peek(1).type != IDENT) {
            const Token& cur = current();
            std::ostringstream oss;
            oss << "Syntax error at line " << cur.line
                << ": unexpected semicolon in formal-parameter-list"
                << " (trailing semicolon not allowed before ')')";
            throw SyntaxError(oss.str());
        }
        node->children.push_back(consume()); // semicolon
        node->children.push_back(parseParameterGroup());
    }

    node->children.push_back(expect(RPARENT));
    return node;
}

NodePtr Parser::parseParameterGroup() {
    auto node = std::make_shared<ParseNode>("<parameter-group>");
    node->children.push_back(parseIdentifierList());
    node->children.push_back(expect(COLON));
    if (check(ARRAYSY)) node->children.push_back(parseArrayType());
    else                node->children.push_back(expect(IDENT));
    return node;
}



NodePtr Parser::parseCompoundStatement() {
    auto node = std::make_shared<ParseNode>("<compound-statement>");
    node->children.push_back(expect(BEGINSY));
    node->children.push_back(parseStatementList());
    node->children.push_back(expect(ENDSY));
    return node;
}

NodePtr Parser::parseStatementList() {
    auto node = std::make_shared<ParseNode>("<statement-list>");
    node->children.push_back(parseStatement());
    while (check(SEMICOLON)) {
        node->children.push_back(consume()); // semicolon
        node->children.push_back(parseStatement()); // boleh kosong (ε)
    }
    return node;
}



NodePtr Parser::parseStatement() {
    auto node = std::make_shared<ParseNode>("<statement>");

    if (check(BEGINSY)) {
        node->children.push_back(parseCompoundStatement());

    } else if (check(IFSY)) {
        node->children.push_back(parseIfStatement());

    } else if (check(CASESY)) {
        node->children.push_back(parseCaseStatement());

    } else if (check(WHILESY)) {
        node->children.push_back(parseWhileStatement());

    } else if (check(REPEATSY)) {
        node->children.push_back(parseRepeatStatement());

    } else if (check(FORSY)) {
        node->children.push_back(parseForStatement());

    } else if (check(IDENT)) {
        if (peek(1).type == LPARENT) {
            node->children.push_back(parseProcFuncCall());

        } else {
            NodePtr varNode = parseVariable();

            if (check(BECOMES)) {
                node->children.push_back(parseAssignmentStatement(varNode));
            } else {

                auto callNode = std::make_shared<ParseNode>("<procedure/function-call>");
                if (!varNode->children.empty())
                    callNode->children.push_back(varNode->children[0]); 
                else
                    callNode->children.push_back(varNode);
                node->children.push_back(callNode);
            }
        }
    }

    return node;
}


NodePtr Parser::parseAssignmentStatement(NodePtr varNode) {
    auto node = std::make_shared<ParseNode>("<assignment-statement>");
    node->children.push_back(varNode);          // <variable> yang sudah di-parse
    node->children.push_back(expect(BECOMES));
    node->children.push_back(parseExpression());
    return node;
}



NodePtr Parser::parseVariable() {
    auto node = std::make_shared<ParseNode>("<variable>");

    NodePtr identLeaf = expect(IDENT);

    if (check(LBRACK) || check(PERIOD)) {

        auto baseVar = std::make_shared<ParseNode>("<variable>");
        baseVar->children.push_back(identLeaf);


        node->children.push_back(parseComponentVariable(baseVar));
    } else {

        node->children.push_back(identLeaf);
    }

    return node;
}


NodePtr Parser::parseComponentVariable(NodePtr baseVar) {
    NodePtr currentBase = baseVar;

    while (check(LBRACK) || check(PERIOD)) {
        auto compNode = std::make_shared<ParseNode>("<component-variable>");
        compNode->children.push_back(currentBase); 

        if (check(LBRACK)) {
            compNode->children.push_back(consume()); 
            compNode->children.push_back(parseIndexList());
            compNode->children.push_back(expect(RBRACK));
        } else { 
            compNode->children.push_back(consume()); 
            compNode->children.push_back(expect(IDENT)); 
        }


        if (check(LBRACK) || check(PERIOD)) {
            
            auto wrapVar = std::make_shared<ParseNode>("<variable>");
            wrapVar->children.push_back(compNode);
            currentBase = wrapVar;
        } else {
            return compNode;
        }
    }
    return currentBase;
}


NodePtr Parser::parseIndexList() {
    auto node = std::make_shared<ParseNode>("<index-list>");

    if (checkAny({INTCON, CHARCON, IDENT})) {
        node->children.push_back(consume());
    } else {
        const Token& cur = current();
        std::ostringstream oss;
        oss << "Syntax error at line " << cur.line
            << ": expected index (intcon, charcon, or ident), got '"
            << tokenTypetoString(cur.type) << "'";
        throw SyntaxError(oss.str());
    }

    while (check(COMMA)) {
        node->children.push_back(consume()); 
        if (checkAny({INTCON, CHARCON, IDENT})) {
            node->children.push_back(consume());
        } else {
            const Token& cur = current();
            std::ostringstream oss;
            oss << "Syntax error at line " << cur.line
                << ": expected index after comma (intcon, charcon, or ident), got '"
                << tokenTypetoString(cur.type) << "'";
            throw SyntaxError(oss.str());
        }
    }

    return node;
}


NodePtr Parser::parseIfStatement() {
    auto node = std::make_shared<ParseNode>("<if-statement>");
    node->children.push_back(expect(IFSY));
    node->children.push_back(parseExpression());
    node->children.push_back(expect(THENSY));
    node->children.push_back(parseStatement());
    if (check(ELSESY)) {
        node->children.push_back(consume()); 
        node->children.push_back(parseStatement());
    }
    return node;
}

NodePtr Parser::parseCaseStatement() {
    auto node = std::make_shared<ParseNode>("<case-statement>");
    node->children.push_back(expect(CASESY));
    node->children.push_back(parseExpression());
    node->children.push_back(expect(OFSY));
    node->children.push_back(parseCaseBlock());
    node->children.push_back(expect(ENDSY));
    return node;
}


NodePtr Parser::parseCaseBlock() {
    auto node = std::make_shared<ParseNode>("<case-block>");

    node->children.push_back(parseConstant());
    while (check(COMMA)) {
        node->children.push_back(consume()); // comma
        node->children.push_back(parseConstant());
    }
    node->children.push_back(expect(COLON));
    node->children.push_back(parseStatement());

    while (check(SEMICOLON)) {
        if (peek(1).type == ENDSY) {
            node->children.push_back(consume()); 
            break;
        }
        node->children.push_back(consume()); 
        if (isConstantStart()) {
            // Ada case-block berikutnya
            node->children.push_back(parseCaseBlock());
            break; 
        }
        break;
    }
    return node;
}

NodePtr Parser::parseWhileStatement() {
    auto node = std::make_shared<ParseNode>("<while-statement>");
    node->children.push_back(expect(WHILESY));
    node->children.push_back(parseExpression());
    node->children.push_back(expect(DOSY));
    node->children.push_back(parseStatement());
    return node;
}


NodePtr Parser::parseRepeatStatement() {
    auto node = std::make_shared<ParseNode>("<repeat-statement>");
    node->children.push_back(expect(REPEATSY));
    node->children.push_back(parseStatementList());
    node->children.push_back(expect(UNTILSY));
    node->children.push_back(parseExpression());
    return node;
}

NodePtr Parser::parseForStatement() {
    auto node = std::make_shared<ParseNode>("<for-statement>");
    node->children.push_back(expect(FORSY));
    node->children.push_back(expect(IDENT));
    node->children.push_back(expect(BECOMES));
    node->children.push_back(parseExpression());
    if (check(TOSY)) node->children.push_back(consume());
    else if (check(DOWNTOSY)) node->children.push_back(consume());
    else {
        const Token& cur = current();
        std::ostringstream oss;
        oss << "Syntax error at line " << cur.line
            << ": expected 'to' or 'downto' in for-statement, got '"
            << tokenTypetoString(cur.type) << "'";
        throw SyntaxError(oss.str());
    }
    node->children.push_back(parseExpression());
    node->children.push_back(expect(DOSY));
    node->children.push_back(parseStatement());
    return node;
}


NodePtr Parser::parseProcFuncCall() {
    auto node = std::make_shared<ParseNode>("<procedure/function-call>");
    node->children.push_back(expect(IDENT));
    if (check(LPARENT)) {
        node->children.push_back(consume()); 
        if (!check(RPARENT))
            node->children.push_back(parseParameterList());
        node->children.push_back(expect(RPARENT));
    }
    return node;
}

NodePtr Parser::parseParameterList() {
    auto node = std::make_shared<ParseNode>("<parameter-list>");
    node->children.push_back(parseExpression());
    while (check(COMMA)) {
        node->children.push_back(consume()); // comma
        node->children.push_back(parseExpression());
    }
    return node;
}



NodePtr Parser::parseExpression() {
    auto node = std::make_shared<ParseNode>("<expression>");
    node->children.push_back(parseSimpleExpression());
    if (isRelationalOp()) {
        node->children.push_back(parseRelationalOperator());
        node->children.push_back(parseSimpleExpression());
    }
    return node;
}


NodePtr Parser::parseSimpleExpression() {
    auto node = std::make_shared<ParseNode>("<simple-expression>");

    if (check(PLUS) || check(MINUS)) {
        node->children.push_back(consume()); 
    }

    node->children.push_back(parseTerm());

    while (isAdditiveOp()) {
        node->children.push_back(parseAdditiveOperator());
        node->children.push_back(parseTerm());
    }
    return node;
}


NodePtr Parser::parseTerm() {
    auto node = std::make_shared<ParseNode>("<term>");
    node->children.push_back(parseFactor());
    while (isMultiplicativeOp()) {
        node->children.push_back(parseMultiplicativeOperator());
        node->children.push_back(parseFactor());
    }
    return node;
}



NodePtr Parser::parseFactor() {
    auto node = std::make_shared<ParseNode>("<factor>");

    if (check(INTCON) || check(REALCON) || check(CHARCON) || check(STRING)) {
        node->children.push_back(consume());

    } else if (check(IDENT)) {
        if (peek(1).type == LPARENT) {
            node->children.push_back(parseProcFuncCall());
        } else {
            node->children.push_back(parseVariable());
        }

    } else if (check(LPARENT)) {
        node->children.push_back(consume()); 
        node->children.push_back(parseExpression());
        node->children.push_back(expect(RPARENT));

    } else if (check(NOTSY)) {
        node->children.push_back(consume()); 
        node->children.push_back(parseFactor()); 

    } else {
        const Token& cur = current();
        std::ostringstream oss;
        oss << "Syntax error at line " << cur.line << " col " << cur.col
            << ": unexpected token '" << tokenTypetoString(cur.type)
            << "' in expression";
        throw SyntaxError(oss.str());
    }
    return node;
}



NodePtr Parser::parseRelationalOperator() {
    auto node = std::make_shared<ParseNode>("<relational-operator>");
    if (isRelationalOp()) node->children.push_back(consume());
    return node;
}


NodePtr Parser::parseAdditiveOperator() {
    auto node = std::make_shared<ParseNode>("<additive-operator>");
    if (isAdditiveOp()) node->children.push_back(consume());
    return node;
}


NodePtr Parser::parseMultiplicativeOperator() {
    auto node = std::make_shared<ParseNode>("<multiplicative-operator>");
    if (isMultiplicativeOp()) node->children.push_back(consume());
    return node;
}