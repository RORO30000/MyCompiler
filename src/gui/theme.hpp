#pragma once
#include <QString>

// ═════════════════════════════════════════════════════════════════
//  Tema centralizado — todos los colores de la GUI en un solo lugar
//  inline → modificable en runtime para toggle claro/oscuro
// ═════════════════════════════════════════════════════════════════
namespace Theme {

    // ── Fondos ────────────────────────────────────────────────────
    inline QString BG_APP       = "#1e1e1e";
    inline QString BG_EDITOR    = "#1e1e1e";
    inline QString BG_PANEL     = "#252526";
    inline QString BG_GUTTER    = "#2d2d30";
    inline QString BG_MENUBAR   = "#2d2d30";
    inline QString BG_TOOLBAR   = "#252526";
    inline QString BG_CARD      = "#0f172a";
    inline QString BG_TERM      = "#0d1117";
    inline QString BG_TERM_BAR  = "#161b22";
    inline QString BG_SUCCESS   = "#052e16";
    inline QString BG_ERROR     = "#3b0000";
    inline QString BG_DISABLED  = "#2d2d2d";
    inline QString BG_MENU      = "#2d2d2d";
    inline QString BG_ARR_ACT   = "#2d1d00";

    // ── Texto ─────────────────────────────────────────────────────
    inline QString TXT_MAIN     = "#d4d4d4";
    inline QString TXT_DIM      = "#9ca3af";
    inline QString TXT_BTN      = "#f0f0f0";
    inline QString TXT_DISABLED = "#555";
    inline QString TXT_TERM     = "#e6edf3";
    inline QString TXT_CARD     = "#e2e8f0";
    inline QString TXT_CARD_VAL = "#f8fafc";
    inline QString TXT_SUCCESS  = "#86efac";
    inline QString TXT_ERROR    = "#fca5a5";
    inline QString TXT_ARR      = "#93c5fd";
    inline QString TXT_ARR_IDX  = "#4b5563";

    // ── Acentos ───────────────────────────────────────────────────
    inline QString ACCENT_BLUE   = "#007acc";
    inline QString ACCENT_GREEN  = "#16a34a";
    inline QString ACCENT_GRAY   = "#4b5563";
    inline QString ACCENT_PURPLE = "#818cf8";
    inline QString ACCENT_CYAN   = "#38bdf8";
    inline QString ACCENT_AMBER  = "#fbbf24";
    inline QString ACCENT_BORDER = "#3b82f6";

    inline QString H_BLUE  = "#1d4ed8";
    inline QString H_GREEN = "#15803d";
    inline QString H_GRAY  = "#6b7280";

    // ── Bordes ────────────────────────────────────────────────────
    inline QString BORDER_MUTED   = "#3a3a3a";
    inline QString BORDER_DIM     = "#444";
    inline QString BORDER_SUBTLE  = "#333";
    inline QString BORDER_PANEL   = "#2a2a3e";
    inline QString BORDER_SUCCESS = "#166534";
    inline QString BORDER_ERROR   = "#7f1d1d";
    inline QString BORDER_LINE    = "#1e3a5f";

    // ── Línea activa / gutter ─────────────────────────────────────
    inline QString LINE_HIGHLIGHT = "#1e3a5f";
    inline QString GUTTER_ARROW   = "#22c55e";

    // ── Terminal ──────────────────────────────────────────────────
    inline QString TERM_SEL       = "#264f78";
    inline QString TERM_SCROLL    = "#30363d";
    inline QString TERM_BTN       = "#238636";
    inline QString TERM_BTN_HOVER = "#2ea043";

    // ── Tarjetas de variables ─────────────────────────────────────
    inline QString CARD_BORDER_NEW = "#818cf8";
    inline QString CARD_BORDER     = "#38bdf8";

    // ── Helper: aplicar tema oscuro ───────────────────────────────
    inline void setDark() {
        BG_APP       = "#1e1e1e";
        BG_EDITOR    = "#1e1e1e";
        BG_PANEL     = "#252526";
        BG_GUTTER    = "#2d2d30";
        BG_MENUBAR   = "#2d2d30";
        BG_TOOLBAR   = "#252526";
        BG_CARD      = "#0f172a";
        BG_TERM      = "#0d1117";
        BG_TERM_BAR  = "#161b22";
        BG_SUCCESS   = "#052e16";
        BG_ERROR     = "#3b0000";
        BG_DISABLED  = "#2d2d2d";
        BG_MENU      = "#2d2d2d";
        BG_ARR_ACT   = "#2d1d00";
        TXT_MAIN     = "#d4d4d4";
        TXT_DIM      = "#9ca3af";
        TXT_BTN      = "#f0f0f0";
        TXT_DISABLED = "#555";
        TXT_TERM     = "#e6edf3";
        TXT_CARD     = "#e2e8f0";
        TXT_CARD_VAL = "#f8fafc";
        TXT_SUCCESS  = "#86efac";
        TXT_ERROR    = "#fca5a5";
        TXT_ARR      = "#93c5fd";
        TXT_ARR_IDX  = "#4b5563";
        ACCENT_BLUE   = "#007acc";
        ACCENT_GREEN  = "#16a34a";
        ACCENT_GRAY   = "#4b5563";
        ACCENT_PURPLE = "#818cf8";
        ACCENT_CYAN   = "#38bdf8";
        ACCENT_AMBER  = "#fbbf24";
        ACCENT_BORDER = "#3b82f6";
        H_BLUE   = "#1d4ed8";
        H_GREEN  = "#15803d";
        H_GRAY   = "#6b7280";
        BORDER_MUTED   = "#3a3a3a";
        BORDER_DIM     = "#444";
        BORDER_SUBTLE  = "#333";
        BORDER_PANEL   = "#2a2a3e";
        BORDER_SUCCESS = "#166534";
        BORDER_ERROR   = "#7f1d1d";
        BORDER_LINE    = "#1e3a5f";
        LINE_HIGHLIGHT = "#1e3a5f";
        GUTTER_ARROW   = "#22c55e";
        TERM_SEL       = "#264f78";
        TERM_SCROLL    = "#30363d";
        TERM_BTN       = "#238636";
        TERM_BTN_HOVER = "#2ea043";
        CARD_BORDER_NEW = "#818cf8";
        CARD_BORDER     = "#38bdf8";
    }

    // ── Helper: aplicar tema claro ────────────────────────────────
    inline void setLight() {
        BG_APP       = "#f5f5f5";
        BG_EDITOR    = "#ffffff";
        BG_PANEL     = "#f0f0f0";
        BG_GUTTER    = "#e8e8e8";
        BG_MENUBAR   = "#e8e8e8";
        BG_TOOLBAR   = "#f0f0f0";
        BG_CARD      = "#ffffff";
        BG_TERM      = "#ffffff";
        BG_TERM_BAR  = "#f8f8f8";
        BG_SUCCESS   = "#dcfce7";
        BG_ERROR     = "#fef2f2";
        BG_DISABLED  = "#e5e5e5";
        BG_MENU      = "#ffffff";
        BG_ARR_ACT   = "#fef3c7";
        TXT_MAIN     = "#1e1e1e";
        TXT_DIM      = "#6b7280";
        TXT_BTN      = "#ffffff";
        TXT_DISABLED = "#9ca3af";
        TXT_TERM     = "#1e1e1e";
        TXT_CARD     = "#1e293b";
        TXT_CARD_VAL = "#0f172a";
        TXT_SUCCESS  = "#166534";
        TXT_ERROR    = "#dc2626";
        TXT_ARR      = "#1e40af";
        TXT_ARR_IDX  = "#6b7280";
        ACCENT_BLUE   = "#2563eb";
        ACCENT_GREEN  = "#16a34a";
        ACCENT_GRAY   = "#6b7280";
        ACCENT_PURPLE = "#7c3aed";
        ACCENT_CYAN   = "#06b6d4";
        ACCENT_AMBER  = "#d97706";
        ACCENT_BORDER = "#3b82f6";
        H_BLUE   = "#1d4ed8";
        H_GREEN  = "#15803d";
        H_GRAY   = "#4b5563";
        BORDER_MUTED   = "#d4d4d4";
        BORDER_DIM     = "#c0c0c0";
        BORDER_SUBTLE  = "#ccc";
        BORDER_PANEL   = "#d0d0d0";
        BORDER_SUCCESS = "#86efac";
        BORDER_ERROR   = "#fca5a5";
        BORDER_LINE    = "#93c5fd";
        LINE_HIGHLIGHT = "#bfdbfe";
        GUTTER_ARROW   = "#22c55e";
        TERM_SEL       = "#bfdbfe";
        TERM_SCROLL    = "#c0c0c0";
        TERM_BTN       = "#2563eb";
        TERM_BTN_HOVER = "#1d4ed8";
        CARD_BORDER_NEW = "#7c3aed";
        CARD_BORDER     = "#06b6d4";
    }

}
