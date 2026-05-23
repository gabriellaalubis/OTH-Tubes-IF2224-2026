#include "symbol_table.hpp"
#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <algorithm>


std::string objClassToString(ObjClass oc) {
    switch (oc) {
        case ObjClass::CONSTANT:  return "constant";
        case ObjClass::VARIABLE:  return "variable";
        case ObjClass::TYPE_NAME: return "type";
        case ObjClass::PROCEDURE: return "procedure";
        case ObjClass::FUNCTION:  return "function";
        case ObjClass::PROGRAM:   return "program";
        case ObjClass::FIELD:     return "field";
        case ObjClass::PARAMETER: return "parameter";
        default:                  return "?";
    }
}


SymbolTable::SymbolTable() {
    initPredefined();
    BTabEntry globalBlock;
    btab_.push_back(globalBlock); // btab[0] = global block
    display_.push_back(0);  // display[0] = btab[0]
}


void SymbolTable::initPredefined() {
    tab_.clear();

    // 0: sentinel / dummy
    {
        TabEntry e; e.name = ""; e.obj = ObjClass::VARIABLE;
        e.type = DataType::NOTYPE; e.lev = 0; e.adr = 0; e.link = 0;
        tab_.push_back(e);
    }

    // 1: Integer
    {
        TabEntry e; e.name = "integer"; e.obj = ObjClass::TYPE_NAME;
        e.type = DataType::INTEGER; e.lev = 0; e.adr = 0; e.link = 0;
        e.initialized = true;
        tab_.push_back(e);
    }
    // 2: Real
    {
        TabEntry e; e.name = "real"; e.obj = ObjClass::TYPE_NAME;
        e.type = DataType::REAL; e.lev = 0; e.adr = 0; e.link = 0;
        e.initialized = true;
        tab_.push_back(e);
    }
    // 3: Char
    {
        TabEntry e; e.name = "char"; e.obj = ObjClass::TYPE_NAME;
        e.type = DataType::CHAR; e.lev = 0; e.adr = 0; e.link = 0;
        e.initialized = true;
        tab_.push_back(e);
    }
    // 4: Boolean
    {
        TabEntry e; e.name = "boolean"; e.obj = ObjClass::TYPE_NAME;
        e.type = DataType::BOOLEAN; e.lev = 0; e.adr = 0; e.link = 0;
        e.initialized = true;
        tab_.push_back(e);
    }
    // 5: String
    {
        TabEntry e; e.name = "string"; e.obj = ObjClass::TYPE_NAME;
        e.type = DataType::STRING; e.lev = 0; e.adr = 0; e.link = 0;
        e.initialized = true;
        tab_.push_back(e);
    }
    // 6: True
    {
        TabEntry e; e.name = "true"; e.obj = ObjClass::CONSTANT;
        e.type = DataType::BOOLEAN; e.lev = 0; e.adr = 1; e.link = 0;
        e.initialized = true;
        tab_.push_back(e);
    }
    // 7: False
    {
        TabEntry e; e.name = "false"; e.obj = ObjClass::CONSTANT;
        e.type = DataType::BOOLEAN; e.lev = 0; e.adr = 0; e.link = 0;
        e.initialized = true;
        tab_.push_back(e);
    }
    // 8: writeln
    {
        TabEntry e; e.name = "writeln"; e.obj = ObjClass::PROCEDURE;
        e.type = DataType::VOID; e.lev = 0; e.adr = 0; e.link = 0;
        e.initialized = true;
        tab_.push_back(e);
    }
    // 9: readln
    {
        TabEntry e; e.name = "readln"; e.obj = ObjClass::PROCEDURE;
        e.type = DataType::VOID; e.lev = 0; e.adr = 0; e.link = 0;
        e.initialized = true;
        tab_.push_back(e);
    }
    // 10: write
    {
        TabEntry e; e.name = "write"; e.obj = ObjClass::PROCEDURE;
        e.type = DataType::VOID; e.lev = 0; e.adr = 0; e.link = 0;
        e.initialized = true;
        tab_.push_back(e);
    }
    // 11: read
    {
        TabEntry e; e.name = "read"; e.obj = ObjClass::PROCEDURE;
        e.type = DataType::VOID; e.lev = 0; e.adr = 0; e.link = 0;
        e.initialized = true;
        tab_.push_back(e);
    }

    // 12-32: padding dummy supaya user identifier mulai dari indeks 33
    for (int i = 12; i < TAB_USER_START; ++i) {
        TabEntry e; e.name = ""; e.obj = ObjClass::VARIABLE;
        e.type = DataType::NOTYPE; e.lev = 0; e.adr = 0; e.link = 0;
        tab_.push_back(e);
    }
}

int SymbolTable::enterBlock() {
    BTabEntry newBlock;
    // Blok baru mulai kosong, outer scope dicek lewat display_.
    newBlock.last = 0;
    btab_.push_back(newBlock);
    int newIdx = (int)btab_.size() - 1;
    display_.push_back(newIdx);
    return newIdx;
}


void SymbolTable::leaveBlock() {
    if (display_.size() <= 1) return; 
    display_.pop_back();
}

int SymbolTable::addTab(const std::string& name, ObjClass obj,
                        DataType type, int ref, int nrm, int adr) {
    TabEntry e;
    e.name = name;
    e.obj  = obj;
    e.type = type;
    e.ref  = ref;
    e.nrm  = nrm;
    e.lev  = currentLevel();
    e.adr  = adr;

    // link ke identifier sebelumnya
    int curBlock = display_.back();
    e.link = btab_[curBlock].last;

    e.initialized = false;
    tab_.push_back(e);
    int newIdx = (int)tab_.size() - 1;

    // update last di current blok
    btab_[curBlock].last = newIdx;

    return newIdx;
}


int SymbolTable::addAtab(const ATabEntry& e) {
    atab_.push_back(e);
    return (int)atab_.size() - 1;
}


int SymbolTable::addBtab(const BTabEntry& e) {
    btab_.push_back(e);
    return (int)btab_.size() - 1;
}


int SymbolTable::lookup(const std::string& name) const {
    std::string lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    // Setiap level ditelusurin
    for (int lv = currentLevel(); lv >= 0; --lv) {
        int blockIdx = display_[lv];
        // Mengikuti linked list yang ada di dalam blok
        int idx = btab_[blockIdx].last;
        while (idx > 0) {
            std::string entryName = tab_[idx].name;
            std::transform(entryName.begin(), entryName.end(),
                           entryName.begin(), ::tolower);
            if (entryName == lower) return idx;
            idx = tab_[idx].link;
        }
    }

    for (int i = 0; i < TAB_USER_START; ++i) {
        std::string entryName = tab_[i].name;
        std::transform(entryName.begin(), entryName.end(),
                       entryName.begin(), ::tolower);
        if (!entryName.empty() && entryName == lower) return i;
    }

    return -1; 
}


int SymbolTable::lookupCurrentBlock(const std::string& name) const {
    std::string lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    int blockIdx = display_.back();
    int idx = btab_[blockIdx].last;
    while (idx > 0) {
        std::string entryName = tab_[idx].name;
        std::transform(entryName.begin(), entryName.end(),
                       entryName.begin(), ::tolower);
        if (entryName == lower) return idx;
        idx = tab_[idx].link;
    }
    return -1;
}


int SymbolTable::sizeOf(DataType dt, int ref) const {
    switch (dt) {
        case DataType::INTEGER:  return 1;
        case DataType::REAL:     return 2; // real biasanya 2 word
        case DataType::CHAR:     return 1;
        case DataType::BOOLEAN:  return 1;
        case DataType::STRING:   return 256; // default max string
        case DataType::ARRAY:
            if (ref >= 0 && ref < (int)atab_.size())
                return atab_[ref].size;
            return 1;
        case DataType::RECORD:
            if (ref >= 0 && ref < (int)btab_.size())
                return btab_[ref].vsze;
            return 1;
        default: return 1;
    }
}


void SymbolTable::printTab(std::ostream& out) const {
    out << "\n===== TAB (Identifier Table) =====\n";
    out << std::left
        << std::setw(5)  << "idx"
        << std::setw(16) << "name"
        << std::setw(12) << "obj"
        << std::setw(12) << "type"
        << std::setw(6)  << "ref"
        << std::setw(6)  << "nrm"
        << std::setw(6)  << "lev"
        << std::setw(8)  << "adr"
        << std::setw(6)  << "link"
        << "\n";
    out << std::string(77, '-') << "\n";

    for (int i = 0; i < (int)tab_.size(); ++i) {
        const TabEntry& e = tab_[i];
        if (e.name.empty() && i > 0) continue; 
        out << std::left
            << std::setw(5)  << i
            << std::setw(16) << e.name
            << std::setw(12) << objClassToString(e.obj)
            << std::setw(12) << dataTypeToString(e.type)
            << std::setw(6)  << e.ref
            << std::setw(6)  << e.nrm
            << std::setw(6)  << e.lev
            << std::setw(8)  << e.adr
            << std::setw(6)  << e.link
            << "\n";
    }
}


void SymbolTable::printAtab(std::ostream& out) const {
    out << "\n===== ATAB (Array Table) =====\n";
    if (atab_.empty()) {
        out << "  (kosong)\n";
        return;
    }
    out << std::left
        << std::setw(5)  << "idx"
        << std::setw(12) << "xtyp"
        << std::setw(12) << "etyp"
        << std::setw(6)  << "eref"
        << std::setw(8)  << "low"
        << std::setw(8)  << "high"
        << std::setw(8)  << "elsz"
        << std::setw(8)  << "size"
        << "\n";
    out << std::string(67, '-') << "\n";
    for (int i = 0; i < (int)atab_.size(); ++i) {
        const ATabEntry& e = atab_[i];
        out << std::left
            << std::setw(5)  << i
            << std::setw(12) << dataTypeToString(e.xtyp)
            << std::setw(12) << dataTypeToString(e.etyp)
            << std::setw(6)  << e.eref
            << std::setw(8)  << e.low
            << std::setw(8)  << e.high
            << std::setw(8)  << e.elsz
            << std::setw(8)  << e.size
            << "\n";
    }
}


void SymbolTable::printBtab(std::ostream& out) const {
    out << "\n===== BTAB (Block Table) =====\n";
    out << std::left
        << std::setw(5)  << "idx"
        << std::setw(8)  << "last"
        << std::setw(8)  << "lpar"
        << std::setw(8)  << "psze"
        << std::setw(8)  << "vsze"
        << "\n";
    out << std::string(37, '-') << "\n";
    for (int i = 0; i < (int)btab_.size(); ++i) {
        const BTabEntry& e = btab_[i];
        out << std::left
            << std::setw(5)  << i
            << std::setw(8)  << e.last
            << std::setw(8)  << e.lpar
            << std::setw(8)  << e.psze
            << std::setw(8)  << e.vsze
            << "\n";
    }
}