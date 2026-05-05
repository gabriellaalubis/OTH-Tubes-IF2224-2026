#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
    string inputPath;
    cout << "ARION LEXICAL AND PARSER ANALYZER" << endl;
    cout << "Masukkan path file input (.txt): ";
    getline(cin, inputPath);
    cout << endl;

    while (!inputPath.empty() && (inputPath.front() == ' ' || inputPath.front() == '\t'))
        inputPath.erase(inputPath.begin());
    while (!inputPath.empty() && (inputPath.back() == ' ' || inputPath.back() == '\t' || inputPath.back() == '\r'))
        inputPath.pop_back();

    if (!fs::exists(inputPath)) {
        std::cerr << "Error: File tidak ditemukan: " << inputPath << endl;
        return 1;
    }

    if (fs::path(inputPath).extension() != ".txt") {
        std::cerr << "Error: File harus bertipe .txt" << endl;
        return 1;
    }

    fs::path outDir = "test/milestone-2";
    fs::create_directories(outDir); 

    std::string stem    = fs::path(inputPath).stem().string();
    fs::path outputPath = outDir / ("output-" + stem + ".txt");

    std::ofstream outFile(outputPath);
    if (!outFile.is_open()) {
        std::cerr << "Error: Tidak dapat membuka file output: " << outputPath << std::endl;
        return 1;
    }

    try {
        Lexer lexer(inputPath);
        std::vector<Token> tokens;
        std::cout << "Path file input : " << inputPath << std::endl;
        std::cout << "Path file output: " << outputPath << std::endl;
        std::cout << std::string(40, '-') << std::endl;

        while (!lexer.isEOF()) {
            Token tok = lexer.nextToken();

            if (tok.type == EOF_TOKEN) {
                tokens.push_back(tok);
                break;
            }
            
            if (tok.type == COMMENT) continue;

            std::string formatted = Lexer::formatToken(tok);

            std::cout << formatted << std::endl;

            outFile << formatted << std::endl;

            tokens.push_back(tok);
        }

        std::cout << std::string(40, '-') << std::endl;
        std::cout << "Lexical analysis selesai. Output disimpan di " << outputPath << std::endl;


        std::cout << std::endl;
        std::cout << "=== PARSE TREE ===" << std::endl;
        std::cout << std::string(40, '-') << std::endl;
    
        Parser parser(tokens);
        NodePtr tree = parser.parse();
    
        Parser::printTree(tree, std::cout);
    
        outFile.close();
        outFile.open(outputPath);
        Parser::printTree(tree, outFile);
    
        std::cout << std::string(40, '-') << std::endl;
        std::cout << "Syntax analysis selesai. Output disimpan di " << outputPath << std::endl;

    } catch (const SyntaxError& e) {
        std::cerr << std::endl << "*** " << e.what() << std::endl;
        outFile.close();
        return 1;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
 
    outFile.close();
    return 0;
}