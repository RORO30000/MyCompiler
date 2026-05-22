#pragma once
#include <string>
#include <vector>

enum class TipoToken {
    NUMERO,
    SUMA, RESTA, MULTIPLICA, DIVIDE,
    IGUAL,
    PAREN_IZ, PAREN_DE,
    VARIABLE, DECLARAR,
    SI, SINO, FIN_SI,
    FIN
};

struct Token {
    TipoToken tipo;
    std::string valor;
    int linea;
};

// Declaración de la función (la implementación va en lexer.cpp)
std::vector<Token> tokenizar(const std::string& fuente);
