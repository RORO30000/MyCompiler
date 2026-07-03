# ── Configuración del Compilador ─────────────────────────────────────────────
CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -fPIC -Isrc

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

# ── Archivos Fuente del Proyecto (rutas relativas) ───────────────────────────
SRCS     = src/main_gui.cpp src/gui/VentanaPrincipal.cpp \
           src/core/lexer.cpp src/core/parser.cpp src/core/preprocesador.cpp
OBJS     = $(SRCS:.cpp=.o) moc_VentanaPrincipal.o

# ── Cabeceras compartidas (con ruta relativa a src/) ─────────────────────────
HEADERS  = src/core/lexer.hpp src/core/semantic.hpp src/core/errors.hpp \
           src/core/eventos.hpp src/gui/VentanaPrincipal.hpp

# ── Regla Principal: Compila Todo ────────────────────────────────────────────
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(QT_LIBS)
	@echo ""
	@echo "✓ ¡Compilación Gráfica Exitosa! — Ejecuta tu app con: make run"
	@echo ""

# ── Reglas de Compilación Independientes ─────────────────────────────────────

src/gui/VentanaPrincipal.o: src/gui/VentanaPrincipal.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) $(QT_CFLAGS) -c src/gui/VentanaPrincipal.cpp \
	        -o src/gui/VentanaPrincipal.o

src/main_gui.o: src/main_gui.cpp src/gui/VentanaPrincipal.hpp
	$(CXX) $(CXXFLAGS) $(QT_CFLAGS) -c src/main_gui.cpp -o src/main_gui.o

moc_VentanaPrincipal.o: moc_VentanaPrincipal.cpp
	$(CXX) $(CXXFLAGS) $(QT_CFLAGS) -c moc_VentanaPrincipal.cpp \
	        -o moc_VentanaPrincipal.o

%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# ── Generador MOC Adaptativo ─────────────────────────────────────────────────
moc_VentanaPrincipal.cpp: src/gui/VentanaPrincipal.hpp src/core/eventos.hpp
	$(MOC_ENV) $(MOC_BIN) src/gui/VentanaPrincipal.hpp \
	           -o moc_VentanaPrincipal.cpp

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
	rm -f src/main.o src/core/main.o
	@echo "Limpieza lista. Todos los binarios y archivos temporales eliminados."

.PHONY: all run go clean
