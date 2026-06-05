#pragma once
#include <QMainWindow>
#include <QTextEdit>
#include <QPushButton>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <string>

class VentanaPrincipal : public QMainWindow {
    Q_OBJECT

public:
    VentanaPrincipal(QWidget *parent = nullptr);
    ~VentanaPrincipal() = default;

private slots:
    // Evento que se dispara al pulsar el botón "Compilar y Ejecutar"
    void manejarEjecucion();

private:
    // Componentes de la Interfaz Gráfica
    QTextEdit* editorCodigo;     // Entrada de código fuente
    QTextEdit* consolaSalida;    // Salida de textos y errores
    QWidget* lienzoAnimacion;  // ¡ZONA DE EXPANSIÓN PARA TUS GRÁFICOS FUTUROS!
    QPushButton* botonEjecutar;    // Botón de arranque
};
