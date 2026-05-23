#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "semantic/semantic.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

void printHeader(const std::string& title) {
    std::string border(50, '=');
    std::cout << "\n" << border << "\n";
    int padding = (50 - (int)title.size()) / 2;
    std::cout << std::string(padding, ' ') << title << "\n";
    std::cout << border << "\n";
}

void printSeparator() {
    std::cout << std::string(50, '-') << "\n";
}


int main(int argc, char* argv[]) {
    string inputPath;
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════╗\n";
    std::cout << "║                      ARION COMPILER                      ║\n";
    std::cout << "║  Lexical Analyzer + Syntax Analyzer + Semantic Analyzer  ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════╝\n";
    std::cout << "\n";
    cout << "Masukkan path file input (.txt): ";
    getline(cin, inputPath);
    cout << endl;

    while (!inputPath.empty() && (inputPath.front() == ' ' || inputPath.front() == '\t'))
        inputPath.erase(inputPath.begin());
    while (!inputPath.empty() && (inputPath.back() == ' ' || inputPath.back() == '\t' || inputPath.back() == '\r'))
        inputPath.pop_back();

    if (!fs::exists(inputPath)) {
        std::cerr << "[ERROR]: File tidak ditemukan: " << inputPath << endl;
        return 1;
    }

    if (fs::path(inputPath).extension() != ".txt") {
        std::cerr << "[ERROR]: File harus bertipe .txt" << endl;
        return 1;
    }

    fs::path outDirSemantic = "test/milestone-3";
    fs::create_directories(outDirSemantic);

    std::string stem = fs::path(inputPath).stem().string();
    fs::path outputSemantic = outDirSemantic / ("output-" + stem + ".txt");

    try {
        Lexer lexer(inputPath);
        std::vector<Token> tokens;

        printHeader("LEXICAL ANALYSIS");
        int tokenCount = 0;

        while (!lexer.isEOF()) {
            Token tok = lexer.nextToken();

            if (tok.type == EOF_TOKEN) {
                tokens.push_back(tok);
                break;
            }

            std::string formatted = Lexer::formatToken(tok);
            std::cout << formatted << std::endl;

            if (tok.type == COMMENT) continue;
            tokens.push_back(tok);
            tokenCount++;
        }

        printSeparator();
        std::cout << "  Total token : " << tokenCount << " token\n";
        std::cout << "  Status      : Lexical analysis selesai\n";

        printHeader("SYNTAX ANALYSIS (PARSE TREE)");

        Parser parser(tokens);
        NodePtr tree = parser.parse();

        Parser::printTree(tree, std::cout);

        printSeparator();
        std::cout << "  Status      : Syntax analysis selesai\n";

        printHeader("SEMANTIC ANALYSIS (DECORATED AST)");

        SemanticAnalyser semantic;
        ASTPtr ast = semantic.analyse(tree);

        semantic.printResults(std::cout);

        std::ofstream outSemantic(outputSemantic);
        semantic.printResults(outSemantic);
        outSemantic.close();

        printSeparator();
        std::cout << "  Status      : Semantic analysis selesai\n";
        std::cout << "  Output      : " << outputSemantic << "\n";

        if (!semantic.getWarnings().empty()) {
            std::cout << "\n  [WARNINGS]\n";
            for (auto& w : semantic.getWarnings())
                std::cout << "  " << w.message << "\n";
        }

    } catch (const SyntaxError& e) {
        std::cout << std::endl << "*** " << e.what() << std::endl;
        return 1;

    } catch (const SemanticError& e) {
        std::cout << std::endl << "*** " << e.what() << std::endl;

        std::ofstream outSemantic(outputSemantic);
        outSemantic << e.what() << "\n";
        outSemantic.close();
        return 1;

    } catch (const std::exception& e) {
        std::cerr << "[ERROR]: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}