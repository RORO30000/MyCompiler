# ── Configuración del Compilador ─────────────────────────────────────────────
CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -fPIC

# ── Detección Automática de Qt (Soporta Qt6 o Qt5) ───────────────────────────
QT_VERSION := $(shell pkg-config --exists Qt6Widgets && echo Qt6 || echo Qt5)
QT_CFLAGS  := $(shell pkg-config --cflags $(QT_VERSION)Widgets)
QT_LIBS    := $(shell pkg-config --libs $(QT_VERSION)Widgets)

# ── Detección del binario MOC real ───────────────────────────────────────────
ifeq ($(QT_VERSION),Qt6)
    MOC_ENV := QT_SELECT=qt6
    MOC_BIN := $(shell which moc-qt6 2>/dev/null || \
                find /usr -name moc 2>/dev/null | grep -i qt6 | head -1 || \
                which moc 2>/dev/null)
else
    MOC_ENV := QT_SELECT=qt5
    MOC_BIN := $(shell which moc-qt5 2>/dev/null || \
                find /usr -name moc 2>/dev/null | grep -i qt5 | head -1 || \
                which moc 2>/dev/null)
endif

# ── Nombre del Ejecutable Final ──────────────────────────────────────────────
TARGET   = ide_grafico

# ── Archivos Fuente del Proyecto ─────────────────────────────────────────────
SRCS     = main_gui.cpp VentanaPrincipal.cpp lexer.cpp parser.cpp preprocesador.cpp
OBJS     = $(SRCS:.cpp=.o) moc_VentanaPrincipal.o

# ── Cabeceras compartidas (cualquier cambio fuerza recompilación) ─────────────
HEADERS  = lexer.hpp semantic.hpp errors.hpp eventos.hpp VentanaPrincipal.hpp

# ── Regla Principal: Compila Todo ────────────────────────────────────────────
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(QT_LIBS)
	@echo ""
	@echo "✓ ¡Compilación Gráfica Exitosa! — Ejecuta tu app con: make run"
	@echo ""

# ── Reglas de Compilación Independientes ─────────────────────────────────────

VentanaPrincipal.o: VentanaPrincipal.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) $(QT_CFLAGS) -c VentanaPrincipal.cpp -o VentanaPrincipal.o

main_gui.o: main_gui.cpp VentanaPrincipal.hpp
	$(CXX) $(CXXFLAGS) $(QT_CFLAGS) -c main_gui.cpp -o main_gui.o

moc_VentanaPrincipal.o: moc_VentanaPrincipal.cpp
	$(CXX) $(CXXFLAGS) $(QT_CFLAGS) -c moc_VentanaPrincipal.cpp -o moc_VentanaPrincipal.o

%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# ── Generador MOC Adaptativo ─────────────────────────────────────────────────
moc_VentanaPrincipal.cpp: VentanaPrincipal.hpp eventos.hpp
	$(MOC_ENV) $(MOC_BIN) VentanaPrincipal.hpp -o moc_VentanaPrincipal.cpp

# ── Ejecutar la Aplicación ───────────────────────────────────────────────────
run: $(TARGET)
	@echo "Lanzando la interfaz gráfica..."
	@echo ""
	./$(TARGET)

# ── Compilar y Ejecutar en un solo paso ──────────────────────────────────────
go: all run

# ── Limpieza del Proyecto ────────────────────────────────────────────────────
clean:
	rm -f $(OBJS) $(TARGET) moc_VentanaPrincipal.cpp moc_VentanaPrincipal.o
	rm -f compilador main.o
	@echo "Limpieza lista. Todos los binarios y archivos temporales eliminados."

.PHONY: all run go clean
