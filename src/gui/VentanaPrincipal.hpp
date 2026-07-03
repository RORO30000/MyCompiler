#pragma once
#include <QMainWindow>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QScrollArea>
#include <QPropertyAnimation>
#include <QTimer>
#include <QScrollBar>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QPainter>
#include <QResizeEvent>
#include <string>
#include <vector>
#include <QComboBox>
#include <QDialog>
#include <QKeyEvent>
#include <QCloseEvent>
#include "core/eventos.hpp"


// ─── Gutter de números de línea ──────────────────────────────────
class LineNumberArea : public QWidget {
    Q_OBJECT
public:
    explicit LineNumberArea(QTextEdit* editor);
    int gutterWidth();
protected:
    void paintEvent(QPaintEvent* ev) override;
private:
    QTextEdit* _ed;
};

// ─── Editor con gutter integrado ─────────────────────────────────
class CodeEditor : public QTextEdit {
    Q_OBJECT
public:
    explicit CodeEditor(QWidget* parent = nullptr);
protected:
    void resizeEvent(QResizeEvent* ev) override;
    void scrollContentsBy(int dx, int dy) override;
    void keyPressEvent(QKeyEvent* ev) override;
private slots:
    void _updateGutter();
private:
    LineNumberArea* _gutter;
};

// ─── Ventana Principal ────────────────────────────────────────────
class VentanaPrincipal : public QMainWindow {
    Q_OBJECT

public:
    explicit VentanaPrincipal(QWidget* parent = nullptr);
    ~VentanaPrincipal() = default;

protected:
    void closeEvent(QCloseEvent* ev) override;

private slots:
    void manejarEjecucion();
    void avanzarPaso();
    void retrocederPaso();
    void cargarPlantilla(const QString& nombre);
    void nuevoArchivo();
    void abrirArchivo();
    void guardarArchivo();
    void guardarComoArchivo();
    void salirAplicacion();

private:
    // ── Widgets ───────────────────────────────────────────────────
    CodeEditor*  editorCodigo;
    QPushButton* botonEjecutar;
    QComboBox*   comboPlantillas;
    QPushButton* botonSiguiente;
    QPushButton* botonAnterior;
    QLabel*      etiquetaPaso;

    // Panel derecho
    QWidget*     panelAnimacion;
    QLabel*      tituloVariables;
    QWidget*     zonaTarjetas;
    QHBoxLayout* layoutTarjetas;
    QLabel*      tituloArreglo;
    QWidget*     zonaArreglo;
    QHBoxLayout* layoutArreglo;
    QLabel*      etiquetaEstado;

    // ── Cola de pasos ─────────────────────────────────────────────
    std::vector<EventoPaso> pasos;
    int pasoActual = 0;

    // ── Snapshot ─────────────────────────────────────────────────
    struct SnapshotUI {
        std::vector<std::pair<std::string,std::string>> variables;
        std::vector<std::string> celdas;
        std::string nombreArr;
        int         indiceArr    = -1;
        int         lineaActiva  = -1;
        std::string mensajeEstado;
        bool        mensajeError = false;
    };
    std::vector<SnapshotUI> historial;
    SnapshotUI estadoActual;

    // ── Archivo actual ────────────────────────────────────────────
    QString rutaActual;

    // ── Helpers ───────────────────────────────────────────────────
    void aplicarEvento(const EventoPaso& ev);
    void mostrarTerminal(const std::string& contenido);
    void restaurarSnapshot(const SnapshotUI& snap);
    void resaltarLinea(int linea);
    void actualizarTarjetaVariable(const std::string& nombre,
                                   const std::string& valor, bool esNueva);
    void actualizarVistaArreglo(const std::string& nombre,
                                const std::vector<std::string>& celdas,
                                int indiceActivo);
    void limpiarTarjetas();
    void limpiarArreglo();
    void setMensajeEstado(const std::string& msg, bool esError = false);
    void actualizarBotones();
};
