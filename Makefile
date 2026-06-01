CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Iinclude

# Semua source file
SRCS = src/main.cpp \
       src/lexer/lexer.cpp \
       src/lexer/token.cpp \
	   src/parser/parser.cpp \
	   src/semantic/semantic.cpp \
	   src/semantic/ast.cpp \
	   src/semantic/symbol_table.cpp \
	   src/codegenerator/codegen.cpp \
	   src/interpreter/interpreter.cpp

TARGET = arion

# ─── Build utama ────────────────────────────────────────────
all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS)
	@echo "Build berhasil -> ./$(TARGET)"

# ─── Hapus hasil build ──────────────────────────────────────
clean:
	rm -f $(TARGET)
	@echo "Build dihapus."

# ─── Jalankan program ───────────────────────────────────────
run: all
	./$(TARGET)

.PHONY: all clean run