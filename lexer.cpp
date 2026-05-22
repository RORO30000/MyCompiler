// lexer.cpp — Analizador Léxico
// Convierte el código fuente (texto) en una secuencia de Tokens.
//
// Tokens reconocidos:
//   · Números enteros y decimales     42  3.14
//   · Cadenas de texto                "hola mundo"
//   · Identificadores y palabras clave
//   · Operadores aritméticos          + - * / %
//   · Operadores de comparación       == != < > <= >=
//   · Asignación                      =
//   · Paréntesis                      ( )
//   · Punto y coma                    ;
//   · Comentarios de una línea        // ...   (ignorados)

#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>
#include "lexer.hpp"
#include "errors.hpp"

// ─── Tabla de palabras reservadas ────────────────────────────────
// Añade aquí cualquier nueva palabra clave del lenguaje.
static TipoToken palabraReservada(const std::string& palabra) {
    if (palabra == "numero")        return TipoToken::DECLARAR;
    if (palabra == "si")            return TipoToken::SI;
    if (palabra == "sino")          return TipoToken::SINO;
    if (palabra == "fin_si")        return TipoToken::FIN_SI;
    if (palabra == "mientras")      return TipoToken::MIENTRAS;
    if (palabra == "fin_mientras")  return TipoToken::FIN_MIENTRAS;
    if (palabra == "mostrar")       return TipoToken::MOSTRAR;
    if (palabra == "leer")          return TipoToken::LEER;
    return TipoToken::VARIABLE;     // no es reservada → es un identificador
}

// ─── tipoATexto: para mensajes de error legibles ─────────────────
std::string tipoATexto(TipoToken tipo) {
    switch (tipo) {
        case TipoToken::NUMERO:       return "número";
        case TipoToken::CADENA:       return "cadena";
        case TipoToken::SUMA:         return "+";
        case TipoToken::RESTA:        return "-";
        case TipoToken::MULTIPLICA:   return "*";
        case TipoToken::DIVIDE:       return "/";
        case TipoToken::MODULO:       return "%";
        case TipoToken::IGUAL_IGUAL:  return "==";
        case TipoToken::DIFERENTE:    return "!=";
        case TipoToken::MENOR:        return "<";
        case TipoToken::MAYOR:        return ">";
        case TipoToken::MENOR_IGUAL:  return "<=";
        case TipoToken::MAYOR_IGUAL:  return ">=";
        case TipoToken::IGUAL:        return "=";
        case TipoToken::PAREN_IZ:     return "(";
        case TipoToken::PAREN_DE:     return ")";
        case TipoToken::PUNTO_COMA:   return ";";
        case TipoToken::DECLARAR:     return "numero";
        case TipoToken::SI:           return "si";
        case TipoToken::SINO:         return "sino";
        case TipoToken::FIN_SI:       return "fin_si";
        case TipoToken::MIENTRAS:     return "mientras";
        case TipoToken::FIN_MIENTRAS: return "fin_mientras";
        case TipoToken::MOSTRAR:      return "mostrar";
        case TipoToken::LEER:         return "leer";
        case TipoToken::VARIABLE:     return "identificador";
        case TipoToken::FIN:          return "fin de archivo";
        default:                      return "token desconocido";
    }
}

// ─── tokenizar ────────────────────────────────────────────────────
std::vector<Token> tokenizar(const std::string& fuente) {
    std::vector<Token> tokens;
    int    linea = 1;
    size_t i     = 0;

    while (i < fuente.size()) {
        char c = fuente[i];

        // ── Salto de línea ────────────────────────────────────────
        if (c == '\n') { linea++; i++; continue; }

        // ── Espacios y tabulaciones ───────────────────────────────
        if (isspace(c)) { i++; continue; }

        // ── Comentarios de una línea:  // ... ────────────────────
        if (c == '/' && i + 1 < fuente.size() && fuente[i + 1] == '/') {
            while (i < fuente.size() && fuente[i] != '\n') i++;
            continue;   // el '\n' lo procesa el siguiente ciclo
        }

        // ── Números enteros y decimales ───────────────────────────
        // Forma válida:  dígitos  |  dígitos.dígitos
        if (isdigit(c)) {
            std::string num;
            while (i < fuente.size() && isdigit(fuente[i]))
                num += fuente[i++];

            // parte decimal opcional
            if (i < fuente.size() && fuente[i] == '.' &&
                i + 1 < fuente.size() && isdigit(fuente[i + 1])) {
                num += fuente[i++];   // el punto
                while (i < fuente.size() && isdigit(fuente[i]))
                    num += fuente[i++];
            }

            // error: dígito seguido inmediatamente de letra  →  1abc
            if (i < fuente.size() && isalpha(fuente[i]))
                throw std::runtime_error(error_lexico_numero_mal(num + fuente[i], linea));

            tokens.push_back({TipoToken::NUMERO, num, linea});
            continue;
        }

        // ── Identificadores y palabras reservadas ─────────────────
        if (isalpha(c) || c == '_') {
            std::string palabra;
            while (i < fuente.size() && (isalnum(fuente[i]) || fuente[i] == '_'))
                palabra += fuente[i++];
            tokens.push_back({palabraReservada(palabra), palabra, linea});
            continue;
        }

        // ── Cadenas de texto:  "..." ──────────────────────────────
        if (c == '"') {
            std::string contenido;
            int lineaInicio = linea;
            i++;    // consumir la comilla de apertura
            while (i < fuente.size() && fuente[i] != '"') {
                if (fuente[i] == '\n') linea++;
                // secuencias de escape básicas
                if (fuente[i] == '\\' && i + 1 < fuente.size()) {
                    i++;
                    switch (fuente[i]) {
                        case 'n':  contenido += '\n'; break;
                        case 't':  contenido += '\t'; break;
                        case '"':  contenido += '"';  break;
                        case '\\': contenido += '\\'; break;
                        default:   contenido += fuente[i]; break;
                    }
                } else {
                    contenido += fuente[i];
                }
                i++;
            }
            if (i >= fuente.size())
                throw std::runtime_error(error_lexico_caracter('"', lineaInicio));
            i++;    // consumir la comilla de cierre
            tokens.push_back({TipoToken::CADENA, contenido, linea});
            continue;
        }

        // ── Operadores y símbolos ─────────────────────────────────
        switch (c) {
            // Aritméticos
            case '+': tokens.push_back({TipoToken::SUMA,       "+", linea}); break;
            case '-': tokens.push_back({TipoToken::RESTA,      "-", linea}); break;
            case '*': tokens.push_back({TipoToken::MULTIPLICA, "*", linea}); break;
            case '/': tokens.push_back({TipoToken::DIVIDE,     "/", linea}); break;
            case '%': tokens.push_back({TipoToken::MODULO,     "%", linea}); break;

            // Comparación y asignación  (mira el siguiente char)
            case '=':
                if (i + 1 < fuente.size() && fuente[i + 1] == '=') {
                    tokens.push_back({TipoToken::IGUAL_IGUAL, "==", linea}); i++;
                } else {
                    tokens.push_back({TipoToken::IGUAL, "=", linea});
                }
                break;

            case '!':
                if (i + 1 < fuente.size() && fuente[i + 1] == '=') {
                    tokens.push_back({TipoToken::DIFERENTE, "!=", linea}); i++;
                } else {
                    throw std::runtime_error(error_lexico_caracter(c, linea));
                }
                break;

            case '<':
                if (i + 1 < fuente.size() && fuente[i + 1] == '=') {
                    tokens.push_back({TipoToken::MENOR_IGUAL, "<=", linea}); i++;
                } else {
                    tokens.push_back({TipoToken::MENOR, "<", linea});
                }
                break;

            case '>':
                if (i + 1 < fuente.size() && fuente[i + 1] == '=') {
                    tokens.push_back({TipoToken::MAYOR_IGUAL, ">=", linea}); i++;
                } else {
                    tokens.push_back({TipoToken::MAYOR, ">", linea});
                }
                break;

            // Agrupación
            case '(': tokens.push_back({TipoToken::PAREN_IZ,   "(", linea}); break;
            case ')': tokens.push_back({TipoToken::PAREN_DE,   ")", linea}); break;

            // Separadores
            case ';': tokens.push_back({TipoToken::PUNTO_COMA, ";", linea}); break;

            // Carácter no reconocido → error léxico
            default:
                throw std::runtime_error(error_lexico_caracter(c, linea));
        }
        i++;
    }

    tokens.push_back({TipoToken::FIN, "", linea});
    return tokens;
}
