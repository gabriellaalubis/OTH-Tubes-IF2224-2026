#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "semantic/semantic.hpp"
#include "codegenerator/codegen.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

void printHeader(const std::string& title) {
    std::string border(58, '=');
    std::cout << "\n" << border << "\n";
    int padding = (58 - (int)title.size()) / 2;
    if (padding < 0) padding = 0;
    std::cout << std::string(padding, ' ') << title << "\n";
    std::cout << border << "\n";
}

void printSeparator() {
    std::cout << std::string(58, '-') << "\n";
}

int main(int argc, char* argv[]) {
    std::string inputPath;

    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════╗\n";
    std::cout << "║                    ARION INTERPRETER                     ║\n";
    std::cout << "║  Lexical + Syntax + Semantic + Intermediate Code Gen     ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════╝\n";
    std::cout << "\n";
    std::cout << "Masukkan path file input (.txt): ";
    std::getline(std::cin, inputPath);
    std::cout << "\n";

    while (!inputPath.empty() &&
           (inputPath.front() == ' ' || inputPath.front() == '\t'))
        inputPath.erase(inputPath.begin());
    while (!inputPath.empty() &&
           (inputPath.back() == ' ' || inputPath.back() == '\t' ||
            inputPath.back() == '\r'))
        inputPath.pop_back();

    if (!fs::exists(inputPath)) {
        std::cerr << "[ERROR]: File tidak ditemukan: " << inputPath << "\n";
        return 1;
    }
    if (fs::path(inputPath).extension() != ".txt") {
        std::cerr << "[ERROR]: File harus bertipe .txt\n";
        return 1;
    }

    fs::path outDirSemantic = "test/milestone-3";
    fs::path outDirCodegen  = "test/milestone-4";
    fs::create_directories(outDirSemantic);
    fs::create_directories(outDirCodegen);

    std::string stem = fs::path(inputPath).stem().string();
    fs::path outputSemantic = outDirSemantic / ("output-" + stem + ".txt");
    fs::path outputCodegen  = outDirCodegen  / ("output-" + stem + ".txt");

    try {
        printHeader("LEXICAL ANALYSIS");
        Lexer lexer(inputPath);
        std::vector<Token> tokens;
        int tokenCount = 0;
        while (!lexer.isEOF()) {
            Token tok = lexer.nextToken();
            if (tok.type == EOF_TOKEN) { tokens.push_back(tok); break; }
            std::string formatted = Lexer::formatToken(tok);
            std::cout << formatted << "\n";
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
        {
            std::ofstream outSem(outputSemantic);
            semantic.printResults(outSem);
        }
        printSeparator();
        std::cout << "  Status      : Semantic analysis selesai\n";
        std::cout << "  Output      : " << outputSemantic << "\n";
        if (!semantic.getWarnings().empty()) {
            std::cout << "\n  [WARNINGS]\n";
            for (auto& w : semantic.getWarnings())
                std::cout << "    " << w.message << "\n";
        }

        printHeader("INTERMEDIATE CODE GENERATION");
        CodeGenerator codegen(ast, semantic.getSymbolTable());
        codegen.generate();
        codegen.printCode(std::cout);
        {
            std::ofstream outCG(outputCodegen);
            codegen.printCode(outCG);
            outCG << "\n";
            semantic.printResults(outCG);
        }
        printSeparator();
        std::cout << "  Total instruksi : "
                  << codegen.getInstructions().size() << " instruksi\n";
        std::cout << "  Status          : Code generation selesai\n";
        std::cout << "  Output          : " << outputCodegen << "\n";

    } catch (const SyntaxError& e) {
        std::cerr << "\n*** " << e.what() << "\n";
        return 1;
    } catch (const SemanticError& e) {
        std::cerr << "\n*** " << e.what() << "\n";
        { std::ofstream outSem(outputSemantic); outSem << e.what() << "\n"; }
        return 1;
    } catch (const CodeGenError& e) {
        std::cerr << "\n*** " << e.what() << "\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "[ERROR]: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
