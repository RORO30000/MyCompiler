#include "VentanaPrincipal.hpp"
#include "lexer.hpp"
#include <vector>
#include <sstream>
#include <iostream>

// Prototipos externos ya definidos en tu lexer.cpp y parser.cpp
std::vector<Token> tokenizar(const std::string& fuente);
double parsear(std::vector<Token>& tokens);
std::string preprocesarBibliotecas(const std::string& codigoFuente);

VentanaPrincipal::VentanaPrincipal(QWidget *parent) : QMainWindow(parent) {
    // Configuración general de la ventana de la App
    setWindowTitle("Simulador C++ Didáctico - Entorno de Desarrollo");
    resize(1024, 720);

    // 1. Crear componentes de texto y botones
    editorCodigo = new QTextEdit(this);
    editorCodigo->setPlaceholderText("// Escribe tu código en español aquí...");
    // Aplicar fuente limpia para programadores (Monoespaciada)
    QFont fuenteCodigo("Monospace", 12);
    fuenteCodigo.setStyleHint(QFont::TypeWriter);
    editorCodigo->setFont(fuenteCodigo);

    consolaSalida = new QTextEdit(this);
    consolaSalida->setReadOnly(true); // El usuario no puede alterar la consola
    consolaSalida->setStyleSheet("background-color: #1e1e1e; color: #64e572; font-family: 'Courier New';");
    consolaSalida->setPlaceholderText("Salida del sistema...");

    botonEjecutar = new QPushButton("▶ Compilar y Ejecutar", this);
    botonEjecutar->setStyleSheet("background-color: #2b579a; color: white; font-weight: bold; padding: 8px;");

    // 2. Crear el lienzo donde harás tus futuras animaciones visuales
    lienzoAnimacion = new QWidget(this);
    lienzoAnimacion->setStyleSheet("background-color: #252526; border: 1px solid #3c3c3c;");

    // 3. Organizar la distribución espacial (Layouts y Splitters)
    QWidget* contenedorCentral = new QWidget(this);
    setCentralWidget(contenedorCentral);

    QVBoxLayout* layoutPrincipal = new QVBoxLayout(contenedorCentral);
    QHBoxLayout* layoutBotones = new QHBoxLayout();
    layoutBotones->addWidget(botonEjecutar);
    layoutBotones->addStretch(); // Empuja el botón a la izquierda

    // El Splitter permite al usuario redimensionar los paneles arrastrando el borde
    QSplitter* splitterHorizontal = new QSplitter(Qt::Horizontal, this);
    splitterHorizontal->addWidget(editorCodigo);
    splitterHorizontal->addWidget(lienzoAnimacion);
    // Darle tamaños iniciales proporcionales (50% editor, 50% animaciones)
    splitterHorizontal->setSizes(QList<int>({512, 512}));

    QSplitter* splitterVertical = new QSplitter(Qt::Vertical, this);
    splitterVertical->addWidget(splitterHorizontal);
    splitterVertical->addWidget(consolaSalida);
    splitterVertical->setSizes(QList<int>({500, 220}));

    layoutPrincipal->addLayout(layoutBotones);
    layoutPrincipal->addWidget(splitterVertical);

    // 4. Conectar el clic del botón con el motor del compilador
    connect(botonEjecutar, &QPushButton::clicked, this, &VentanaPrincipal::manejarEjecucion);
}

void VentanaPrincipal::manejarEjecucion() {
    consolaSalida->clear();
    consolaSalida->append("Enviando código al analizador léxico...");

    // Capturar el código fuente escrito en la interfaz gráfica por el alumno
    std::string codigoFuente = editorCodigo->toPlainText().toStdString();

    // REDIRECCIÓN DE COUT: Capturamos lo que tu parser imprime con std::cout
    std::stringstream bufferCaptura;
    std::streambuf* viejoCout = std::cout.rdbuf(bufferCaptura.rdbuf());

    try {
        // Ejecución exacta de las etapas de tu compilador actual
        std::string codigoExpandido = preprocesarBibliotecas(codigoFuente);
        auto tokens = tokenizar(codigoExpandido);
        
        // Fase de parseo y ejecución semántica
        parsear(tokens);

        // Devolver el flujo a la normalidad y mostrar en la app
        std::cout.rdbuf(viejoCout);
        consolaSalida->append(QString::fromStdString(bufferCaptura.str()));
        consolaSalida->append("\n✓ Ejecución finalizada.");

    } catch (const std::exception& e) {
        // Si hay errores léxicos, sintácticos o semánticos, tu errors.hpp lanza excepciones.
        // Aquí las atrapamos y las pintamos limpiamente en color rojo en la app.
        std::cout.rdbuf(viejoCout);
        consolaSalida->append(QString::fromStdString(bufferCaptura.str())); // Mostrar lo impreso antes de caer
        consolaSalida->append(QString("<span style='color:#ff5555;'>%1</span>").arg(e.what()));
    }
}
