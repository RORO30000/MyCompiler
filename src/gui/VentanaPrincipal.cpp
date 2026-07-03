#include "gui/VentanaPrincipal.hpp"
#include "core/lexer.hpp"
#include "core/eventos.hpp"
#include <vector>
#include <sstream>
#include <iostream>
#include <QFont>
#include <QFrame>
#include <QScrollArea>
#include <QPainter>
#include <QAbstractTextDocumentLayout>
#include <QTextBlock>
#include <QMenuBar>
#include <QStatusBar>
#include <QTextBlock>
#include <QComboBox>
#include <QInputDialog>
#include <QLineEdit>
#include <QFileDialog>
#include <QFile>
#include <QDir>
#include <QProcess>
#include <QMessageBox>
#include <QDateTime>
#include <QApplication>

// ── Prototipos externos ───────────────────────────────────────────
std::vector<Token>  tokenizar(const std::string&);
double              parsear(std::vector<Token>&, std::vector<EventoPaso>* = nullptr);
std::string         preprocesarBibliotecas(const std::string&);
void                setInputHook(std::string (*hook)());

// ═════════════════════════════════════════════════════════════════
//  Paleta de colores centralizada
// ═════════════════════════════════════════════════════════════════
namespace Pal {
    static const char* BG_APP      = "#1e1e1e";
    static const char* BG_EDITOR   = "#1e1e1e";
    static const char* BG_PANEL    = "#252526";
    static const char* BG_GUTTER   = "#2d2d30";
    static const char* BG_MENUBAR  = "#2d2d30";

    static const char* ACCENT_BLUE  = "#007acc";
    static const char* ACCENT_GREEN = "#16a34a";
    static const char* ACCENT_GRAY  = "#4b5563";

    static const char* H_BLUE  = "#1d4ed8";
    static const char* H_GREEN = "#15803d";
    static const char* H_GRAY  = "#6b7280";

    static const char* TXT_MAIN = "#d4d4d4";
    static const char* TXT_DIM  = "#9ca3af";
}

// ═════════════════════════════════════════════════════════════════
//  Helper: crear botón con estilo unificado
// ═════════════════════════════════════════════════════════════════
static QPushButton* makeBtn(const QString& label,
                             const char* bg, const char* hover,
                             QWidget* parent = nullptr)
{
    auto* b = new QPushButton(label, parent);
    b->setFixedHeight(26);
    b->setCursor(Qt::PointingHandCursor);
    b->setStyleSheet(
        QString(
        "QPushButton {"
        "  background:%1; color:#f0f0f0;"
        "  font-family:'Segoe UI',sans-serif; font-size:12px; font-weight:600;"
        "  border:none; border-radius:4px; padding:0 14px;"
        "}"
        "QPushButton:hover  { background:%2; }"
        "QPushButton:pressed{ background:%2; opacity:0.85; }"
        "QPushButton:disabled{ background:#2d2d2d; color:#555; border:1px solid #3a3a3a; }"
        ).arg(bg, hover)
    );
    return b;
}

// ═════════════════════════════════════════════════════════════════
//  LineNumberArea
// ═════════════════════════════════════════════════════════════════
LineNumberArea::LineNumberArea(QTextEdit* editor) : QWidget(editor), _ed(editor) {}

int LineNumberArea::gutterWidth() {
    int digits = 1, max = std::max(1, _ed->document()->blockCount());
    while (max >= 10) { max /= 10; ++digits; }
    return 10 + digits * fontMetrics().horizontalAdvance('0');
}

void LineNumberArea::paintEvent(QPaintEvent* ev) {
    QPainter p(this);
    p.fillRect(ev->rect(), QColor(Pal::BG_GUTTER));

    // línea separadora derecha
    p.setPen(QColor("#3a3a3a"));
    p.drawLine(width()-1, ev->rect().top(), width()-1, ev->rect().bottom());

    QFont f = _ed->font();
    p.setFont(f);

    QTextBlock blk = _ed->document()->begin();
    int num = 0;
    auto* layout = _ed->document()->documentLayout();

    while (blk.isValid()) {
        QRectF br = layout->blockBoundingRect(blk)
                         .translated(0, -_ed->verticalScrollBar()->value());
        int top = (int)br.top();
        int h   = (int)br.height();

        if (top + h >= ev->rect().top() && top <= ev->rect().bottom()) {
            p.setPen(QColor(Pal::TXT_DIM));
            p.drawText(0, top, width()-6, h,
                       Qt::AlignRight | Qt::AlignTop,
                       QString::number(num));
        }
        ++num;
        blk = blk.next();
    }
}

// ═════════════════════════════════════════════════════════════════
//  CodeEditor
// ═════════════════════════════════════════════════════════════════
CodeEditor::CodeEditor(QWidget* parent) : QTextEdit(parent) {
    _gutter = new LineNumberArea(this);

    connect(document(), &QTextDocument::blockCountChanged,
            this, &CodeEditor::_updateGutter);
    connect(verticalScrollBar(), &QScrollBar::valueChanged,
            this, [this](int){ _gutter->update(); });
    connect(this, &QTextEdit::textChanged,
            this, &CodeEditor::_updateGutter);

    _updateGutter();
}

void CodeEditor::_updateGutter() {
    int w = _gutter->gutterWidth();
    setViewportMargins(w, 0, 0, 0);
    _gutter->setGeometry(0, 0, w, height());
    _gutter->update();
}

void CodeEditor::resizeEvent(QResizeEvent* ev) {
    QTextEdit::resizeEvent(ev);
    _gutter->setGeometry(0, 0, _gutter->gutterWidth(), height());
}

void CodeEditor::scrollContentsBy(int dx, int dy) {
    QTextEdit::scrollContentsBy(dx, dy);
    _gutter->update();
}

void CodeEditor::keyPressEvent(QKeyEvent* ev) {
    if (ev->key() == Qt::Key_Tab) {
        insertPlainText("    ");
        return;
    }

    if (ev->key() == Qt::Key_Return || ev->key() == Qt::Key_Enter) {
        QTextCursor cur = textCursor();
        int cursorPos = cur.position();

        QTextCursor lineCur = cur;
        lineCur.movePosition(QTextCursor::StartOfLine);
        int lineStart = lineCur.position();
        lineCur.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
        QString fullLine = lineCur.selectedText();

        int indent = 0, wsLen = 0;
        for (int i = 0; i < fullLine.length(); i++) {
            if (fullLine[i] == ' ') { indent++; wsLen++; }
            else if (fullLine[i] == '\t') { indent += 4; wsLen++; }
            else break;
        }

        QString trimmed = fullLine.trimmed();

        int curIndent = indent;
        if (trimmed.startsWith("fin_") || trimmed == "sino" || trimmed.startsWith("}"))
            curIndent = qMax(0, indent - 4);

        int newIndent = curIndent;
        if (trimmed.endsWith("{"))
            newIndent = curIndent + 4;

        if (curIndent != indent && !trimmed.isEmpty()) {
            QTextCursor fix = cur;
            fix.setPosition(lineStart);
            fix.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, wsLen);
            fix.removeSelectedText();
            fix.insertText(QString(curIndent, ' '));
            cursorPos = cursorPos - wsLen + curIndent;
            cur.setPosition(cursorPos);
            setTextCursor(cur);
        }

        QTextEdit::keyPressEvent(ev);

        if (newIndent > 0)
            insertPlainText(QString(newIndent, ' '));
        return;
    }

    QTextEdit::keyPressEvent(ev);
}

// ═════════════════════════════════════════════════════════════════
//  Auto-indentado global
// ═════════════════════════════════════════════════════════════════
static QString autoIndentar(const QString& codigo) {
    QStringList lineas = codigo.split('\n');
    QString resultado;
    int indent = 0;
    const int INC = 4;

    for (int i = 0; i < lineas.size(); i++) {
        QString trimmed = lineas[i].trimmed();

        if (trimmed.startsWith("fin_") || trimmed == "sino" || trimmed.startsWith("}"))
            indent = qMax(0, indent - INC);

        resultado += QString(indent, ' ') + trimmed;
        if (i < lineas.size() - 1) resultado += '\n';

        if (trimmed.endsWith("{"))
            indent += INC;
    }
    return resultado;
}

// ═════════════════════════════════════════════════════════════════
//  VentanaPrincipal — constructor
// ═════════════════════════════════════════════════════════════════
VentanaPrincipal::VentanaPrincipal(QWidget* parent) : QMainWindow(parent)
{
    setWindowTitle("MyCompiler — Visualizador Paso a Paso");
    resize(1300, 800);
    setStyleSheet(QString("QMainWindow { background:%1; }").arg(Pal::BG_APP));


    // ── Barra de menú ─────────────────────────────────────────────
    QMenuBar* mb = menuBar();
    mb->setStyleSheet(
        QString("QMenuBar { background:%1; color:%2; font-size:12px;"
                "           border-bottom:1px solid #3a3a3a; padding:1px 4px; }"
                "QMenuBar::item { padding:3px 10px; border-radius:3px; }"
                "QMenuBar::item:selected { background:#3a3a3a; }"
                "QMenu { background:#2d2d2d; color:%2; border:1px solid #444; }"
                "QMenu::item:selected { background:#3b82f6; }")
        .arg(Pal::BG_MENUBAR, Pal::TXT_MAIN)
    );
    QMenu* mArchivo = mb->addMenu("Archivo");
    QMenu* mEditar  = mb->addMenu("Editar");
    QMenu* mVista   = mb->addMenu("Vista");
    QMenu* mAyuda   = mb->addMenu("Ayuda");

    QAction* actNuevo  = mArchivo->addAction("Nuevo");
    QAction* actAbrir  = mArchivo->addAction("Abrir...");
    QAction* actGuard  = mArchivo->addAction("Guardar");
    QAction* actGuardC = mArchivo->addAction("Guardar como...");
    mArchivo->addSeparator();
    QAction* actSalir  = mArchivo->addAction("Salir");
    connect(actNuevo,  &QAction::triggered, this, &VentanaPrincipal::nuevoArchivo);
    connect(actAbrir,  &QAction::triggered, this, &VentanaPrincipal::abrirArchivo);
    connect(actGuard,  &QAction::triggered, this, &VentanaPrincipal::guardarArchivo);
    connect(actGuardC, &QAction::triggered, this, &VentanaPrincipal::guardarComoArchivo);
    connect(actSalir,  &QAction::triggered, this, &VentanaPrincipal::salirAplicacion);
    mEditar->addAction("Deshacer")->setEnabled(false);
    mEditar->addAction("Rehacer")->setEnabled(false);
    mEditar->addSeparator();
    mEditar->addAction("Copiar")->setEnabled(false);
    mEditar->addAction("Pegar")->setEnabled(false);
    mVista->addAction("Tema oscuro")->setEnabled(false);
    mVista->addAction("Aumentar fuente")->setEnabled(false);
    mVista->addAction("Reducir fuente")->setEnabled(false);
    mAyuda->addAction("Acerca de MyCompiler")->setEnabled(false);

    // ── Barra de estado ───────────────────────────────────────────
    statusBar()->setStyleSheet(
        QString("QStatusBar { background:%1; color:%2; font-size:11px; "
                "border-top:1px solid #333; }")
        .arg(Pal::BG_MENUBAR, Pal::TXT_DIM)
    );
    statusBar()->showMessage("Listo.");

    // ── Editor de código ──────────────────────────────────────────
    editorCodigo = new CodeEditor(this);
    editorCodigo->setPlaceholderText("// Escribe tu código aquí...");
    QFont fe("Monospace", 11);
    fe.setStyleHint(QFont::TypeWriter);
    editorCodigo->setFont(fe);
    editorCodigo->setStyleSheet(
        QString("QTextEdit { background:%1; color:%2; border:none; }")
        .arg(Pal::BG_EDITOR, Pal::TXT_MAIN)
    );

    // ── Botones ───────────────────────────────────────────────────
    botonEjecutar  = makeBtn("▶  Compilar",  Pal::ACCENT_BLUE,  Pal::H_BLUE,  this);
    botonAnterior  = makeBtn("◀  Anterior",  Pal::ACCENT_GRAY,  Pal::H_GRAY,  this);
    botonSiguiente = makeBtn("Siguiente  ▶", Pal::ACCENT_GREEN, Pal::H_GREEN, this);

    // Combos
    comboPlantillas = new QComboBox(this);

    comboPlantillas->addItem("Plantillas...");
    comboPlantillas->addItem("Principal");
    comboPlantillas->addItem("#incluir");
    comboPlantillas->addItem("Variables");
    comboPlantillas->addItem("Si");
    comboPlantillas->addItem("Sino si");
    comboPlantillas->addItem("Mientras");
    comboPlantillas->addItem("Para");
    comboPlantillas->addItem("Funcion");
    comboPlantillas->addItem("Arreglo");
    comboPlantillas->addItem("Op. compuestos");
    comboPlantillas->addItem("Incr/Decr");
    comboPlantillas->addItem("Logicos");

    botonSiguiente->setEnabled(false);
    botonAnterior->setEnabled(false);

    etiquetaPaso = new QLabel("—", this);
    etiquetaPaso->setStyleSheet(
        QString("QLabel { color:%1; font-size:11px; padding:0 8px; }").arg(Pal::TXT_DIM));

    // ── Panel derecho de animación ────────────────────────────────
    panelAnimacion = new QWidget(this);
    panelAnimacion->setStyleSheet(
        QString("QWidget { background:%1; }").arg(Pal::BG_PANEL));

    QVBoxLayout* layPanel = new QVBoxLayout(panelAnimacion);
    layPanel->setContentsMargins(12, 12, 12, 12);
    layPanel->setSpacing(8);

    // — Sección variables —
    tituloVariables = new QLabel("📦  Variables en memoria", panelAnimacion);
    tituloVariables->setStyleSheet(
        "QLabel { color:#818cf8; font-weight:700; font-size:12px; background:transparent; }");
    layPanel->addWidget(tituloVariables);

    QScrollArea* scVars = new QScrollArea(panelAnimacion);
    scVars->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scVars->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scVars->setFixedHeight(98);
    scVars->setStyleSheet("QScrollArea { border:1px solid #2a2a3e; border-radius:4px;"
                          "background:transparent; }");
    scVars->setWidgetResizable(true);

    zonaTarjetas = new QWidget();
    zonaTarjetas->setStyleSheet("background:transparent;");
    layoutTarjetas = new QHBoxLayout(zonaTarjetas);
    layoutTarjetas->setContentsMargins(6, 6, 6, 6);
    layoutTarjetas->setSpacing(8);
    layoutTarjetas->addStretch();
    scVars->setWidget(zonaTarjetas);
    layPanel->addWidget(scVars);

    // separador
    QFrame* sep = new QFrame(panelAnimacion);
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("color:#2a2a3e;");
    layPanel->addWidget(sep);

    // — Sección arreglo —
    tituloArreglo = new QLabel("🗂  Arreglo", panelAnimacion);
    tituloArreglo->setStyleSheet(
        "QLabel { color:#fbbf24; font-weight:700; font-size:12px; background:transparent; }");
    tituloArreglo->setVisible(false);
    layPanel->addWidget(tituloArreglo);

    QScrollArea* scArr = new QScrollArea(panelAnimacion);
    scArr->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scArr->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scArr->setFixedHeight(98);
    scArr->setStyleSheet("QScrollArea { border:1px solid #2a2a3e; border-radius:4px;"
                         "background:transparent; }");
    scArr->setWidgetResizable(true);
    scArr->setVisible(false);
    tituloArreglo->setProperty("scrollRef", QVariant::fromValue((void*)scArr));

    zonaArreglo = new QWidget();
    zonaArreglo->setStyleSheet("background:transparent;");
    layoutArreglo = new QHBoxLayout(zonaArreglo);
    layoutArreglo->setContentsMargins(6, 6, 6, 6);
    layoutArreglo->setSpacing(6);
    layoutArreglo->addStretch();
    scArr->setWidget(zonaArreglo);
    layPanel->addWidget(scArr);

    // — Etiqueta de estado —
    etiquetaEstado = new QLabel("", panelAnimacion);
    etiquetaEstado->setWordWrap(true);
    etiquetaEstado->setVisible(false);
    etiquetaEstado->setStyleSheet(
        "QLabel { color:#86efac; background:#052e16; border:1px solid #166534;"
        "border-radius:4px; padding:5px 8px; font-size:11px; }");
    layPanel->addWidget(etiquetaEstado);

    layPanel->addStretch();

    // ── Widget central ────────────────────────────────────────────
    QWidget* central = new QWidget(this);
    setCentralWidget(central);

    QVBoxLayout* layMain = new QVBoxLayout(central);
    layMain->setContentsMargins(6, 4, 6, 4);
    layMain->setSpacing(4);

    // — Barra de herramientas (toolbar row) —
    QWidget* toolbar = new QWidget(this);
    toolbar->setFixedHeight(36);
    toolbar->setStyleSheet(
        "QWidget { background:#252526; border-bottom:1px solid #333; border-radius:0; }");
    QHBoxLayout* layToolbar = new QHBoxLayout(toolbar);
    layToolbar->setContentsMargins(8, 4, 8, 4);
    layToolbar->setSpacing(6);
    
    layToolbar->addWidget(botonEjecutar);
    layToolbar->addSpacing(10);
    layToolbar->addWidget(comboPlantillas);
    layToolbar->addSpacing(10);

    // separador visual entre compilar y navegación
    QFrame* sepBtn = new QFrame(toolbar);
    sepBtn->setFrameShape(QFrame::VLine);
    sepBtn->setStyleSheet("QFrame { color:#444; }");
    layToolbar->addWidget(sepBtn);
    layToolbar->addSpacing(4);

    layToolbar->addWidget(botonAnterior);
    layToolbar->addWidget(botonSiguiente);
    layToolbar->addWidget(etiquetaPaso);
    layToolbar->addStretch();
    layMain->addWidget(toolbar);

    // — Splitter horizontal: izquierda (editor+consola) / derecha (panel) —
    QSplitter* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setHandleWidth(4);
    splitter->setStyleSheet(
        "QSplitter::handle { background:#333; }");

    // lado izquierdo: solo editor
    QWidget* ladoIzq = new QWidget();
    ladoIzq->setStyleSheet("background:transparent;");
    QVBoxLayout* layIzq = new QVBoxLayout(ladoIzq);
    layIzq->setContentsMargins(0,0,0,0);
    layIzq->setSpacing(0);
    layIzq->addWidget(editorCodigo, 1);

    splitter->addWidget(ladoIzq);
    splitter->addWidget(panelAnimacion);
    splitter->setSizes({680, 520});

    layMain->addWidget(splitter, 1);
    
    // ── Conexiones ────────────────────────────────────────────────
    connect(botonEjecutar,  &QPushButton::clicked, this, &VentanaPrincipal::manejarEjecucion);
    connect(botonSiguiente, &QPushButton::clicked, this, &VentanaPrincipal::avanzarPaso);
    connect(botonAnterior,  &QPushButton::clicked, this, &VentanaPrincipal::retrocederPaso);
    connect(comboPlantillas,
        &QComboBox::currentTextChanged,
        this,
        &VentanaPrincipal::cargarPlantilla);
}

// ═════════════════════════════════════════════════════════════════
//  Terminal emergente
// ═════════════════════════════════════════════════════════════════
void VentanaPrincipal::mostrarTerminal(const std::string& contenido) {
    auto* term = new QDialog(this);
    term->setWindowTitle("Terminal — MyCompiler");
    term->resize(720, 420);
    term->setAttribute(Qt::WA_DeleteOnClose);

    auto* lay = new QVBoxLayout(term);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);

    auto* output = new QTextEdit(term);
    output->setReadOnly(true);
    QFont f("Courier New", 11);
    f.setStyleHint(QFont::TypeWriter);
    output->setFont(f);
    output->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    output->setStyleSheet(
        "QTextEdit {"
        "  background:#0d1117; color:#e6edf3;"
        "  border:none; padding:12px;"
        "  selection-background-color:#264f78;"
        "}"
        "QScrollBar:vertical {"
        "  background:#0d1117; width:10px;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background:#30363d; border-radius:5px; min-height:20px;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "  height:0;"
        "}"
    );
    output->setHtml(
        QString("<pre style='font-family:\"Courier New\",monospace;"
                "margin:0; white-space:pre-wrap; color:#e6edf3;'>%1</pre>")
        .arg(QString::fromStdString(contenido))
    );

    auto* barra = new QWidget(term);
    barra->setFixedHeight(36);
    barra->setStyleSheet("background:#161b22; border-top:1px solid #30363d;");
    auto* layBarra = new QHBoxLayout(barra);
    layBarra->setContentsMargins(8, 4, 8, 4);

    auto* btnCerrar = new QPushButton("Cerrar", barra);
    btnCerrar->setFixedHeight(26);
    btnCerrar->setCursor(Qt::PointingHandCursor);
    btnCerrar->setStyleSheet(
        "QPushButton {"
        "  background:#238636; color:#fff; font-weight:600; font-size:12px;"
        "  border:none; border-radius:4px; padding:0 18px;"
        "}"
        "QPushButton:hover { background:#2ea043; }"
    );
    connect(btnCerrar, &QPushButton::clicked, term, &QDialog::close);

    layBarra->addStretch();
    layBarra->addWidget(btnCerrar);

    lay->addWidget(output, 1);
    lay->addWidget(barra, 0);

    term->show();
}

// ═════════════════════════════════════════════════════════════════
//  Acciones de Archivo
// ═════════════════════════════════════════════════════════════════
void VentanaPrincipal::nuevoArchivo() {
    editorCodigo->clear();
    rutaActual.clear();
    statusBar()->showMessage("Nuevo archivo");
}

void VentanaPrincipal::abrirArchivo() {
    QString ruta = QFileDialog::getOpenFileName(this, "Abrir código",
        "programas", "Archivos de código (*.txt)");
    if (ruta.isEmpty()) return;

    QFile f(ruta);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "No se pudo abrir el archivo.");
        return;
    }

    QString codigo = autoIndentar(QString::fromUtf8(f.readAll()));
    f.close();

    editorCodigo->setPlainText(codigo);
    rutaActual = ruta;
    statusBar()->showMessage("Abierto: " + ruta);
}

void VentanaPrincipal::guardarArchivo() {
    if (!rutaActual.isEmpty()) {
        QFile f(rutaActual);
        if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QString codigo = autoIndentar(editorCodigo->toPlainText());
            f.write(codigo.toUtf8());
            f.close();
            statusBar()->showMessage("Guardado: " + rutaActual);
            return;
        }
    }
    QDir().mkpath("programas");
    QString nombre = QDateTime::currentDateTime().toString("yyMMdd_HHmmss") + ".txt";
    rutaActual = QDir("programas").absoluteFilePath(nombre);
    QFile f(rutaActual);
    if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QString codigo = autoIndentar(editorCodigo->toPlainText());
        f.write(codigo.toUtf8());
        f.close();
        statusBar()->showMessage("Guardado: " + rutaActual);
    }
}

void VentanaPrincipal::guardarComoArchivo() {
    QDir().mkpath("programas");

    QString ruta = QFileDialog::getSaveFileName(this, "Guardar como",
        "programas/", "Archivos de código (*.txt)");
    if (ruta.isEmpty()) return;

    QFile f(ruta);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "No se pudo guardar el archivo.");
        return;
    }

    QString codigo = autoIndentar(editorCodigo->toPlainText());
    f.write(codigo.toUtf8());
    f.close();

    rutaActual = ruta;
    statusBar()->showMessage("Guardado: " + ruta);
}

void VentanaPrincipal::closeEvent(QCloseEvent* ev) {
    QProcess::startDetached("make", QStringList{"clean"});
    QMainWindow::closeEvent(ev);
}

void VentanaPrincipal::salirAplicacion() {
    close();
}

void VentanaPrincipal::cargarPlantilla(const QString& nombre)
    {
        if(nombre == "Plantillas...")
            return;

        if(nombre == "Principal")
        {
            editorCodigo->clear();
            QString texto =
                "\nPrincipal() {\n"
                "    mostrar(\"Hola Mundo\");\n"
                "}\n";
            QTextCursor cursor = editorCodigo->textCursor();
            cursor.insertText(texto);
            comboPlantillas->setCurrentIndex(0);
            return;
        }

        QString codigo;

        if(nombre == "#incluir")
        {
            codigo =
                "\n#incluir \"matematica.txt\"\n";
        }

        else if(nombre == "Variables")
        {
            codigo =
                "\nentero edad = 18;\n"
                "decimal altura = 1.75;\n"
                "booleano activo = verdadero;\n"
                "caracter inicial = 'R';\n"
                "cadena nombre = \"Juan\";\n";
        }

        else if(nombre == "Si")
        {
            codigo =
                "\nentero edad = 18;\n"
                "si(edad >= 18){\n"
                "    mostrar(\"Mayor de edad\");\n"
                "} sino {\n"
                "    mostrar(\"Menor de edad\");\n"
                "}\n"
                "fin_si\n";
        }

        else if(nombre == "Sino si")
        {
            codigo =
                "\nentero nota = 85;\n"
                "si(nota >= 90){\n"
                "    mostrar(\"Sobresaliente\");\n"
                "} sino si(nota >= 70){\n"
                "    mostrar(\"Aprobado\");\n"
                "} sino {\n"
                "    mostrar(\"Reprobado\");\n"
                "}\n"
                "fin_si\n";
        }

        else if(nombre == "Mientras")
        {
            codigo =
                "\nentero i = 0;\n"
                "mientras(i < 5){\n"
                "    mostrar(i);\n"
                "    i++;\n"
                "}\n"
                "fin_mientras\n";
        }

        else if(nombre == "Para")
        {
            codigo =
                "\npara(entero i = 0; i < 5; i++){\n"
                "    mostrar(i);\n"
                "}\n"
                "fin_para\n";
        }

        else if(nombre == "Funcion")
        {
            codigo =
                "\nentero sumar(entero a, entero b){\n"
                "    retornar a + b;\n"
                "}\n";
        }

        else if(nombre == "Arreglo")
        {
            codigo =
                "\narreglo entero numeros[5];\n"
                "numeros[0] = 10;\n"
                "mostrar(numeros[0]);\n";
        }

        else if(nombre == "Op. compuestos")
        {
            codigo =
                "\nentero x = 10;\n"
                "x += 5;\n"
                "x -= 3;\n"
                "x *= 2;\n"
                "mostrar(x);\n";
        }

        else if(nombre == "Incr/Decr")
        {
            codigo =
                "\nentero c = 0;\n"
                "c++;\n"
                "mostrar(c);\n"
                "c--;\n"
                "mostrar(c);\n";
        }

        else if(nombre == "Logicos")
        {
            codigo =
                "\nentero a = 5;\n"
                "entero b = 10;\n"
                "si(a > 0 && b > 0){\n"
                "    mostrar(\"ambos positivos\");\n"
                "}\n"
                "si(!(a == b)){\n"
                "    mostrar(\"son diferentes\");\n"
                "}\n"
                "fin_si\n"
                "fin_si\n";
        }

        QTextCursor cursor = editorCodigo->textCursor();
        cursor.insertText(codigo);

        comboPlantillas->setCurrentIndex(0);
    }

void VentanaPrincipal::manejarEjecucion() {
    pasos.clear();
    historial.clear();
    pasoActual = 0;
    estadoActual = SnapshotUI();
    limpiarTarjetas();
    limpiarArreglo();
    etiquetaEstado->setVisible(false);
    resaltarLinea(-1);

    std::string src = editorCodigo->toPlainText().toStdString();

    std::stringstream buf;
    std::streambuf* old = std::cout.rdbuf(buf.rdbuf());

    QString html;

    setInputHook([]() -> std::string {
        bool ok;
        QString r = QInputDialog::getText(nullptr, "Entrada requerida",
            "Ingresa un valor:", QLineEdit::Normal, "", &ok);
        return ok ? r.toStdString() : "0";
    });

    try {
        std::string expanded = preprocesarBibliotecas(src);
        auto tokens = tokenizar(expanded);
        parsear(tokens, &pasos);
        setInputHook(nullptr);
        std::cout.rdbuf(old);

        QString salida = QString::fromStdString(buf.str()).toHtmlEscaped()
                            .replace("\n","<br>");
        html += salida;

        html += QString("<br><span style='color:#22c55e;'>✓ %1 pasos generados."
                        " Presiona Siguiente para animar.</span>")
                .arg(pasos.size());
        statusBar()->showMessage(QString("Compilado — %1 pasos").arg(pasos.size()));

    } catch (const std::exception& e) {
        setInputHook(nullptr);
        std::cout.rdbuf(old);
        QString salida = QString::fromStdString(buf.str()).toHtmlEscaped()
                            .replace("\n","<br>");
        if (!salida.isEmpty())
            html += salida;
        html += QString("<br><span style='color:#f87171;'>%1</span>")
                .arg(QString::fromStdString(e.what()).toHtmlEscaped()
                     .replace("\n","<br>"));
        statusBar()->showMessage("Error de compilación.");
    }

    actualizarBotones();
    mostrarTerminal(html.toStdString());
}

// ═════════════════════════════════════════════════════════════════
//  Navegación paso a paso
// ═════════════════════════════════════════════════════════════════
void VentanaPrincipal::avanzarPaso() {
    if (pasoActual >= (int)pasos.size()) return;
    historial.push_back(estadoActual);   // guardar estado ANTES de aplicar
    aplicarEvento(pasos[pasoActual]);
    pasoActual++;
    actualizarBotones();
}

void VentanaPrincipal::retrocederPaso() {
    if (historial.empty()) return;
    pasoActual--;
    SnapshotUI snap = historial.back();
    historial.pop_back();
    restaurarSnapshot(snap);
    actualizarBotones();
}

// ═════════════════════════════════════════════════════════════════
//  aplicarEvento
// ═════════════════════════════════════════════════════════════════
void VentanaPrincipal::aplicarEvento(const EventoPaso& ev) {

    auto setVar = [&](bool esNueva){
        resaltarLinea(ev.linea);
        estadoActual.lineaActiva = ev.linea;
        actualizarTarjetaVariable(ev.nombre, ev.valor, esNueva);
        bool found = false;
        for (auto& p : estadoActual.variables)
            if (p.first == ev.nombre){ p.second = ev.valor; found = true; break; }
        if (!found) estadoActual.variables.push_back({ev.nombre, ev.valor});
        std::string msg = (esNueva ? "Declarada: " : "Modificada: ") + ev.nombre + " = " + ev.valor;
        estadoActual.mensajeEstado = msg;
        estadoActual.mensajeError  = false;
        setMensajeEstado(msg, false);
    };

    switch (ev.tipo) {

    case TipoEvento::LINEA_ACTIVA:
        resaltarLinea(ev.linea);
        estadoActual.lineaActiva = ev.linea;
        break;

    case TipoEvento::VAR_DECLARADA:  setVar(true);  break;
    case TipoEvento::VAR_MODIFICADA: setVar(false); break;

    case TipoEvento::VAR_LEIDA:
        resaltarLinea(ev.linea);
        estadoActual.mensajeEstado = "Leyendo: " + ev.nombre + " = " + ev.valor;
        estadoActual.mensajeError  = false;
        setMensajeEstado(estadoActual.mensajeEstado, false);
        break;

    case TipoEvento::ARREGLO_DECLARADO:
        resaltarLinea(ev.linea);
        estadoActual.lineaActiva = ev.linea;
        estadoActual.nombreArr   = ev.nombre;
        estadoActual.celdas      = ev.celdas;
        estadoActual.indiceArr   = -1;
        actualizarVistaArreglo(ev.nombre, ev.celdas, -1);
        tituloArreglo->setText(
            QString("🗂  %1  [%2]").arg(ev.nombre.c_str()).arg(ev.celdas.size()));
        {
            std::string msg = "Arreglo '" + ev.nombre + "' creado con " +
                              std::to_string(ev.celdas.size()) + " celdas";
            estadoActual.mensajeEstado = msg;
            estadoActual.mensajeError  = false;
            setMensajeEstado(msg, false);
        }
        break;

    case TipoEvento::ARREGLO_ESCRITO:
        resaltarLinea(ev.linea);
        estadoActual.lineaActiva = ev.linea;
        estadoActual.celdas      = ev.celdas;
        estadoActual.indiceArr   = ev.indice;
        actualizarVistaArreglo(ev.nombre, ev.celdas, ev.indice);
        {
            std::string msg = ev.nombre + "[" + std::to_string(ev.indice) + "] ← " + ev.valor;
            estadoActual.mensajeEstado = msg;
            estadoActual.mensajeError  = false;
            setMensajeEstado(msg, false);
        }
        break;

    case TipoEvento::ARREGLO_LEIDO:
        resaltarLinea(ev.linea);
        estadoActual.indiceArr = ev.indice;
        estadoActual.celdas    = ev.celdas;
        actualizarVistaArreglo(ev.nombre, ev.celdas, ev.indice);
        {
            std::string msg = "Leyendo " + ev.nombre + "[" +
                              std::to_string(ev.indice) + "] = " + ev.valor;
            estadoActual.mensajeEstado = msg;
            estadoActual.mensajeError  = false;
            setMensajeEstado(msg, false);
        }
        break;

    case TipoEvento::BUCLE_CONDICION:
        resaltarLinea(ev.linea);
        estadoActual.lineaActiva = ev.linea;
        {
            bool ok = (ev.valor == "verdadero");
            std::string msg = std::string("Condición: ") +
                (ok ? "✓ verdadero — entra al bucle" : "✗ falso — sale del bucle");
            estadoActual.mensajeEstado = msg;
            estadoActual.mensajeError  = !ok;
            setMensajeEstado(msg, !ok);
        }
        break;

    case TipoEvento::BUCLE_FIN:
        resaltarLinea(ev.linea);
        estadoActual.mensajeEstado = "Fin del bucle mientras";
        estadoActual.mensajeError  = false;
        setMensajeEstado(estadoActual.mensajeEstado, false);
        break;

    case TipoEvento::CONDICION_SI:
        resaltarLinea(ev.linea);
        estadoActual.lineaActiva = ev.linea;
        {
            bool ok = (ev.valor == "verdadero");
            std::string msg = std::string("si: condición ") + (ok ? "✓ verdadero" : "✗ falso");
            estadoActual.mensajeEstado = msg;
            estadoActual.mensajeError  = !ok;
            setMensajeEstado(msg, !ok);
        }
        break;

    case TipoEvento::FUNCION_ENTRADA:
        resaltarLinea(ev.linea);
        estadoActual.mensajeEstado = "→ Entrando a: " + ev.nombre + "()";
        estadoActual.mensajeError  = false;
        setMensajeEstado(estadoActual.mensajeEstado, false);
        break;

    case TipoEvento::FUNCION_RETORNO:
        resaltarLinea(ev.linea);
        estadoActual.mensajeEstado = "← Retorna: " + ev.valor;
        estadoActual.mensajeError  = false;
        setMensajeEstado(estadoActual.mensajeEstado, false);
        break;

    case TipoEvento::MOSTRAR_SALIDA:
        resaltarLinea(ev.linea);
        estadoActual.mensajeEstado = "mostrar: " + ev.valor;
        estadoActual.mensajeError  = false;
        setMensajeEstado(estadoActual.mensajeEstado, false);
        break;

    case TipoEvento::LEER_SOLICITUD:
        resaltarLinea(ev.linea);
        estadoActual.lineaActiva = ev.linea;
        estadoActual.mensajeEstado = "⌨ Solicitando entrada para: " + ev.nombre;
        estadoActual.mensajeError  = false;
        setMensajeEstado(estadoActual.mensajeEstado, false);
        break;
    }
}

// ═════════════════════════════════════════════════════════════════
//  restaurarSnapshot  — reconstruye TODO el estado visual
// ═════════════════════════════════════════════════════════════════
void VentanaPrincipal::restaurarSnapshot(const SnapshotUI& snap) {
    estadoActual = snap;

    // Reconstruir tarjetas de variables (todas, sin animación)
    limpiarTarjetas();
    for (auto& [nombre, valor] : snap.variables)
        actualizarTarjetaVariable(nombre, valor, false);

    // Reconstruir arreglo
    if (!snap.nombreArr.empty()) {
        tituloArreglo->setText(
            QString("🗂  %1  [%2]").arg(snap.nombreArr.c_str()).arg(snap.celdas.size()));
        actualizarVistaArreglo(snap.nombreArr, snap.celdas, snap.indiceArr);
    } else {
        limpiarArreglo();
    }

    resaltarLinea(snap.lineaActiva);

    if (!snap.mensajeEstado.empty())
        setMensajeEstado(snap.mensajeEstado, snap.mensajeError);
    else
        etiquetaEstado->setVisible(false);
}

// ═════════════════════════════════════════════════════════════════
//  resaltarLinea
// ═════════════════════════════════════════════════════════════════
void VentanaPrincipal::resaltarLinea(int linea) {
    QList<QTextEdit::ExtraSelection> sels;
    if (linea > 0) {
        QTextEdit::ExtraSelection sel;
        sel.format.setBackground(QColor("#1e3a5f"));
        sel.format.setProperty(QTextFormat::FullWidthSelection, true);

        QTextCursor cur(editorCodigo->document());
        cur.movePosition(QTextCursor::Start);
        for (int i = 1; i < linea; ++i)
            cur.movePosition(QTextCursor::NextBlock);
        cur.movePosition(QTextCursor::StartOfBlock);
        sel.cursor = cur;
        sels.append(sel);

        editorCodigo->setTextCursor(cur);
        editorCodigo->ensureCursorVisible();
    }
    editorCodigo->setExtraSelections(sels);
}

// ═════════════════════════════════════════════════════════════════
//  Tarjetas de variable
// ═════════════════════════════════════════════════════════════════
void VentanaPrincipal::actualizarTarjetaVariable(const std::string& nombre,
                                                   const std::string& valor,
                                                   bool esNueva)
{
    QString qNombre = QString::fromStdString(nombre);
    QString qValor  = QString::fromStdString(valor);
    QString objName = "var_" + qNombre;

    QString colorBorde = esNueva ? "#818cf8" : "#38bdf8";
    QString estilo =
        QString("QLabel { background:#0f172a; color:#e2e8f0;"
                "border:1px solid %1; border-radius:6px;"
                "padding:6px 10px; font-family:'Courier New'; font-size:12px;"
                "min-width:60px; }").arg(colorBorde);

    QString txt = QString("<div style='color:%1;font-weight:700;'>%2</div>"
                          "<div style='color:#f8fafc;'>%3</div>")
                  .arg(colorBorde, qNombre, qValor);

    QList<QLabel*> found = zonaTarjetas->findChildren<QLabel*>(objName);
    if (!found.isEmpty()) {
        found.first()->setText(txt);
        found.first()->setStyleSheet(estilo);
    } else {
        QLabel* card = new QLabel(zonaTarjetas);
        card->setObjectName(objName);
        card->setText(txt);
        card->setStyleSheet(estilo);
        card->setAlignment(Qt::AlignCenter);
        card->setTextFormat(Qt::RichText);
        layoutTarjetas->insertWidget(layoutTarjetas->count()-1, card);

        if (esNueva) {
            // animación solo al declarar por primera vez
            card->setMaximumWidth(0);
            auto* anim = new QPropertyAnimation(card, "maximumWidth");
            anim->setDuration(200);
            anim->setStartValue(0);
            anim->setEndValue(105);
            anim->setEasingCurve(QEasingCurve::OutCubic);
            anim->start(QAbstractAnimation::DeleteWhenStopped);
        } else {
            // restauración instantánea (sin animación)
            card->setMaximumWidth(105);
        }
    }
}

// ═════════════════════════════════════════════════════════════════
//  Vista del arreglo
// ═════════════════════════════════════════════════════════════════
void VentanaPrincipal::actualizarVistaArreglo(const std::string& /*nombre*/,
                                               const std::vector<std::string>& celdas,
                                               int indiceActivo)
{
    tituloArreglo->setVisible(true);
    void* ptr = tituloArreglo->property("scrollRef").value<void*>();
    if (ptr) static_cast<QScrollArea*>(ptr)->setVisible(true);

    // limpiar
    while (layoutArreglo->count() > 1) {
        QLayoutItem* it = layoutArreglo->takeAt(0);
        if (it->widget()) delete it->widget();
        delete it;
    }

    for (int i = 0; i < (int)celdas.size(); ++i) {
        bool activa = (i == indiceActivo);

        QWidget* celda = new QWidget(zonaArreglo);
        celda->setFixedSize(66, 74);

        QVBoxLayout* lc = new QVBoxLayout(celda);
        lc->setContentsMargins(0,0,0,0);
        lc->setSpacing(2);

        if (activa) {
            QLabel* arr = new QLabel("▼", celda);
            arr->setAlignment(Qt::AlignCenter);
            arr->setStyleSheet("color:#fbbf24; font-size:11px; background:transparent;");
            lc->addWidget(arr);
        }

        QLabel* val = new QLabel(QString::fromStdString(celdas[i]), celda);
        val->setAlignment(Qt::AlignCenter);
        val->setStyleSheet(
            QString("QLabel { background:%1; color:%2; border:1px solid %3;"
                    "border-radius:4px; font-family:'Courier New';"
                    "font-size:13px; font-weight:700; min-height:38px; }")
            .arg(activa ? "#2d1d00" : "#0f172a")
            .arg(activa ? "#fbbf24" : "#93c5fd")
            .arg(activa ? "#fbbf24" : "#1e3a5f")
        );

        QLabel* idx = new QLabel(QString("[%1]").arg(i), celda);
        idx->setAlignment(Qt::AlignCenter);
        idx->setStyleSheet(
            QString("QLabel { color:%1; font-size:10px; background:transparent; }")
            .arg(activa ? "#fbbf24" : "#4b5563"));

        lc->addWidget(val);
        lc->addWidget(idx);

        layoutArreglo->insertWidget(layoutArreglo->count()-1, celda);
    }
}

// ═════════════════════════════════════════════════════════════════
//  Limpiar
// ═════════════════════════════════════════════════════════════════
void VentanaPrincipal::limpiarTarjetas() {
    while (layoutTarjetas->count() > 1) {
        QLayoutItem* it = layoutTarjetas->takeAt(0);
        if (it->widget()) delete it->widget();
        delete it;
    }
}

void VentanaPrincipal::limpiarArreglo() {
    tituloArreglo->setVisible(false);
    void* ptr = tituloArreglo->property("scrollRef").value<void*>();
    if (ptr) static_cast<QScrollArea*>(ptr)->setVisible(false);
    while (layoutArreglo->count() > 1) {
        QLayoutItem* it = layoutArreglo->takeAt(0);
        if (it->widget()) delete it->widget();
        delete it;
    }
}

// ═════════════════════════════════════════════════════════════════
//  Etiqueta de estado
// ═════════════════════════════════════════════════════════════════
void VentanaPrincipal::setMensajeEstado(const std::string& msg, bool esError) {
    etiquetaEstado->setText(QString::fromStdString(msg));
    if (esError) {
        etiquetaEstado->setStyleSheet(
            "QLabel { color:#fca5a5; background:#3b0000; border:1px solid #7f1d1d;"
            "border-radius:4px; padding:5px 8px; font-size:11px; }");
    } else {
        etiquetaEstado->setStyleSheet(
            "QLabel { color:#86efac; background:#052e16; border:1px solid #166534;"
            "border-radius:4px; padding:5px 8px; font-size:11px; }");
    }
    etiquetaEstado->setVisible(true);
}

// ═════════════════════════════════════════════════════════════════
//  Botones y etiqueta de paso
// ═════════════════════════════════════════════════════════════════
void VentanaPrincipal::actualizarBotones() {
    bool hay = !pasos.empty();
    botonSiguiente->setEnabled(hay && pasoActual < (int)pasos.size());
    botonAnterior->setEnabled(!historial.empty());
    etiquetaPaso->setText(
        hay ? QString("Paso %1 / %2").arg(pasoActual).arg(pasos.size()) : "—");
}

