#include "lexer/lexer.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
    string inputPath;
    cout << "ARION LEXICAL ANALYZER" << endl;
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

    fs::path outDir = "test/milestone-1";
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

        std::cout << "Path file input : " << inputPath << std::endl;
        std::cout << "Path file output: " << outputPath << std::endl;
        std::cout << std::string(40, '-') << std::endl;

        while (!lexer.isEOF()) {
            Token tok = lexer.nextToken();

            if (tok.type == EOF_TOKEN) break;

            std::string formatted = Lexer::formatToken(tok);

            std::cout << formatted << std::endl;

            outFile << formatted << std::endl;
        }

        std::cout << std::string(40, '-') << std::endl;
        std::cout << "Lexical analysis selesai. Output disimpan di " << outputPath << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    outFile.close();
    return 0;
}