#include "interpreter.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <climits>

Interpreter::Interpreter(const std::vector<Instruction>& code,
                         const SymbolTable& symtab,
                         const std::vector<std::string>& stringTable)
    : code_(code), symtab_(symtab), stringTable_(stringTable),
      top_(-1), pc_(0), base_(0), callDepth_(0)
{
    for (int i = 0; i < STACK_SIZE; ++i) stack_[i] = StackVal{};
    buildAddrMap();
}

void Interpreter::buildAddrMap() {
    for (const auto& e : symtab_.tab()) {
        if ((e.obj == ObjClass::PROCEDURE || e.obj == ObjClass::FUNCTION) && e.adr > 0) {
            int btabIdx = e.ref;
            if (btabIdx >= 0 && btabIdx < (int)symtab_.btab().size()) {
                addrToPsze_[e.adr] = symtab_.btab()[btabIdx].psze;
                int fsize = 3 + symtab_.btab()[btabIdx].psze + symtab_.btab()[btabIdx].vsze;
                addrToFsize_[e.adr] = fsize;
            }
        }
    }
}

StackVal Interpreter::pop() {
    if (top_ < 0)
        throw StackUnderflowError("Pop dari stack kosong pada PC=" + std::to_string(pc_));
    return stack_[top_--];
}

void Interpreter::push(int val) {
    if (top_ >= STACK_SIZE - 1)
        throw StackOverflowError("Stack penuh pada PC=" + std::to_string(pc_));
    stack_[++top_] = StackVal{val, false, 0};
}

void Interpreter::pushStr(int sidx) {
    if (top_ >= STACK_SIZE - 1)
        throw StackOverflowError("Stack penuh pada PC=" + std::to_string(pc_));
    stack_[++top_] = StackVal{0, true, sidx};
}

int Interpreter::ibase(int level) const {
    int b = base_;
    while (level > 0) {
        b = stack_[b].ival;
        --level;
    }
    return b;
}

std::string Interpreter::valToString(const StackVal& v) const {
    if (v.isStr) {
        if (v.sidx >= 0 && v.sidx < (int)stringTable_.size())
            return stringTable_[v.sidx];
        return "";
    }
    return std::to_string(v.ival);
}

void Interpreter::checkJumpTarget(int target, const std::string& instr) const {
    if (target < 0 || target >= (int)code_.size())
        throw InvalidJumpError(instr + " ke alamat tidak valid: " + std::to_string(target));
}

void Interpreter::checkArithOverflow(long long result, const std::string& op) const {
    if (result > INT32_MAX_VAL)
        throw OverflowError("Operasi " + op + " menghasilkan nilai " + std::to_string(result) +
                            " melebihi batas maksimum integer (2147483647)");
    if (result < INT32_MIN_VAL)
        throw UnderflowError("Operasi " + op + " menghasilkan nilai " + std::to_string(result) +
                             " di bawah batas minimum integer (-2147483648)");
}

void Interpreter::checkStackSmashing(int addr, int writeLevel) const {
    if (writeLevel > 0) return;
    if (addr == base_) return;
    if (addr == base_ + 1 || addr == base_ + 2)
        throw StackSmashingError("Penulisan ke alamat " + std::to_string(addr) +
                                 " akan menimpa frame control word (DL/RA) pada base=" +
                                 std::to_string(base_) + ")");
}

void Interpreter::checkVariableAccess(int addr, int frameBase, int frameSize) const {
    int frameEnd = frameBase + frameSize - 1;
    if (addr < frameBase + 3 || addr > frameEnd)
        throw IndexOutOfBoundsError("Akses ke alamat " + std::to_string(addr) +
                                    " di luar batas frame [" +
                                    std::to_string(frameBase + 3) + ".." +
                                    std::to_string(frameEnd) + "]");
}

void Interpreter::execute() {
    top_       = -1;
    pc_        = 0;
    base_      = 0;
    callDepth_ = 0;
    for (int i = 0; i < STACK_SIZE; ++i) stack_[i] = StackVal{};

    if (code_.empty()) return;

    while (true) {
        if (pc_ < 0 || pc_ >= (int)code_.size())
            throw InvalidJumpError("PC di luar batas: " + std::to_string(pc_));

        const Instruction& instr = code_[pc_];
        ++pc_;

        switch (instr.op) {

            case OpCode::LIT:
                if (instr.level == 1)
                    pushStr(instr.operand);
                else
                    push(instr.operand);
                break;

            case OpCode::LOD: {
                int b    = ibase(instr.level);
                int addr = b + instr.operand;
                if (addr < 0 || addr >= STACK_SIZE)
                    throw IndexOutOfBoundsError("LOD: alamat tidak valid " + std::to_string(addr));
                if (top_ >= STACK_SIZE - 1)
                    throw StackOverflowError("Stack penuh pada LOD PC=" + std::to_string(pc_-1));
                stack_[++top_] = stack_[addr];
                break;
            }

            case OpCode::STO: {
                StackVal val = pop();
                int b    = ibase(instr.level);
                int addr = b + instr.operand;
                if (addr < 0 || addr >= STACK_SIZE)
                    throw IndexOutOfBoundsError("STO: alamat tidak valid " + std::to_string(addr));
                checkStackSmashing(addr, instr.level);
                stack_[addr] = val;
                break;
            }

            case OpCode::INT: {
                int newTop = base_ + instr.operand - 1;
                if (newTop >= STACK_SIZE)
                    throw StackOverflowError("INT: alokasi frame melebihi batas stack");
                top_ = newTop;
                break;
            }

            case OpCode::JMP:
                checkJumpTarget(instr.operand, "JMP");
                pc_ = instr.operand;
                break;

            case OpCode::JPC: {
                StackVal cond = pop();
                if (cond.ival == 0) {
                    checkJumpTarget(instr.operand, "JPC");
                    pc_ = instr.operand;
                }
                break;
            }

            case OpCode::CAL: {
                if (callDepth_ >= MAX_CALL_DEPTH)
                    throw StackOverflowError("Kedalaman pemanggilan fungsi melebihi batas maksimum (" +
                                             std::to_string(MAX_CALL_DEPTH) + " frame). Kemungkinan infinite recursion.");

                int procAddr = instr.operand;
                int psze = 0;
                auto it = addrToPsze_.find(procAddr);
                if (it != addrToPsze_.end()) psze = it->second;

                int newBase = top_ - psze - 2;
                if (newBase < 0)
                    throw StackUnderflowError("CAL: tidak cukup ruang untuk frame baru (newBase=" +
                                              std::to_string(newBase) + ")");

                stack_[newBase].ival    = ibase(instr.level);
                stack_[newBase].isStr   = false;
                stack_[newBase+1].ival  = base_;
                stack_[newBase+1].isStr = false;
                stack_[newBase+2].ival  = pc_;
                stack_[newBase+2].isStr = false;

                base_ = newBase;
                pc_   = procAddr;
                ++callDepth_;
                break;
            }

            case OpCode::RET: {
                if (base_ == 0) return;

                int retPc   = stack_[base_ + 2].ival;
                int retBase = stack_[base_ + 1].ival;

                if (retPc < 0 || retPc > (int)code_.size())
                    throw StackSmashingError("Return address " + std::to_string(retPc) +
                                             " tidak valid — kemungkinan stack smashing terdeteksi");

                StackVal retVal{};
                bool hasRet = (instr.operand == 1);
                if (hasRet) retVal = stack_[base_];

                int expectedTop = base_ - 1;
                top_  = expectedTop;
                pc_   = retPc;
                base_ = retBase;
                --callDepth_;

                if (hasRet) stack_[++top_] = retVal;
                break;
            }

            case OpCode::OPR: {
                switch (instr.operand) {
                    case 0: break;

                    case 1: {
                        StackVal a = pop();
                        long long res = -(long long)a.ival;
                        checkArithOverflow(res, "NEG");
                        push((int)res);
                        break;
                    }
                    case 2: {
                        StackVal b = pop(); StackVal a = pop();
                        long long res = (long long)a.ival + b.ival;
                        checkArithOverflow(res, "ADD");
                        push((int)res);
                        break;
                    }
                    case 3: {
                        StackVal b = pop(); StackVal a = pop();
                        long long res = (long long)a.ival - b.ival;
                        checkArithOverflow(res, "SUB");
                        push((int)res);
                        break;
                    }
                    case 4: {
                        StackVal b = pop(); StackVal a = pop();
                        long long res = (long long)a.ival * b.ival;
                        checkArithOverflow(res, "MUL");
                        push((int)res);
                        break;
                    }
                    case 5: {
                        StackVal b = pop(); StackVal a = pop();
                        if (b.ival == 0)
                            throw InterpreterError("Pembagian dengan nol pada PC=" + std::to_string(pc_-1));
                        push(a.ival / b.ival);
                        break;
                    }
                    case 6: {
                        StackVal b = pop(); StackVal a = pop();
                        if (b.ival == 0)
                            throw InterpreterError("Modulo dengan nol pada PC=" + std::to_string(pc_-1));
                        push(a.ival % b.ival);
                        break;
                    }
                    case 7:  { StackVal b=pop(); StackVal a=pop(); push(a.ival==b.ival?1:0); break; }
                    case 8:  { StackVal b=pop(); StackVal a=pop(); push(a.ival!=b.ival?1:0); break; }
                    case 9:  { StackVal b=pop(); StackVal a=pop(); push(a.ival< b.ival?1:0); break; }
                    case 10: { StackVal b=pop(); StackVal a=pop(); push(a.ival>=b.ival?1:0); break; }
                    case 11: { StackVal b=pop(); StackVal a=pop(); push(a.ival> b.ival?1:0); break; }
                    case 12: { StackVal b=pop(); StackVal a=pop(); push(a.ival<=b.ival?1:0); break; }

                    case 13: {
                        StackVal val = pop();
                        output_.push_back(valToString(val));
                        break;
                    }
                    case 14: {
                        StackVal val = pop();
                        output_.push_back(valToString(val) + "\n");
                        break;
                    }

                    default:
                        throw InterpreterError("OPR: operand tidak dikenal " + std::to_string(instr.operand));
                }
                break;
            }

            default:
                throw InterpreterError("Instruksi tidak dikenal pada PC=" + std::to_string(pc_ - 1));
        }
    }
}

void Interpreter::printOutput(std::ostream& out) const {
    for (const auto& s : output_) {
        out << s;
    }
}