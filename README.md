# Arion Compiler
Milestone 1 - Lexical Analysis  
Milestone 2 - Syntax Analysis (Parse Tree)
Milestone 3 - Semantic Analysis (Decorated AST)
Milestone 4 - Intermediate Code Generation & Interpreter

## Identitas Kelompok

**Nama Kelompok: oey_thensy** 

**Anggota:** \
Gabriella Botimada Lubis - 13524006 \
Stefani Angeline Oroh - 13524064 \
Reva Natania Sitohang - 13524098

## Deskripsi Program

Arion Compiler adalah program untuk menganalisis dan mengeksekusi kode sumber bahasa Arion dalam empat tahap:
1. **Lexical Analysis (Milestone 1)** menggunakan **Deterministic Finite Automata (DFA)** untuk menghasilkan token.
2. **Syntax Analysis (Milestone 2)** menggunakan **recursive-descent parser** untuk membangun parse tree dan memvalidasi struktur program.
3. **Semantic Analysis (Milestone 3)** menggunakan **attributed grammar** untuk membangun decorated AST, mengelola symbol table, dan melakukan type & scope checking.
4. **Intermediate Code Generation & Interpreter (Milestone 4)** mengonversi decorated AST menjadi instruksi stack machine dan mengeksekusinya melalui sebuah virtual machine interpreter.

### Fitur Utama Milestone 1 (Lexer)

1. Membaca source code dalam bahasa pemrograman Arion dari file input (.txt)
2. Menganalisis setiap karakter dan mengidentifikasi token-token sesuai dengan DFA yang telah dirancang
3. Menghasilkan daftar token dalam format output (.txt) yang detail

### Fitur Utama Milestone 2 (Parser)

1. Menerima hasil tokenisasi dari lexer (komentar diabaikan untuk parsing).
2. Melakukan parsing program Arion dan membangun **parse tree**.
3. Menulis hasil parse tree ke file `test/milestone-2/output-<nama-file>.txt`.
4. Menangani syntax error dengan pesan yang informatif (line/col dan token yang diharapkan).
5. Memvalidasi urutan deklarasi (`const -> type -> var -> subprogram`).

#### Cakupan Grammar Milestone 2

Parser menangani konstruksi utama berikut:
- `program ... ; ... begin ... end .`
- Deklarasi `const`, `type`, `var`
- Tipe: identifier, `array`, `record`, `enumerated`, `range`
- Subprogram: `procedure`, `function`, formal parameter
- Statement: assignment, procedure/function call, compound statement
- Control flow: `if`, `case`, `while`, `repeat-until`, `for ... to/downto`
- Ekspresi: relational, additive, multiplicative, unary (`+`, `-`, `not`)

### Fitur Utama Milestone 3 (Semantic Analyser)

1. Menerima parse tree dari parser dan melakukan konversi ke **Abstract Syntax Tree (AST)** menggunakan Syntax-Directed Translation.
2. Melakukan **dekorasi AST** dengan anotasi tipe data, tab index, block index, dan lexical level pada setiap node.
3. Membangun dan mengelola **symbol table** (TAB, BTAB, ATAB) selama proses analisis.
4. Melakukan **type checking** — memastikan operator dan operand kompatibel, termasuk aturan assignment-compatible.
5. Melakukan **scope resolution** — lookup identifier dari scope terdalam ke terluar menggunakan display stack.
6. Mendeteksi **semantic error** dengan pesan yang informatif.
7. Menyimpan output decorated AST dan symbol table ke file `test/milestone-3/output-<nama-file>.txt`.

#### Cakupan Semantic Analysis Milestone 3

- Deklarasi: `const`, `type` (subrange, enumerated, record, array), `var`, `procedure`, `function`
- Parameter formal procedure/function (pass by value), scope lokal parameter
- Statement: assignment, if, while, for, repeat-until, case, procedure/function call
- Ekspresi: binary operator, unary operator, array access (termasuk array of array), field access
- Predefined identifier: `integer`, `real`, `char`, `boolean`, `string`, `true`, `false`, `writeln`, `readln`, `write`, `read`

## Requirements

- **Operating System:** Linux, macOS, atau Windows (dengan WSL/MinGW)
- **Compiler:** GCC atau G++ dengan dukungan C++17 atau lebih baru
- **Build Tool:** GNU Make


## Cara Instalasi dan Penggunaan Program

### Clone repository
```bash
git clone https://github.com/gabriellaalubis/OTH-Tubes-IF2224-2026
```

### Masuk ke direktori proyek
```bash
cd OTH-Tubes-IF2224-2026
```

### Build program
```bash
make
```
atau
```bash
make clean
make all
```

### Jalankan Program

```bash
make run
```

### Cara Penggunaan

#### Step-by-step:

1. **Program akan menampilkan:**
   ```
   ╔═══════════════════════════════════════════════════════════════╗
   ║                       ARION INTERPRETER                       ║
   ║  Lexical + Syntax + Semantic + Intermediate Code + Execution  ║
   ╚═══════════════════════════════════════════════════════════════╝

   Masukkan path file input (.txt):
   ```

2. **Masukkan path file input**, contoh:
   ```
   test/milestone-4/tc1.txt
   ```

3. **Tekan Enter** dan program akan:
   - Membaca file input
   - Menampilkan **hasil lexical analysis di terminal**
   - Menampilkan **parse tree di terminal**
   - Menampilkan **decorated AST dan symbol table di terminal**
   - Menampilkan **intermediate code (stack machine) di terminal**
   - Mengeksekusi program dan menampilkan **output eksekusi di terminal**
   - Menyimpan **intermediate code** dan **hasil eksekusi** ke file `test/milestone-4/output-<nama-file>.txt`
   - Jika terjadi error pada code generation atau execution, pesan error juga disimpan ke file output
   - **Notes: Lexical analysis, parse tree, dan semantic analysis tidak disimpan ke file output**

## Testing Milestone 2

Folder test yang tersedia:
- `test/milestone-2/tc1.txt` sampai `test/milestone-2/tc12.txt`
- Referensi output: `test/milestone-2/output-tc1.txt` sampai `output-tc12.txt`

Ringkasan:
- **Kasus valid (parse tree terbentuk):** `tc1` s.d. `tc6`
- **Kasus error sintaks (pesan error):** `tc7` s.d. `tc12`

## Testing Milestone 3

Folder test yang tersedia:
- `test/milestone-3/tc1.txt` sampai `test/milestone-3/tc14.txt`
- `test/milestone-3/error1.txt` sampai `test/milestone-3/error10.txt`
- Referensi output: `test/milestone-3/output-tc1.txt` sampai `output-tc14.txt` dan `output-error1.txt` sampai `output-error10.txt`

Ringkasan:
- **Kasus valid (decorated AST terbentuk):** `tc1` s.d. `tc14`
- **Kasus error semantik (pesan error):** `error1` s.d. `error10`

## Testing Milestone 4

Folder test yang tersedia:
- `test/milestone-4/tc1.txt` sampai `test/milestone-4/tc13.txt` — kasus valid
- `test/milestone-4/tc_ms4err1.txt` sampai `test/milestone-4/tc_ms4err6.txt` — kasus error runtime
- `test/milestone-4/tc-spec.txt` — contoh program dari spesifikasi (halaman 21–29)
- `test/milestone-4/tc-comprehensive.txt` — kasus komprehensif gabungan fitur
- Referensi output: `test/milestone-4/output-<nama-file>.txt`

Ringkasan:
- **Kasus valid (hasil eksekusi ada):** `tc1` s.d. `tc13`
- **Kasus error interpreter (pesan error):** `error1` s.d. `error6`


## Pembagian Tugas (Milestone 4)

**Gabriella Botimada Lubis - 13524006**
- Implementasi Intermediate Code Generator
- Implementasi Interpreter (Stack Machine)
- Testing
- Bug Fixing
- Penyusunan Laporan

\
**Stefani Angeline Oroh - 13524064**
- Implementasi Intermediate Code Generator
- Implementasi Interpreter (Stack Machine)
- Testing
- Bug Fixing
- Penyusunan Laporan

\
**Reva Natania Sitohang - 13524098**
- Implementasi Intermediate Code Generator
- Implementasi Interpreter (Stack Machine)
- Testing
- Bug Fixing
- Penyusunan Laporan