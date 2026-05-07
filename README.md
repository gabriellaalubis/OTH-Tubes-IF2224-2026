# Arion Compiler
Milestone 1 - Lexical Analysis  
Milestone 2 - Syntax Analysis (Parse Tree)

## Identitas Kelompok

**Nama Kelompok: oey_thensy** 

**Anggota:** \
Gabriella Botimada Lubis - 13524006 \
Stefani Angeline Oroh - 13524064 \
Reva Natania Sitohang - 13524098

## Deskripsi Program

Arion Compiler adalah program untuk menganalisis kode sumber bahasa Arion dalam dua tahap:
1. **Lexical Analysis (Milestone 1)** menggunakan **Deterministic Finite Automata (DFA)** untuk menghasilkan token.
2. **Syntax Analysis (Milestone 2)** menggunakan **recursive-descent parser** untuk membangun parse tree dan memvalidasi struktur program.

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

### Cakupan Grammar Milestone 2

Parser menangani konstruksi utama berikut:
- `program ... ; ... begin ... end .`
- Deklarasi `const`, `type`, `var`
- Tipe: identifier, `array`, `record`, `enumerated`, `range`
- Subprogram: `procedure`, `function`, formal parameter
- Statement: assignment, procedure/function call, compound statement
- Control flow: `if`, `case`, `while`, `repeat-until`, `for ... to/downto`
- Ekspresi: relational, additive, multiplicative, unary (`+`, `-`, `not`)

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
   ARION COMPILER
   Masukkan path file input (.txt): 
   ```

2. **Masukkan path file input**, contoh:
   ```
   test/milestone-2/tc1.txt
   ```

3. **Tekan Enter** dan program akan:
   - Membaca file input
   - Menampilkan **hasil lexical analysis di terminal**
   - Menampilkan **parse tree di terminal**
   - Menyimpan **hanya parse tree** ke file `test/milestone-2/output-<nama-file>.txt`
   - **Notes: Pada Milestone 2, lexical analysis tidak disimpan ke file output**

## Testing Milestone 2

Folder test yang tersedia:
- `test/milestone-2/tc1.txt` sampai `test/milestone-2/tc12.txt`
- Referensi output: `test/milestone-2/output-tc1.txt` sampai `output-tc12.txt`

Ringkasan:
- **Kasus valid (parse tree terbentuk):** `tc1` s.d. `tc6`
- **Kasus error sintaks (pesan error):** `tc7` s.d. `tc12`


## Pembagian Tugas (Milestone 2)

**Gabriella Botimada Lubis - 13524006**
- Implementasi Kode Parser
- Testing
- Bug Fixing
- Penyusunan Laporan


\
**Stefani Angeline Oroh - 13524064** 
- Implementasi Kode Parser
- Testing
- Bug Fixing
- Penyusunan Laporan

\
**Reva Natania Sitohang - 13524098**
- Implementasi Kode Parser
- Testing
- Bug Fixing
- Penyusunan Laporan



