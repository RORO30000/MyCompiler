#pragma once
#include <string>
#include <vector>

// ─── Tipos de Token ───────────────────────────────────────────────
enum class TipoToken {
    // Literales
    NUMERO,         // 42  |  3.14
    CADENA,         // "hola"

    // Operadores aritméticos
    SUMA,           // +
    RESTA,          // -
    MULTIPLICA,     // *
    DIVIDE,         // /
    MODULO,         // %

    // Operadores de comparación
    IGUAL_IGUAL,    // ==
    DIFERENTE,      // !=
    MENOR,          // <
    MAYOR,          // >
    MENOR_IGUAL,    // <=
    MAYOR_IGUAL,    // >=

    // Asignación
    IGUAL,          // =

    // Agrupación
    PAREN_IZ,       // (
    PAREN_DE,       // )

    // Separadores
    PUNTO_COMA,     // ;

    // Palabras reservadas
    DECLARAR,       // numero
    SI,             // si
    SINO,           // sino
    FIN_SI,         // fin_si
    MIENTRAS,       // mientras
    FIN_MIENTRAS,   // fin_mientras
    MOSTRAR,        // mostrar
    LEER,           // leer

    // Identificadores y fin
    VARIABLE,       // cualquier nombre
    FIN             // fin del archivo
};

// ─── Estructura Token ─────────────────────────────────────────────
struct Token {
    TipoToken   tipo;
    std::string valor;
    int         linea;
};

// Convierte un TipoToken a texto legible (útil para mensajes de error)
std::string tipoATexto(TipoToken tipo);

// Tokeniza el código fuente y devuelve la lista de tokens
std::vector<Token> tokenizar(const std::string& fuente);

