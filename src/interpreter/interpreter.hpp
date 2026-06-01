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

class StackOverflowError : public InterpreterError {
public:
    explicit StackOverflowError(const std::string& msg)
        : InterpreterError("StackOverflowError: " + msg) {}
};

class StackUnderflowError : public InterpreterError {
public:
    explicit StackUnderflowError(const std::string& msg)
        : InterpreterError("StackUnderflowError: " + msg) {}
};

class StackSmashingError : public InterpreterError {
public:
    explicit StackSmashingError(const std::string& msg)
        : InterpreterError("StackSmashingError: " + msg) {}
};

class StackCorruptionError : public InterpreterError {
public:
    explicit StackCorruptionError(const std::string& msg)
        : InterpreterError("StackCorruptionError: " + msg) {}
};

class IndexOutOfBoundsError : public InterpreterError {
public:
    explicit IndexOutOfBoundsError(const std::string& msg)
        : InterpreterError("IndexOutOfBoundsException: " + msg) {}
};

class InvalidJumpError : public InterpreterError {
public:
    explicit InvalidJumpError(const std::string& msg)
        : InterpreterError("InvalidJumpTarget: " + msg) {}
};

class OverflowError : public InterpreterError {
public:
    explicit OverflowError(const std::string& msg)
        : InterpreterError("OverflowError: " + msg) {}
};

class UnderflowError : public InterpreterError {
public:
    explicit UnderflowError(const std::string& msg)
        : InterpreterError("UnderflowError: " + msg) {}
};

struct StackVal {
    int  ival  = 0;
    bool isStr = false;
    int  sidx  = 0;
};

class Interpreter {
public:
    Interpreter(const std::vector<Instruction>& code,
                const SymbolTable& symtab,
                const std::vector<std::string>& stringTable);
    void execute();
    void printOutput(std::ostream& out) const;
    const std::vector<std::string>& getOutput() const { return output_; }

    static constexpr int MAX_CALL_DEPTH = 1000;

private:
    const std::vector<Instruction>&  code_;
    const SymbolTable&               symtab_;
    const std::vector<std::string>&  stringTable_;

    std::unordered_map<int, int> addrToPsze_;
    std::unordered_map<int, int> addrToFsize_;

    static constexpr int STACK_SIZE = 32768;
    static constexpr int INT32_MAX_VAL =  2147483647;
    static constexpr int INT32_MIN_VAL = -2147483648;

    StackVal stack_[STACK_SIZE];
    int top_;
    int pc_;
    int base_;
    int callDepth_;

    std::vector<std::string> output_;

    void buildAddrMap();
    StackVal pop();
    void     push(int val);
    void     pushStr(int sidx);
    int      ibase(int level) const;
    std::string valToString(const StackVal& v) const;

    void checkJumpTarget(int target, const std::string& instr) const;
    void checkArithOverflow(long long result, const std::string& op) const;
    void checkStackSmashing(int addr, int writeLevel) const;
    void checkVariableAccess(int addr, int frameBase, int frameSize) const;
};

#endif