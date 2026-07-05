#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>
#include "core/lexer.hpp"
#include "core/errors.hpp"

static TipoToken palabraReservada(const std::string& palabra) {
    if (palabra == "entero")        return TipoToken::ENTERO;
    if (palabra == "decimal")       return TipoToken::DECIMAL;
    if (palabra == "cadena")        return TipoToken::TIPO_CADENA;
    if (palabra == "booleano")      return TipoToken::BOOLEANO;
    if (palabra == "caracter")      return TipoToken::CARACTER;
    if (palabra == "vacio")         return TipoToken::VACIO;
    if (palabra == "funcion")       return TipoToken::FUNCION;
    if (palabra == "retornar")      return TipoToken::RETORNAR;
    if (palabra == "arreglo")       return TipoToken::ARREGLO;
    if (palabra == "si")            return TipoToken::SI;
    if (palabra == "sino")          return TipoToken::SINO;
    if (palabra == "fin_si")        return TipoToken::FIN_SI;
    if (palabra == "mientras")      return TipoToken::MIENTRAS;
    if (palabra == "fin_mientras")  return TipoToken::FIN_MIENTRAS;
    if (palabra == "para")          return TipoToken::PARA;
    if (palabra == "fin_para")      return TipoToken::FIN_PARA;
    if (palabra == "mostrar")       return TipoToken::MOSTRAR;
    if (palabra == "leer")          return TipoToken::LEER;
    if (palabra == "romper")        return TipoToken::BREAK;
    if (palabra == "continuar")     return TipoToken::CONTINUE;
    if (palabra == "verdadero" || palabra == "falso") return TipoToken::LITERAL_BOOLEANO;
    return TipoToken::VARIABLE;
}

std::vector<Token> tokenizar(const std::string& fuente) {
    std::vector<Token> tokens;
    int linea = 1;
    size_t i = 0;

    while (i < fuente.size()) {
        char c = fuente[i];

        if (c == '\n') { linea++; i++; continue; }
        if (isspace(c)) { i++; continue; }

        // Comentarios de línea y bloque
        if (c == '/' && i + 1 < fuente.size()) {
            if (fuente[i + 1] == '/') {
                while (i < fuente.size() && fuente[i] != '\n') i++;
                continue;
            }
            if (fuente[i + 1] == '*') {
                i += 2;
                bool cerrado = false;
                while (i < fuente.size()) {
                    if (fuente[i] == '\n') linea++;
                    if (fuente[i] == '*' && i + 1 < fuente.size() && fuente[i + 1] == '/') {
                        i += 2; cerrado = true; break;
                    }
                    i++;
                }
                if (!cerrado) throw std::runtime_error(error_lexico_comentario_sin_cerrar(linea));
                continue;
            }
        }

        // Cadenas
        if (c == '"') {
            std::string cad = ""; i++;
            while (i < fuente.size() && fuente[i] != '"') {
                if (fuente[i] == '\n') linea++;
                cad += fuente[i]; i++;
            }
            if (i >= fuente.size()) throw std::runtime_error(error_falta_token("\"", linea));
            i++;
            tokens.push_back({TipoToken::CADENA, cad, linea});
            continue;
        }

        // Caracteres individuales
        if (c == '\'') {
            std::string car = ""; i++;
            if (i < fuente.size() && fuente[i] != '\'') { car += fuente[i]; i++; }
            if (i >= fuente.size() || fuente[i] != '\'') throw std::runtime_error(error_falta_token("'", linea));
            i++;
            tokens.push_back({TipoToken::LITERAL_CARACTER, car, linea});
            continue;
        }

        // Números
        if (isdigit(c)) {
            std::string num = "";
            while (i < fuente.size() && (isdigit(fuente[i]) || fuente[i] == '.')) {
                num += fuente[i]; i++;
            }
            tokens.push_back({TipoToken::NUMERO, num, linea});
            continue;
        }

        // Identificadores y palabras clave
        if (isalpha(c) || c == '_') {
            std::string palabra = "";
            while (i < fuente.size() && (isalnum(fuente[i]) || fuente[i] == '_')) {
                palabra += fuente[i]; i++;
            }
            tokens.push_back({palabraReservada(palabra), palabra, linea});
            continue;
        }

        // Operadores de dos caracteres (orden importa: ++ antes que +, etc.)
        if (c == '+' && i + 1 < fuente.size() && fuente[i + 1] == '+') {
            tokens.push_back({TipoToken::INCREMENTO, "++", linea}); i += 2; continue;
        }
        if (c == '+' && i + 1 < fuente.size() && fuente[i + 1] == '=') {
            tokens.push_back({TipoToken::MAS_IGUAL, "+=", linea}); i += 2; continue;
        }
        if (c == '-' && i + 1 < fuente.size() && fuente[i + 1] == '-') {
            tokens.push_back({TipoToken::DECREMENTO, "--", linea}); i += 2; continue;
        }
        if (c == '-' && i + 1 < fuente.size() && fuente[i + 1] == '=') {
            tokens.push_back({TipoToken::MENOS_IGUAL, "-=", linea}); i += 2; continue;
        }
        if (c == '*' && i + 1 < fuente.size() && fuente[i + 1] == '=') {
            tokens.push_back({TipoToken::POR_IGUAL, "*=", linea}); i += 2; continue;
        }

        // Operadores lógicos
        if (c == '&' && i + 1 < fuente.size() && fuente[i + 1] == '&') {
            tokens.push_back({TipoToken::AND_LOGICO, "&&", linea}); i += 2; continue;
        }
        if (c == '|' && i + 1 < fuente.size() && fuente[i + 1] == '|') {
            tokens.push_back({TipoToken::OR_LOGICO, "||", linea}); i += 2; continue;
        }

        // Operadores de comparación
        if (c == '=') {
            if (i + 1 < fuente.size() && fuente[i + 1] == '=') {
                tokens.push_back({TipoToken::IGUAL_IGUAL, "==", linea}); i += 2;
            } else {
                tokens.push_back({TipoToken::IGUAL, "=", linea}); i++;
            }
            continue;
        }
        if (c == '!' && i + 1 < fuente.size() && fuente[i + 1] == '=') {
            tokens.push_back({TipoToken::DIFERENTE, "!=", linea}); i += 2; continue;
        }
        if (c == '!') {
            tokens.push_back({TipoToken::NOT_LOGICO, "!", linea}); i++; continue;
        }
        if (c == '<') {
            if (i + 1 < fuente.size() && fuente[i + 1] == '=') { tokens.push_back({TipoToken::MENOR_IGUAL, "<=", linea}); i += 2; }
            else { tokens.push_back({TipoToken::MENOR, "<", linea}); i++; }
            continue;
        }
        if (c == '>') {
            if (i + 1 < fuente.size() && fuente[i + 1] == '=') { tokens.push_back({TipoToken::MAYOR_IGUAL, ">=", linea}); i += 2; }
            else { tokens.push_back({TipoToken::MAYOR, ">", linea}); i++; }
            continue;
        }

        switch (c) {
            case '+': tokens.push_back({TipoToken::SUMA,         "+", linea}); break;
            case '-': tokens.push_back({TipoToken::RESTA,        "-", linea}); break;
            case '*': tokens.push_back({TipoToken::MULTIPLICA,   "*", linea}); break;
            case '/': tokens.push_back({TipoToken::DIVIDE,       "/", linea}); break;
            case '%': tokens.push_back({TipoToken::MODULO,       "%", linea}); break;
            case '(': tokens.push_back({TipoToken::PAREN_IZ,     "(", linea}); break;
            case ')': tokens.push_back({TipoToken::PAREN_DE,     ")", linea}); break;
            case '{': tokens.push_back({TipoToken::LLAVE_IZ,     "{", linea}); break;
            case '}': tokens.push_back({TipoToken::LLAVE_DE,     "}", linea}); break;
            case '[': tokens.push_back({TipoToken::CORCHETE_IZ,  "[", linea}); break;
            case ']': tokens.push_back({TipoToken::CORCHETE_DE,  "]", linea}); break;
            case ';': tokens.push_back({TipoToken::PUNTO_COMA,   ";", linea}); break;
            case ',': tokens.push_back({TipoToken::COMA,         ",", linea}); break;
            default:  throw std::runtime_error(error_lexico_caracter(c, linea));
        }
        i++;
    }

    tokens.push_back({TipoToken::FIN, "EOF", linea});
    return tokens;
}
