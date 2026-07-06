#pragma once
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>
#include <QVector>
#include <QStringList>

class SyntaxHighlighter : public QSyntaxHighlighter {
public:
    explicit SyntaxHighlighter(QTextDocument* parent)
        : QSyntaxHighlighter(parent)
    {
        // ── Paleta de colores (match con tema oscuro) ──────────────
        _fmtKeyword.setForeground(QColor("#569cd6"));
        _fmtKeyword.setFontWeight(QFont::Bold);

        _fmtString.setForeground(QColor("#ce9178"));

        _fmtChar.setForeground(QColor("#ce9178"));

        _fmtNumber.setForeground(QColor("#b5cea8"));

        _fmtComment.setForeground(QColor("#6a9955"));

        _fmtPreproc.setForeground(QColor("#c586c0"));

        // ── Palabras clave ─────────────────────────────────────────
        QStringList kws = {
            "entero", "decimal", "cadena", "booleano", "caracter",
            "vacio", "funcion", "retornar", "arreglo",
            "si", "sino", "fin_si", "entonces",
            "hacer", "mientras", "fin_mientras",
            "para", "fin_para",
            "elegir", "caso", "defecto", "parar",
            "mostrar", "leer", "romper", "continuar", "no",
            "Principal", "verdadero", "falso", "incluir"
        };
        for (const auto& kw : kws) {
            Rule r;
            r.pattern = QRegularExpression(
                "\\b" + QRegularExpression::escape(kw) + "\\b");
            r.format  = _fmtKeyword;
            _rules.append(r);
        }

        // ── Números ────────────────────────────────────────────────
        _rules.append({
            QRegularExpression("\\b[0-9]+(\\.[0-9]+)?\\b"),
            _fmtNumber
        });

        // ── Comentario de línea ────────────────────────────────────
        _rules.append({
            QRegularExpression("//[^\n]*"),
            _fmtComment
        });

        // ── #incluir / directivas ──────────────────────────────────
        _rules.append({
            QRegularExpression("#\\w+"),
            _fmtPreproc
        });
    }

protected:
    void highlightBlock(const QString& text) override {
        // 1. Aplicar reglas simples (keywords, números, comentarios, #directivas)
        for (const auto& rule : _rules) {
            auto it = rule.pattern.globalMatch(text);
            while (it.hasNext()) {
                auto match = it.next();
                setFormat(match.capturedStart(), match.capturedLength(), rule.format);
            }
        }

        // 2. Strings "..." (multi-línea con estado)
        _aplicarString(text, '"', _fmtString);

        // 3. Caracteres '...' (una línea)
        _aplicarChar(text);
    }

private:
    struct Rule {
        QRegularExpression pattern;
        QTextCharFormat    format;
    };
    QVector<Rule> _rules;

    QTextCharFormat _fmtKeyword;
    QTextCharFormat _fmtString;
    QTextCharFormat _fmtChar;
    QTextCharFormat _fmtNumber;
    QTextCharFormat _fmtComment;
    QTextCharFormat _fmtPreproc;

    // ── String multi-línea con estado ────────────────────────────
    void _aplicarString(const QString& text, QChar delim,
                        const QTextCharFormat& fmt)
    {
        int state = previousBlockState();
        if (state < 0 || state > 1) state = 0;

        int i = 0;
        if (state == 1) {
            // Viene de bloque anterior — buscamos el cierre
            int end = _findDelimEnd(text, i, delim);
            if (end >= 0) {
                setFormat(0, end + 1, fmt);
                i = end + 1;
                setCurrentBlockState(0);
            } else {
                setFormat(0, text.length(), fmt);
                setCurrentBlockState(1);
                return;
            }
        }

        while (i < text.length()) {
            // Buscar apertura
            int start = text.indexOf(delim, i);
            if (start < 0) break;

            int end = _findDelimEnd(text, start + 1, delim);
            if (end >= 0) {
                setFormat(start, end - start + 1, fmt);
                i = end + 1;
            } else {
                // String sin cerrar — se extiende al resto de la línea
                setFormat(start, text.length() - start, fmt);
                setCurrentBlockState(1);
                return;
            }
        }
        setCurrentBlockState(0);
    }

    // ── Carácter '...' (una línea siempre) ───────────────────────
    void _aplicarChar(const QString& text) {
        QRegularExpression re("'[^'\\\\]*(\\\\.[^'\\\\]*)*'");
        auto it = re.globalMatch(text);
        while (it.hasNext()) {
            auto match = it.next();
            setFormat(match.capturedStart(), match.capturedLength(), _fmtChar);
        }
    }

    // ── Buscar fin de string saltando \\secuencias ───────────────
    int _findDelimEnd(const QString& text, int start, QChar delim) const {
        int i = start;
        while (i < text.length()) {
            if (text[i] == '\\' && i + 1 < text.length()) {
                i += 2; // saltar escape
                continue;
            }
            if (text[i] == delim) return i;
            i++;
        }
        return -1; // no encontrado
    }
};
