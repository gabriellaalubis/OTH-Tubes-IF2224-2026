# Arion Compiler<br> Milestone 1 - Lexical Analysis

## Identitas Kelompok

**Nama Kelompok: oey_thensy** 

**Anggota:** \
Gabriella Botimada Lubis - 13524006 \
Stefani Angeline Oroh - 13524064 \
Reva Natania Sitohang - 13524098

## Deskripsi Program

Arion Lexical Analyzer adalah sebuah program yang mengimplementasikan **Deterministic Finite Automata (DFA)** untuk melakukan analisis leksikal pada bahasa pemrograman Arion. Program ini merupakan tahap pertama dalam proses kompilasi/interpretasi bahasa pemrograman.

### Fitur Utama

1. Membaca source code dalam bahasa pemrograman Arion dari file input (.txt)
2. Menganalisis setiap karakter dan mengidentifikasi token-token sesuai dengan DFA yang telah dirancang
3. Menghasilkan daftar token dalam format output (.txt) yang detail

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
   ARION LEXICAL ANALYZER
   Masukkan path file input (.txt): 
   ```

2. **Masukkan path file input**, contoh:
   ```
   test/milestone-1/tc1.txt
   ```

3. **Tekan Enter** dan program akan:
   - Membaca file input
   - Melakukan analisis leksikal
   - Menghasilkan file output di `test/milestone-1/output-<nama-file>.txt`


## Pembagian Tugas

**Gabriella Botimada Lubis - 13524006**
- Perancangan DFA
- Implementasi Kode
- Pengujian
- Penyusunan Laporan

\
**Stefani Angeline Oroh - 13524064** 
- Perancangan DFA
- Implementasi Kode
- Pengujian
- Penyusunan Laporan

\
**Reva Natania Sitohang - 13524098**
- Perancangan DFA
- Implementasi Kode
- Pengujian
- Penyusunan Laporan



