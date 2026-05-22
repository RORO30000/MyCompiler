#  Makefile — Compilador 

CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra
TARGET   = compilador
# Apenas os arquivos .cpp reais que existem no seu projeto:
SRCS     = main.cpp lexer.cpp parser.cpp
OBJS     = $(SRCS:.cpp=.o)

# ── Regla principal: compila todo ─────────────
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)
	@echo ""
	@echo "✓ Compilación exitosa — ejecuta con:  make run"
	@echo ""

# ── Compila cada .cpp a su .o ─────────────────
# Adicionamos semantic.hpp como dependência para que, se você mudá-lo, o make recompile o projeto.
%.o: %.cpp semantic.hpp lexer.hpp errors.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# ── Ejecuta el compilador ─────────────────────
run: $(TARGET)
	@echo ""
	./$(TARGET)

# ── Compila y ejecuta en un solo paso ─────────
go: all run

# ── Borra los archivos generados ──────────────
clean:
	rm -f $(OBJS) $(TARGET)
	@echo "Limpieza lista."

.PHONY: all run go clean
