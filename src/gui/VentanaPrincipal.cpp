#include "gui/VentanaPrincipal.hpp"
#include "gui/syntax_highlighter.hpp"
#include "core/lexer.hpp"
#include "core/eventos.hpp"
#include <vector>
#include <set>
#include <regex>
#include <sstream>
#include <iostream>
#include <QFont>
#include <QFrame>
#include <QScrollArea>
#include <QPainter>
#include <QPainterPath>
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
#include "gui/theme.hpp"

// ── Prototipos externos ───────────────────────────────────────────
std::vector<Token>  tokenizar(const std::string&);
double              parsear(std::vector<Token>&, std::vector<EventoPaso>* = nullptr);
std::string         preprocesarBibliotecas(const std::string&);
void                preprocesarBibliotecas(const std::string&, std::string&, std::vector<int>&);
void                setInputHook(std::string (*hook)());

// ─── Helpers para vista solo-funciones de biblioteca ─────────────
static std::vector<std::string> _splitLines(const std::string& s) {
    std::vector<std::string> lines;
    std::string line;
    for (char c : s) {
        if (c == '\n') {
            lines.push_back(line);
            line.clear();
        } else {
            line += c;
        }
    }
    if (!line.empty()) lines.push_back(line);
    return lines;
}

struct _FuncInfo {
    std::string nombre;
    std::string cuerpo;
    int lineaInicio = 0; // 1-indexed en expanded
    int lineaFin    = 0;
    bool usada = false;
};

// Extrae el cuerpo de una función desde su línea de definición hasta el '}' de cierre
static std::string _extraerFuncion(const std::vector<std::string>& lines,
                                    int defLine, int& endLine)
{
    // Buscar la línea que contiene '{'
    int braceStart = -1;
    for (int j = defLine - 1; j < (int)lines.size(); j++) {
        if (lines[j].find('{') != std::string::npos) {
            braceStart = j; break;
        }
    }
    if (braceStart < 0) return "";

    int braces = 0;
    bool opened = false;
    for (int j = braceStart; j < (int)lines.size(); j++) {
        for (char c : lines[j]) {
            if (c == '{') { braces++; opened = true; }
            if (c == '}') { braces--; }
        }
        if (opened && braces == 0) {
            endLine = j + 1; // 1-indexed
            break;
        }
    }
    if (endLine <= 0) return "";

    std::string cuerpo;
    for (int j = defLine - 1; j < endLine; j++) {
        cuerpo += lines[j] + "\n";
    }
    return cuerpo;
}

// ═════════════════════════════════════════════════════════════════
//  Helper: crear botón con estilo unificado
// ═════════════════════════════════════════════════════════════════
static QPushButton* makeBtn(const QString& label,
                             const QString& bg, const QString& hover,
                             QWidget* parent = nullptr)
{
    auto* b = new QPushButton(label, parent);
    b->setFixedHeight(26);
    b->setCursor(Qt::PointingHandCursor);
    b->setStyleSheet(
        QString(
        "QPushButton { background:%1; color:%2;"
        "  font-family:'Segoe UI',sans-serif; font-size:12px; font-weight:600;"
        "  border:none; border-radius:4px; padding:0 14px;"
        "}"
        "QPushButton:hover  { background:%3; }"
        "QPushButton:pressed{ background:%3; opacity:0.85; }"
        "QPushButton:disabled{ background:%4; color:%5; border:1px solid %6; }"
        ).arg(bg, Theme::TXT_BTN, hover,
              Theme::BG_DISABLED, Theme::TXT_DISABLED, Theme::BORDER_MUTED)
    );
    return b;
}

// ═════════════════════════════════════════════════════════════════
//  LineNumberArea
// ═════════════════════════════════════════════════════════════════
LineNumberArea::LineNumberArea(QTextEdit* editor) : QWidget(editor), _ed(editor) {
    _animPuntero = new QPropertyAnimation(this, "punteroY", this);
    _animPuntero->setDuration(180);
    _animPuntero->setEasingCurve(QEasingCurve::OutCubic);
}

void LineNumberArea::setLineaActiva(int linea) {
    _lineaActiva = linea;
    if (linea <= 0) {
        if (_animPuntero->state() == QAbstractAnimation::Running)
            _animPuntero->stop();
        _punteroY = -30.0;
        update();
        return;
    }

    QTextBlock blk = _ed->document()->findBlockByNumber(linea - 1);
    if (!blk.isValid()) return;

    auto* layout = _ed->document()->documentLayout();
    QRectF br = layout->blockBoundingRect(blk)
                     .translated(0, -_ed->verticalScrollBar()->value());
    qreal targetY = br.top() + 2;

    if (_animPuntero->state() == QAbstractAnimation::Running)
        _animPuntero->stop();

    _animPuntero->setStartValue(_punteroY);
    _animPuntero->setEndValue(targetY);
    _animPuntero->start();
}

void LineNumberArea::setPunteroY(qreal y) {
    _punteroY = y;
    update();
}

int LineNumberArea::gutterWidth() {
    int digits = 1, max = std::max(1, _ed->document()->blockCount());
    while (max >= 10) { max /= 10; ++digits; }
    return 10 + digits * fontMetrics().horizontalAdvance('0');
}

void LineNumberArea::paintEvent(QPaintEvent* ev) {
    QPainter p(this);
    p.fillRect(ev->rect(), QColor(Theme::BG_GUTTER));

    // línea separadora derecha
    p.setPen(QColor(Theme::BORDER_MUTED));
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
            p.setPen(QColor(Theme::TXT_DIM));
            p.drawText(0, top, width()-6, h,
                       Qt::AlignRight | Qt::AlignTop,
                       QString::number(num));
        }
        ++num;
        blk = blk.next();
    }

    // Flecha animada en la línea activa
    if (_punteroY > -20.0) {
        qreal arrowX = width() - 8.0;
        QPainterPath path;
        path.moveTo(arrowX,     _punteroY);
        path.lineTo(arrowX + 8, _punteroY + 6);
        path.lineTo(arrowX,     _punteroY + 12);
        path.closeSubpath();
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(Theme::GUTTER_ARROW));
        p.setRenderHint(QPainter::Antialiasing);
        p.drawPath(path);
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
    setStyleSheet(QString("QMainWindow { background:%1; }").arg(Theme::BG_APP));


    // ── Barra de menú ─────────────────────────────────────────────
    QMenuBar* mb = menuBar();
    mb->setStyleSheet(
        QString("QMenuBar { background:%1; color:%2; font-size:12px;"
                "           border-bottom:1px solid %3; padding:1px 4px; }"
                "QMenuBar::item { padding:3px 10px; border-radius:3px; }"
                "QMenuBar::item:selected { background:%3; }"
                "QMenu { background:%4; color:%2; border:1px solid %5; }"
                "QMenu::item:selected { background:%6; }")
        .arg(Theme::BG_MENUBAR, Theme::TXT_MAIN,
             Theme::BORDER_MUTED, Theme::BG_MENU,
             Theme::BORDER_DIM, Theme::ACCENT_BORDER)
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

    QAction* actUndo = mEditar->addAction("Deshacer");
    actUndo->setShortcut(QKeySequence::Undo);
    connect(actUndo, &QAction::triggered, this, [this]{ editorCodigo->undo(); });

    QAction* actRedo = mEditar->addAction("Rehacer");
    actRedo->setShortcut(QKeySequence::Redo);
    connect(actRedo, &QAction::triggered, this, [this]{ editorCodigo->redo(); });

    mEditar->addSeparator();

    QAction* actCut = mEditar->addAction("Cortar");
    actCut->setShortcut(QKeySequence::Cut);
    connect(actCut, &QAction::triggered, this, [this]{ editorCodigo->cut(); });

    QAction* actCopy = mEditar->addAction("Copiar");
    actCopy->setShortcut(QKeySequence::Copy);
    connect(actCopy, &QAction::triggered, this, [this]{ editorCodigo->copy(); });

    QAction* actPaste = mEditar->addAction("Pegar");
    actPaste->setShortcut(QKeySequence::Paste);
    connect(actPaste, &QAction::triggered, this, [this]{ editorCodigo->paste(); });

    QAction* actSelAll = mEditar->addAction("Seleccionar todo");
    actSelAll->setShortcut(QKeySequence::SelectAll);
    connect(actSelAll, &QAction::triggered, this, [this]{ editorCodigo->selectAll(); });

    QAction* actTheme = mVista->addAction("Tema oscuro");
    actTheme->setCheckable(true);
    actTheme->setChecked(true);
    connect(actTheme, &QAction::triggered, this, [this, actTheme]{
        toggleTheme();
        actTheme->setText(_temaOscuro ? "Tema oscuro" : "Tema claro");
    });

    QAction* actFontUp = mVista->addAction("Aumentar fuente");
    actFontUp->setShortcut(QKeySequence::ZoomIn);
    connect(actFontUp, &QAction::triggered, this, &VentanaPrincipal::aumentarFuente);

    QAction* actFontDown = mVista->addAction("Reducir fuente");
    actFontDown->setShortcut(QKeySequence::ZoomOut);
    connect(actFontDown, &QAction::triggered, this, &VentanaPrincipal::reducirFuente);

    QAction* actAbout = mAyuda->addAction("Acerca de MyCompiler");
    connect(actAbout, &QAction::triggered, this, &VentanaPrincipal::acercaDe);

    // ── Barra de estado ───────────────────────────────────────────
    statusBar()->setStyleSheet(
        QString("QStatusBar { background:%1; color:%2; font-size:11px; "
                "border-top:1px solid %3; }")
        .arg(Theme::BG_MENUBAR, Theme::TXT_DIM, Theme::BORDER_SUBTLE)
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
        .arg(Theme::BG_EDITOR, Theme::TXT_MAIN)
    );

    _hlCodigo = new SyntaxHighlighter(editorCodigo->document());

    // ── Editor expandido (lado derecho, read-only, oculto) ───────
    editorExpandido = new CodeEditor(this);
    editorExpandido->setReadOnly(true);
    editorExpandido->setFont(fe);
    editorExpandido->setStyleSheet(
        QString("QTextEdit { background:%1; color:%2; border:none;"
                "border-left:2px solid %3; }")
        .arg(Theme::BG_EDITOR, Theme::TXT_DIM, Theme::ACCENT_BORDER)
    );
    editorExpandido->setPlaceholderText("El código expandido aparecerá aquí al compilar...");
    editorExpandido->hide();
    _hlExpandido = new SyntaxHighlighter(editorExpandido->document());

    // ── Botones ───────────────────────────────────────────────────
    botonEjecutar  = makeBtn("▶  Compilar",  Theme::ACCENT_BLUE,  Theme::H_BLUE,  this);
    botonAnterior  = makeBtn("◀  Anterior",  Theme::ACCENT_GRAY,  Theme::H_GRAY,  this);
    botonSiguiente = makeBtn("Siguiente  ▶", Theme::ACCENT_GREEN, Theme::H_GREEN, this);
    botonPlay      = makeBtn("▶ Play",       Theme::ACCENT_GREEN, Theme::H_GREEN, this);
    botonStop      = makeBtn("■ Stop",       Theme::ACCENT_GRAY,  Theme::H_GRAY,  this);
    botonPlay->setCheckable(true);

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
    comboPlantillas->addItem("Hacer");
    comboPlantillas->addItem("Elegir");
    comboPlantillas->addItem("Ternario");
    comboPlantillas->addItem("Sin init");

    botonSiguiente->setEnabled(false);
    botonAnterior->setEnabled(false);
    botonPlay->setEnabled(false);
    botonStop->setEnabled(false);

    // ── Speed slider ──────────────────────────────────────────────
    sliderVelocidad = new QSlider(Qt::Horizontal, this);
    sliderVelocidad->setRange(1, 10);
    sliderVelocidad->setValue(3);
    sliderVelocidad->setFixedWidth(100);
    sliderVelocidad->setStyleSheet(
        QString("QSlider::groove:horizontal { height:4px; background:%1; border-radius:2px; }"
        "QSlider::handle:horizontal { background:%2; width:14px; height:14px;"
        "  margin:-5px 0; border-radius:7px; }"
        "QSlider::sub-page:horizontal { background:%2; border-radius:2px; }")
        .arg(Theme::BORDER_DIM, Theme::ACCENT_CYAN));
    sliderVelocidad->setEnabled(false);

    etiquetaVelocidad = new QLabel("lento", this);
    etiquetaVelocidad->setStyleSheet(
        QString("QLabel { color:%1; font-size:10px; padding:0 4px; }").arg(Theme::TXT_DIM));

    // ── Timer para auto-play ──────────────────────────────────────
    timerAutoPlay = new QTimer(this);
    timerAutoPlay->setSingleShot(false);

    etiquetaPaso = new QLabel("—", this);
    etiquetaPaso->setStyleSheet(
        QString("QLabel { color:%1; font-size:11px; padding:0 8px; }").arg(Theme::TXT_DIM));

    // ── Panel derecho de animación ────────────────────────────────
    panelAnimacion = new QWidget(this);
    panelAnimacion->setStyleSheet(
        QString("QWidget { background:%1; }").arg(Theme::BG_PANEL));

    QVBoxLayout* layPanel = new QVBoxLayout(panelAnimacion);
    layPanel->setContentsMargins(12, 12, 12, 12);
    layPanel->setSpacing(8);

    // — Sección variables —
    tituloVariables = new QLabel("📦  Variables en memoria", panelAnimacion);
    tituloVariables->setStyleSheet(
        QString("QLabel { color:%1; font-weight:700; font-size:12px; background:transparent; }")
        .arg(Theme::ACCENT_PURPLE));
    layPanel->addWidget(tituloVariables);

    QScrollArea* scVars = new QScrollArea(panelAnimacion);
    scVars->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scVars->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scVars->setFixedHeight(98);
    scVars->setStyleSheet(QString("QScrollArea { border:1px solid %1; border-radius:4px;")
                          .arg(Theme::BORDER_PANEL) +
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
    sep->setStyleSheet(QString("color:%1;").arg(Theme::BORDER_PANEL));
    layPanel->addWidget(sep);

    // — Sección arreglo —
    tituloArreglo = new QLabel("🗂  Arreglo", panelAnimacion);
    tituloArreglo->setStyleSheet(
        QString("QLabel { color:%1; font-weight:700; font-size:12px; background:transparent; }")
        .arg(Theme::ACCENT_AMBER));
    tituloArreglo->setVisible(false);
    layPanel->addWidget(tituloArreglo);

    QScrollArea* scArr = new QScrollArea(panelAnimacion);
    scArr->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scArr->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scArr->setFixedHeight(98);
    scArr->setStyleSheet(QString("QScrollArea { border:1px solid %1; border-radius:4px;")
                         .arg(Theme::BORDER_PANEL) +
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
        QString("QLabel { color:%1; background:%2; border:1px solid %3;"
        "border-radius:4px; padding:5px 8px; font-size:11px; }")
        .arg(Theme::TXT_SUCCESS, Theme::BG_SUCCESS, Theme::BORDER_SUCCESS));
    layPanel->addWidget(etiquetaEstado);

    layPanel->addStretch();

    // ── Widget central ────────────────────────────────────────────
    QWidget* central = new QWidget(this);
    setCentralWidget(central);

    QVBoxLayout* layMain = new QVBoxLayout(central);
    layMain->setContentsMargins(6, 4, 6, 4);
    layMain->setSpacing(4);

    // — Barra de herramientas (toolbar row) —
    _toolbar = new QWidget(this);
    _toolbar->setFixedHeight(36);
    _toolbar->setStyleSheet(
        QString("QWidget { background:%1; border-bottom:1px solid %2; border-radius:0; }")
        .arg(Theme::BG_TOOLBAR, Theme::BORDER_SUBTLE));
    QHBoxLayout* layToolbar = new QHBoxLayout(_toolbar);
    layToolbar->setContentsMargins(8, 4, 8, 4);
    layToolbar->setSpacing(6);
    
    layToolbar->addWidget(botonEjecutar);
    layToolbar->addSpacing(10);
    layToolbar->addWidget(comboPlantillas);
    layToolbar->addSpacing(10);

    // separador visual entre compilar y navegación
    QFrame* sepBtn = new QFrame(_toolbar);
    sepBtn->setFrameShape(QFrame::VLine);
    sepBtn->setStyleSheet(QString("QFrame { color:%1; }").arg(Theme::BORDER_DIM));
    layToolbar->addWidget(sepBtn);
    layToolbar->addSpacing(4);

    layToolbar->addWidget(botonAnterior);
    layToolbar->addWidget(botonSiguiente);
    layToolbar->addSpacing(4);
    layToolbar->addWidget(botonPlay);
    layToolbar->addWidget(botonStop);
    layToolbar->addSpacing(6);
    layToolbar->addWidget(etiquetaPaso);
    layToolbar->addSpacing(8);
    layToolbar->addWidget(sliderVelocidad);
    layToolbar->addWidget(etiquetaVelocidad);
    layToolbar->addStretch();
    layMain->addWidget(_toolbar);

    // — Splitter horizontal: izquierda (editor+consola) / derecha (panel) —
    QSplitter* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setHandleWidth(4);
    splitter->setStyleSheet(
        QString("QSplitter::handle { background:%1; }").arg(Theme::BORDER_SUBTLE));

    // lado izquierdo: splitter con editor original + expandido
    QWidget* ladoIzq = new QWidget();
    ladoIzq->setStyleSheet("background:transparent;");
    QVBoxLayout* layIzq = new QVBoxLayout(ladoIzq);
    layIzq->setContentsMargins(0,0,0,0);
    layIzq->setSpacing(0);

    splitterEditores = new QSplitter(Qt::Horizontal, ladoIzq);
    splitterEditores->setHandleWidth(4);
    splitterEditores->setStyleSheet(
        QString("QSplitter::handle { background:%1; }").arg(Theme::BORDER_SUBTLE));
    splitterEditores->addWidget(editorCodigo);
    splitterEditores->addWidget(editorExpandido);
    splitterEditores->setSizes({680, 0});
    layIzq->addWidget(splitterEditores, 1);

    splitter->addWidget(ladoIzq);
    splitter->addWidget(panelAnimacion);
    splitter->setSizes({680, 520});

    layMain->addWidget(splitter, 1);
    
    // ── Conexiones ────────────────────────────────────────────────
    connect(botonEjecutar,  &QPushButton::clicked, this, &VentanaPrincipal::manejarEjecucion);
    connect(botonSiguiente, &QPushButton::clicked, this, &VentanaPrincipal::avanzarPaso);
    connect(botonAnterior,  &QPushButton::clicked, this, &VentanaPrincipal::retrocederPaso);
    connect(botonPlay,      &QPushButton::clicked, this, &VentanaPrincipal::playPause);
    connect(botonStop,      &QPushButton::clicked, this, &VentanaPrincipal::stopPlay);
    connect(timerAutoPlay,  &QTimer::timeout,      this, &VentanaPrincipal::avanzarAuto);
    connect(sliderVelocidad, &QSlider::valueChanged, this, [this](int v) {
        int ms[] = {800, 500, 350, 250, 180, 120, 80, 50, 30, 15};
        timerAutoPlay->setInterval(ms[qBound(0, v-1, 9)]);
        etiquetaVelocidad->setText(v <= 3 ? "lento" : v <= 7 ? "medio" : "rápido");
    });
    connect(comboPlantillas,
        &QComboBox::currentTextChanged,
        this,
        &VentanaPrincipal::cargarPlantilla);
}

// ═════════════════════════════════════════════════════════════════
//  reestilarTodo — re-aplica todos los estilos visuales
// ═════════════════════════════════════════════════════════════════
void VentanaPrincipal::reestilarTodo() {
    // Ventana principal
    setStyleSheet(QString("QMainWindow { background:%1; }").arg(Theme::BG_APP));

    // Barra de menú
    menuBar()->setStyleSheet(
        QString("QMenuBar { background:%1; color:%2; font-size:12px;"
                "           border-bottom:1px solid %3; padding:1px 4px; }"
                "QMenuBar::item { padding:3px 10px; border-radius:3px; }"
                "QMenuBar::item:selected { background:%3; }"
                "QMenu { background:%4; color:%2; border:1px solid %5; }"
                "QMenu::item:selected { background:%6; }")
        .arg(Theme::BG_MENUBAR, Theme::TXT_MAIN,
             Theme::BORDER_MUTED, Theme::BG_MENU,
             Theme::BORDER_DIM, Theme::ACCENT_BORDER)
    );

    // Barra de estado
    statusBar()->setStyleSheet(
        QString("QStatusBar { background:%1; color:%2; font-size:11px; "
                "border-top:1px solid %3; }")
        .arg(Theme::BG_MENUBAR, Theme::TXT_DIM, Theme::BORDER_SUBTLE)
    );

    // Editores
    editorCodigo->setStyleSheet(
        QString("QTextEdit { background:%1; color:%2; border:none; }")
        .arg(Theme::BG_EDITOR, Theme::TXT_MAIN)
    );
    editorExpandido->setStyleSheet(
        QString("QTextEdit { background:%1; color:%2; border:none;"
                "border-left:2px solid %3; }")
        .arg(Theme::BG_EDITOR, Theme::TXT_DIM, Theme::ACCENT_BORDER)
    );

    // Toolbar
    if (_toolbar) {
        _toolbar->setStyleSheet(
            QString("QWidget { background:%1; border-bottom:1px solid %2; border-radius:0; }")
            .arg(Theme::BG_TOOLBAR, Theme::BORDER_SUBTLE));
    }

    // Panel derecho
    panelAnimacion->setStyleSheet(
        QString("QWidget { background:%1; }").arg(Theme::BG_PANEL));

    // Speed slider
    sliderVelocidad->setStyleSheet(
        QString("QSlider::groove:horizontal { height:4px; background:%1; border-radius:2px; }"
                "QSlider::handle:horizontal { background:%2; width:14px; height:14px;"
                "  margin:-5px 0; border-radius:7px; }"
                "QSlider::sub-page:horizontal { background:%2; border-radius:2px; }")
        .arg(Theme::BORDER_DIM, Theme::ACCENT_CYAN));

    // Etiquetas
    etiquetaVelocidad->setStyleSheet(
        QString("QLabel { color:%1; font-size:10px; padding:0 4px; }").arg(Theme::TXT_DIM));
    etiquetaPaso->setStyleSheet(
        QString("QLabel { color:%1; font-size:11px; padding:0 8px; }").arg(Theme::TXT_DIM));

    // Títulos de sección
    tituloVariables->setStyleSheet(
        QString("QLabel { color:%1; font-weight:700; font-size:12px; background:transparent; }")
        .arg(Theme::ACCENT_PURPLE));
    tituloArreglo->setStyleSheet(
        QString("QLabel { color:%1; font-weight:700; font-size:12px; background:transparent; }")
        .arg(Theme::ACCENT_AMBER));

    // Etiqueta de estado (solo si hay mensaje activo)
    if (!estadoActual.mensajeEstado.empty())
        setMensajeEstado(estadoActual.mensajeEstado, estadoActual.mensajeError);
    else
        etiquetaEstado->setVisible(false);

    // Splitter handles
    auto splitters = findChildren<QSplitter*>();
    for (auto* s : splitters) {
        s->setStyleSheet(
            QString("QSplitter::handle { background:%1; }").arg(Theme::BORDER_SUBTLE));
    }

    // Recrear syntax highlighters para que usen los colores del tema actual
    delete _hlCodigo;
    delete _hlExpandido;
    _hlCodigo    = new SyntaxHighlighter(editorCodigo->document());
    _hlExpandido = new SyntaxHighlighter(editorExpandido->document());
}

// ═════════════════════════════════════════════════════════════════
//  _construirVistaSoloFunciones — extrae solo las funciones de
//  biblioteca que son invocadas en el código del usuario
// ═════════════════════════════════════════════════════════════════
void VentanaPrincipal::_construirVistaSoloFunciones(
    const std::string& expanded,
    const std::vector<int>& mapaLineas)
{
    _vistaSoloFunciones.clear();
    _mapaLineasLibreria.assign(mapaLineas.size(), -1);

    auto lines = _splitLines(expanded);
    if (lines.empty()) return;

    // 1. Encontrar todas las definiciones de función en código de biblioteca
    std::regex fnDefRe(R"(^\s*(entero|decimal|cadena|booleano|caracter|vacio|funcion)\s+(\w+)\s*\()");
    std::vector<_FuncInfo> funciones;
    int numLines = (int)lines.size();

    for (int i = 0; i < numLines; i++) {
        int expLine = i + 1; // 1-indexed
        if ((size_t)expLine >= mapaLineas.size()) break;
        if (mapaLineas[expLine] != 0) continue; // solo líneas de librería

        std::smatch m;
        if (std::regex_search(lines[i], m, fnDefRe)) {
            _FuncInfo fn;
            fn.nombre = m[2].str();
            fn.lineaInicio = expLine;
            fn.cuerpo = _extraerFuncion(lines, expLine, fn.lineaFin);
            if (!fn.cuerpo.empty())
                funciones.push_back(fn);
        }
    }

    if (funciones.empty()) return;

    // 2. Construir un set de nombres de función para búsqueda rápida
    std::set<std::string> nombresLib;
    for (auto& fn : funciones)
        nombresLib.insert(fn.nombre);

    // 3. Marcar funciones llamadas desde código de usuario
    std::regex callRe;
    for (int i = 0; i < numLines; i++) {
        int expLine = i + 1;
        if ((size_t)expLine >= mapaLineas.size()) break;
        if (mapaLineas[expLine] == 0) continue; // solo líneas de usuario

        for (auto& fn : funciones) {
            if (fn.usada) continue;
            callRe = std::regex("\\b" + fn.nombre + "\\s*\\(");
            if (std::regex_search(lines[i], callRe))
                fn.usada = true;
        }
    }

    // 4. Propagación transitiva: funciones de lib que llaman a otras funciones de lib
    bool changed = true;
    while (changed) {
        changed = false;
        for (auto& fn : funciones) {
            if (!fn.usada) continue;
            for (auto& fn2 : funciones) {
                if (fn2.usada) continue;
                callRe = std::regex("\\b" + fn2.nombre + "\\s*\\(");
                if (std::regex_search(fn.cuerpo, callRe)) {
                    fn2.usada = true;
                    changed = true;
                }
            }
        }
    }

    // 5. Construir vista solo-funciones y mapa de líneas
    for (auto& fn : funciones) {
        if (!fn.usada) continue;

        int soloLineStart = (int)std::count(
            _vistaSoloFunciones.begin(), _vistaSoloFunciones.end(), '\n') + 1;

        _vistaSoloFunciones += fn.cuerpo;
        if (!_vistaSoloFunciones.empty() && _vistaSoloFunciones.back() != '\n')
            _vistaSoloFunciones += '\n';

        // Mapear cada línea expandida → línea en la vista solo-funciones
        int soloLine = soloLineStart;
        for (int el = fn.lineaInicio; el <= fn.lineaFin; el++) {
            if ((size_t)el < _mapaLineasLibreria.size())
                _mapaLineasLibreria[el] = soloLine++;
        }
    }
}

// ═════════════════════════════════════════════════════════════════
//  toggleTheme — alterna entre tema oscuro y claro
// ═════════════════════════════════════════════════════════════════
void VentanaPrincipal::toggleTheme() {
    _temaOscuro = !_temaOscuro;
    if (_temaOscuro)
        Theme::setDark();
    else
        Theme::setLight();
    reestilarTodo();
}

// ═════════════════════════════════════════════════════════════════
//  Fuente
// ═════════════════════════════════════════════════════════════════
void VentanaPrincipal::aumentarFuente() {
    QFont f = editorCodigo->font();
    if (f.pointSize() < 28) {
        f.setPointSize(f.pointSize() + 1);
        editorCodigo->setFont(f);
        editorExpandido->setFont(f);
    }
}

void VentanaPrincipal::reducirFuente() {
    QFont f = editorCodigo->font();
    if (f.pointSize() > 8) {
        f.setPointSize(f.pointSize() - 1);
        editorCodigo->setFont(f);
        editorExpandido->setFont(f);
    }
}

// ═════════════════════════════════════════════════════════════════
//  Acerca de
// ═════════════════════════════════════════════════════════════════
void VentanaPrincipal::acercaDe() {
    QMessageBox::about(this, "Acerca de MyCompiler",
        "<h2>MyCompiler</h2>"
        "<p>Compilador e intérprete educativo con sintaxis en español.</p>"
        "<p>Versión 1.0</p>"
        "<hr>"
        "<p>Creado por <b>Jose Chullo</b> y <b>Rodrigo Ramos</b></p>"
        "<p>Licencia MIT</p>"
    );
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
        QString("QTextEdit {"
        "  background:%1; color:%2;"
        "  border:none; padding:12px;"
        "  selection-background-color:%3;"
        "}"
        "QScrollBar:vertical {"
        "  background:%1; width:10px;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background:%4; border-radius:5px; min-height:20px;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "  height:0;"
        "}")
        .arg(Theme::BG_TERM, Theme::TXT_TERM, Theme::TERM_SEL, Theme::TERM_SCROLL)
    );
    output->setHtml(
        QString("<pre style='font-family:\"Courier New\",monospace;"
                "margin:0; white-space:pre-wrap; color:%1;'>%2</pre>")
        .arg(Theme::TXT_TERM, QString::fromStdString(contenido))
    );

    auto* barra = new QWidget(term);
    barra->setFixedHeight(36);
    barra->setStyleSheet(
        QString("background:%1; border-top:1px solid %2;")
        .arg(Theme::BG_TERM_BAR, Theme::TERM_SCROLL));
    auto* layBarra = new QHBoxLayout(barra);
    layBarra->setContentsMargins(8, 4, 8, 4);

    auto* btnCerrar = new QPushButton("Cerrar", barra);
    btnCerrar->setFixedHeight(26);
    btnCerrar->setCursor(Qt::PointingHandCursor);
    btnCerrar->setStyleSheet(
        QString("QPushButton { background:%1; color:#fff; font-weight:600; font-size:12px;"
        "  border:none; border-radius:4px; padding:0 18px;"
        "}"
        "QPushButton:hover { background:%2; }")
        .arg(Theme::TERM_BTN, Theme::TERM_BTN_HOVER)
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
                "\n#incluir \"matematica.txt\"       // librería básica\n"
                "// #incluir \"matematica_avanzada.txt\"  // + funciones\n"
                "// #incluir \"texto.txt\"                // + cadenas\n";
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
                "fin_si\n"
                "si(!(a == b)){\n"
                "    mostrar(\"son diferentes\");\n"
                "}\n"
                "fin_si\n";
        }

        else if(nombre == "Hacer")
        {
            codigo =
                "\nentero i = 0;\n"
                "hacer {\n"
                "    mostrar(i);\n"
                "    i++;\n"
                "} mientras (i < 3) fin_mientras\n";
        }

        else if(nombre == "Elegir")
        {
            codigo =
                "\nentero x = 2;\n"
                "elegir (x) {\n"
                "    caso 1: mostrar(\"uno\"); parar;\n"
                "    caso 2: mostrar(\"dos\"); parar;\n"
                "    defecto: mostrar(\"otro\");\n"
                "}\n";
        }

        else if(nombre == "Ternario")
        {
            codigo =
                "\nentero edad = 15;\n"
                "cadena msg = si (edad >= 18) entonces \"Mayor\" sino \"Menor\";\n"
                "mostrar(msg);\n";
        }

        else if(nombre == "Sin init")
        {
            codigo =
                "\nentero a;\n"
                "decimal b;\n"
                "cadena c;\n"
                "booleano d;\n"
                "caracter e;\n"
                "a = 10;\n"
                "mostrar(a);\n";
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
        std::string expanded;
        mapaLineas.clear();
        preprocesarBibliotecas(src, expanded, mapaLineas);
        // Mostrar código expandido en el panel derecho solo si hay #incluir
        bool hayInclude = (src.find("#incluir") != std::string::npos);
        if (hayInclude) {
            if (!editorExpandido->isVisible()) {
                editorExpandido->show();
                splitterEditores->setSizes({480, 480});
            }
            // Construir vista solo con las funciones de biblioteca invocadas
            _construirVistaSoloFunciones(expanded, mapaLineas);
            editorExpandido->setPlainText(
                _vistaSoloFunciones.empty()
                ? QString::fromStdString(expanded)
                : QString::fromStdString(_vistaSoloFunciones));
        } else {
            if (editorExpandido->isVisible()) {
                editorExpandido->hide();
                splitterEditores->setSizes({680, 0});
            }
        }
        auto tokens = tokenizar(expanded);
        parsear(tokens, &pasos);
        setInputHook(nullptr);
        std::cout.rdbuf(old);

        // Colapsar eventos consecutivos de la misma línea en un meta-paso
        if (!pasos.empty()) {
            std::vector<EventoPaso> agrupados;
            for (auto& ev : pasos) {
                if (agrupados.empty() || agrupados.back().linea != ev.linea)
                    agrupados.push_back(ev);
                else
                    agrupados.back() = ev;
            }
            pasos = std::move(agrupados);
        }

        QString salida = QString::fromStdString(buf.str()).toHtmlEscaped()
                            .replace("\n","<br>");
        html += salida;

        html += QString("<br><span style='color:%1;'>✓ %2 pasos generados."
                        " Presiona Siguiente para animar.</span>")
                .arg(Theme::GUTTER_ARROW)
                .arg(pasos.size());
        statusBar()->showMessage(QString("Compilado — %1 pasos").arg(pasos.size()));

    } catch (const std::exception& e) {
        setInputHook(nullptr);
        std::cout.rdbuf(old);
        // En error, ocultar panel expandido solo si no hay #incluir en el código
        bool hayInclude = (src.find("#incluir") != std::string::npos);
        if (!hayInclude && editorExpandido->isVisible()) {
            editorExpandido->hide();
            splitterEditores->setSizes({680, 0});
        }
        QString salida = QString::fromStdString(buf.str()).toHtmlEscaped()
                            .replace("\n","<br>");
        if (!salida.isEmpty())
            html += salida;
        html += QString("<br><span style='color:%1;'>%2</span>").arg(Theme::TXT_ERROR)
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
//  Auto-play
// ═════════════════════════════════════════════════════════════════
void VentanaPrincipal::playPause() {
    if (timerAutoPlay->isActive()) {
        timerAutoPlay->stop();
        botonPlay->setText("▶ Play");
        botonPlay->setChecked(false);
    } else {
        if (pasoActual >= (int)pasos.size()) {
            stopPlay();
            return;
        }
        botonPlay->setText("⏸ Pausa");
        botonPlay->setChecked(true);
        timerAutoPlay->start();
    }
}

void VentanaPrincipal::stopPlay() {
    timerAutoPlay->stop();
    botonPlay->setText("▶ Play");
    botonPlay->setChecked(false);
    if (editorExpandido->isVisible()) {
        editorExpandido->hide();
        splitterEditores->setSizes({680, 0});
    }
    // Reset
    historial.clear();
    pasoActual = 0;
    estadoActual = SnapshotUI();
    limpiarTarjetas();
    limpiarArreglo();
    etiquetaEstado->setVisible(false);
    resaltarLinea(-1);
    actualizarBotones();
}

void VentanaPrincipal::avanzarAuto() {
    if (pasoActual >= (int)pasos.size()) {
        timerAutoPlay->stop();
        botonPlay->setText("▶ Play");
        botonPlay->setChecked(false);
        return;
    }
    avanzarPaso();
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
    
    case TipoEvento::ELEGIR_CASO: {
        resaltarLinea(ev.linea);
        estadoActual.lineaActiva = ev.linea;
        std::string msg = "elegir: coincidió " +
            (ev.valor == "defecto" ? std::string("defecto") :
             std::string("caso ") + ev.valor);
        estadoActual.mensajeEstado = msg;
        estadoActual.mensajeError  = false;
        setMensajeEstado(msg, false);
        break;
    }

    case TipoEvento::ROMPER:
        resaltarLinea(ev.linea);
        estadoActual.mensajeEstado = "parar: saliendo del bloque";
        estadoActual.mensajeError  = false;
        setMensajeEstado(estadoActual.mensajeEstado, false);
        break;

    case TipoEvento::CONTINUAR:
        resaltarLinea(ev.linea);
        estadoActual.mensajeEstado = "continuar: saltando a siguiente iteración";
        estadoActual.mensajeError  = false;
        setMensajeEstado(estadoActual.mensajeEstado, false);
        break;

    case TipoEvento::PUNTERO_MODIFICADO:
    case TipoEvento::PUNTERO_DESREFERENCIADO:
        break; // Se implementará el diseño visual después con Jesus y Estrella
    
    
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
//  _resaltarEditor — helper para un solo editor
// ═════════════════════════════════════════════════════════════════
static void _resaltarEditor(CodeEditor* ed, int linea, bool scroll = true) {
    QList<QTextEdit::ExtraSelection> sels;
    if (linea > 0) {
        QTextEdit::ExtraSelection sel;
        sel.format.setBackground(QColor(Theme::LINE_HIGHLIGHT));
        sel.format.setProperty(QTextFormat::FullWidthSelection, true);

        QTextCursor cur(ed->document());
        cur.movePosition(QTextCursor::Start);
        for (int i = 1; i < linea; ++i)
            cur.movePosition(QTextCursor::NextBlock);
        cur.movePosition(QTextCursor::StartOfBlock);
        sel.cursor = cur;
        sels.append(sel);

        if (scroll) {
            ed->setTextCursor(cur);
            ed->ensureCursorVisible();
        }
    }
    ed->setExtraSelections(sels);
    ed->setLineaActiva(linea);
}

// ═════════════════════════════════════════════════════════════════
//  resaltarLinea  — sincroniza ambos paneles según el mapa
// ═════════════════════════════════════════════════════════════════
void VentanaPrincipal::resaltarLinea(int lineaExpandida) {
    if (lineaExpandida <= 0) {
        _resaltarEditor(editorCodigo, -1);
        if (editorExpandido->isVisible())
            _resaltarEditor(editorExpandido, -1);
        _ultimaLineaExpandida = -1;
        return;
    }

    _ultimaLineaExpandida = lineaExpandida;

    bool esLibreria = ((size_t)lineaExpandida < mapaLineas.size())
                      && mapaLineas[lineaExpandida] == 0;

    if (esLibreria && !_vistaSoloFunciones.empty()) {
        // Línea de biblioteca → flecha en panel derecho (vista solo-funciones)
        _resaltarEditor(editorCodigo, -1);

        int soloLinea = ((size_t)lineaExpandida < _mapaLineasLibreria.size())
                        ? _mapaLineasLibreria[lineaExpandida] : -1;
        if (editorExpandido->isVisible() && soloLinea > 0)
            _resaltarEditor(editorExpandido, soloLinea, false);
        else if (editorExpandido->isVisible())
            _resaltarEditor(editorExpandido, -1);
    } else {
        // Línea de usuario → flecha en panel izquierdo (editor original)
        int lineaOrig = ((size_t)lineaExpandida < mapaLineas.size())
                        ? mapaLineas[lineaExpandida] : -1;
        if (lineaOrig > 0)
            _resaltarEditor(editorCodigo, lineaOrig, true);
        else
            _resaltarEditor(editorCodigo, -1);

        // Panel derecho: sin flecha (solo muestra las funciones)
        if (editorExpandido->isVisible())
            _resaltarEditor(editorExpandido, -1);
    }
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

    QString colorBorde = esNueva ? Theme::CARD_BORDER_NEW : Theme::CARD_BORDER;
    QString estilo =
        QString("QLabel { background:%1; color:%2;"
                "border:1px solid %3; border-radius:6px;"
                "padding:6px 10px; font-family:'Courier New'; font-size:12px;"
                "min-width:60px; }").arg(Theme::BG_CARD, Theme::TXT_CARD, colorBorde);

    QString txt = QString("<div style='color:%1;font-weight:700;'>%2</div>"
                          "<div style='color:%3;'>%4</div>")
                  .arg(colorBorde, qNombre, Theme::TXT_CARD_VAL, qValor);

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
            arr->setStyleSheet(
                QString("color:%1; font-size:11px; background:transparent;")
                .arg(Theme::ACCENT_AMBER));
            lc->addWidget(arr);
        }

        QLabel* val = new QLabel(QString::fromStdString(celdas[i]), celda);
        val->setAlignment(Qt::AlignCenter);
        val->setStyleSheet(
            QString("QLabel { background:%1; color:%2; border:1px solid %3;"
                    "border-radius:4px; font-family:'Courier New';"
                    "font-size:13px; font-weight:700; min-height:38px; }")
            .arg(activa ? Theme::BG_ARR_ACT : Theme::BG_CARD)
            .arg(activa ? Theme::ACCENT_AMBER : Theme::TXT_ARR)
            .arg(activa ? Theme::ACCENT_AMBER : Theme::BORDER_LINE)
        );

        QLabel* idx = new QLabel(QString("[%1]").arg(i), celda);
        idx->setAlignment(Qt::AlignCenter);
        idx->setStyleSheet(
            QString("QLabel { color:%1; font-size:10px; background:transparent; }")
            .arg(activa ? Theme::ACCENT_AMBER : Theme::TXT_ARR_IDX));

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
            QString("QLabel { color:%1; background:%2; border:1px solid %3;"
            "border-radius:4px; padding:5px 8px; font-size:11px; }")
            .arg(Theme::TXT_ERROR, Theme::BG_ERROR, Theme::BORDER_ERROR));
    } else {
        etiquetaEstado->setStyleSheet(
            QString("QLabel { color:%1; background:%2; border:1px solid %3;"
            "border-radius:4px; padding:5px 8px; font-size:11px; }")
            .arg(Theme::TXT_SUCCESS, Theme::BG_SUCCESS, Theme::BORDER_SUCCESS));
    }
    etiquetaEstado->setVisible(true);
}

// ═════════════════════════════════════════════════════════════════
//  Botones y etiqueta de paso
// ═════════════════════════════════════════════════════════════════
void VentanaPrincipal::actualizarBotones() {
    bool hay = !pasos.empty();
    bool finalizado = hay && pasoActual >= (int)pasos.size();
    botonSiguiente->setEnabled(hay && !finalizado);
    botonAnterior->setEnabled(!historial.empty() && !timerAutoPlay->isActive());
    botonPlay->setEnabled(hay);
    botonStop->setEnabled(hay && (pasoActual > 0 || timerAutoPlay->isActive()));
    sliderVelocidad->setEnabled(hay);
    etiquetaPaso->setText(
        hay ? QString("Paso %1 / %2").arg(pasoActual).arg(pasos.size()) : "—");
}

