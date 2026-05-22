#  Makefile — Compilador 

CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra
TARGET   = compilador
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
%.o: %.cpp
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

# ── Evita conflictos con archivos del mismo nombre
.PHONY: all run go clean
