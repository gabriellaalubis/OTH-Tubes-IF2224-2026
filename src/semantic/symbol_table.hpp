#ifndef SYMBOL_TABLE_HPP
#define SYMBOL_TABLE_HPP

#include "ast.hpp"
#include <ostream>
#include <string>
#include <vector>

enum class ObjClass {
    CONSTANT,
    VARIABLE,
    TYPE_NAME,
    PROCEDURE,
    FUNCTION,
    PROGRAM,
    FIELD,
    PARAMETER,
};

std::string objClassToString(ObjClass oc);

// Satu baris simbol: nama, jenis, tipe, dan info alamatnya
struct TabEntry {
    std::string name;
    int link = 0;
    ObjClass obj = ObjClass::VARIABLE;
    DataType type = DataType::NOTYPE;
    int ref = 0;
    int nrm = 1;
    int lev = 0;
    int adr = 0;
};

// Kalau simbolnya array, detailnya disimpan di sini
struct ATabEntry {
    DataType xtyp = DataType::NOTYPE;
    DataType etyp = DataType::NOTYPE;
    int eref = 0;
    int low = 0;
    int high = 0;
    int elsz = 1;
    int size = 0;
};

// Ringkasan isi dari satu blok/scope
struct BTabEntry {
    int last = 0;
    int lpar = 0;
    int psze = 0;
    int vsze = 0;
};

class SymbolTable {
public:
    SymbolTable();

    std::vector<TabEntry>& tab() { return tab_; }
    std::vector<ATabEntry>& atab() { return atab_; }
    std::vector<BTabEntry>& btab() { return btab_; }

    const std::vector<TabEntry>& tab() const { return tab_; }
    const std::vector<ATabEntry>& atab() const { return atab_; }
    const std::vector<BTabEntry>& btab() const { return btab_; }

    int enterBlock();
    void leaveBlock();

    int currentLevel() const { return static_cast<int>(display_.size()) - 1; }
    int blockAt(int level) const { return display_[level]; }

    int addTab(const std::string& name, ObjClass obj, DataType type,
               int ref = 0, int nrm = 1, int adr = 0);
    int addAtab(const ATabEntry& e);
    int addBtab(const BTabEntry& e);

    int lookup(const std::string& name) const;
    int lookupCurrentBlock(const std::string& name) const;

    int sizeOf(DataType dt, int ref = 0) const;

    void printTab(std::ostream& out) const;
    void printAtab(std::ostream& out) const;
    void printBtab(std::ostream& out) const;

private:
    std::vector<TabEntry> tab_;
    std::vector<ATabEntry> atab_;
    std::vector<BTabEntry> btab_;
    std::vector<int> display_;

    void initPredefined();
};

// Slot bawaan di `tab_`, dipakai bareng sama parser/semantic
inline constexpr int TAB_IDX_INTEGER = 1;
inline constexpr int TAB_IDX_REAL = 2;
inline constexpr int TAB_IDX_CHAR = 3;
inline constexpr int TAB_IDX_BOOLEAN = 4;
inline constexpr int TAB_IDX_STRING = 5;
inline constexpr int TAB_IDX_TRUE = 6;
inline constexpr int TAB_IDX_FALSE = 7;
inline constexpr int TAB_IDX_WRITELN = 8;
inline constexpr int TAB_IDX_READLN = 9;
inline constexpr int TAB_IDX_WRITE = 10;
inline constexpr int TAB_IDX_READ = 11;
inline constexpr int TAB_USER_START = 33;

#endif
