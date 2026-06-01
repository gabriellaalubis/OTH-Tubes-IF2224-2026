#ifndef INTERPRETER_HPP
#define INTERPRETER_HPP

#include "../codegenerator/codegen.hpp"
#include "../semantic/symbol_table.hpp"
#include <vector>
#include <ostream>
#include <stdexcept>
#include <string>
#include <unordered_map>

class InterpreterError : public std::runtime_error {
public:
    explicit InterpreterError(const std::string& msg)
        : std::runtime_error("[INTERPRETER ERROR] " + msg) {}
};

struct StackVal {
    int  ival = 0;
    bool isStr = false;
    int  sidx = 0;
};

class Interpreter {
public:
    Interpreter(const std::vector<Instruction>& code,
                const SymbolTable& symtab,
                const std::vector<std::string>& stringTable);
    void execute();
    void printOutput(std::ostream& out) const;
    const std::vector<std::string>& getOutput() const { return output_; }

private:
    const std::vector<Instruction>&  code_;
    const SymbolTable&               symtab_;
    const std::vector<std::string>&  stringTable_;

    std::unordered_map<int, int> addrToPsze_;

    static constexpr int STACK_SIZE = 32768;
    StackVal stack_[STACK_SIZE];
    int top_;
    int pc_;
    int base_;

    std::vector<std::string> output_;

    void buildAddrMap();
    StackVal pop();
    void     push(int val);
    void     pushStr(int sidx);
    int      ibase(int level) const;
    std::string valToString(const StackVal& v) const;
};

#endif