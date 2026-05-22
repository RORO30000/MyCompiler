#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>
#include "lexer.hpp"
#include "errors.hpp"


std::vector<Token> tokenizar(const std::string& fuente) {
    std::vector<Token> tokens;
    int linea = 1;
    size_t i = 0;

    while (i < fuente.size()) {
        char c = fuente[i];

        // Saltos de línea
        if (c == '\n') { linea++; i++; continue; }

        // Espacios
        if (isspace(c)) { i++; continue; }

        // Números
        if (isdigit(c)) {
            std::string num;
            while (i < fuente.size() && isdigit(fuente[i]))
                num += fuente[i++];
            tokens.push_back({TipoToken::NUMERO, num, linea});
            continue;
        }

        // Palabras clave y variables
        if (isalpha(c) || c == '_') {
            std::string palabra;
            while (i < fuente.size() && (isalnum(fuente[i]) || fuente[i] == '_'))
                palabra += fuente[i++];

            if      (palabra == "numero") tokens.push_back({TipoToken::DECLARAR, palabra, linea});
            else if (palabra == "si")     tokens.push_back({TipoToken::SI,       palabra, linea});
            else if (palabra == "sino")   tokens.push_back({TipoToken::SINO,     palabra, linea});
            else if (palabra == "fin_si") tokens.push_back({TipoToken::FIN_SI,   palabra, linea});
            else                          tokens.push_back({TipoToken::VARIABLE,  palabra, linea});
            continue;
        }

        // Operadores y símbolos
        switch (c) {
            case '+': tokens.push_back({TipoToken::SUMA,        "+", linea}); break;
            case '-': tokens.push_back({TipoToken::RESTA,       "-", linea}); break;
            case '*': tokens.push_back({TipoToken::MULTIPLICA,  "*", linea}); break;
            case '/': tokens.push_back({TipoToken::DIVIDE,      "/", linea}); break;
            case '=': tokens.push_back({TipoToken::IGUAL,       "=", linea}); break;
            case '(': tokens.push_back({TipoToken::PAREN_IZ,    "(", linea}); break;
            case ')': tokens.push_back({TipoToken::PAREN_DE,    ")", linea}); break;
            default:
                throw std::runtime_error(error_lexico_caracter(c, linea));
        }
        i++;
    }

    tokens.push_back({TipoToken::FIN, "", linea});
    return tokens;
}
