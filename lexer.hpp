#pragma once
#include <string>
#include <vector>

enum class TipoToken {
    NUMERO, LITERAL_BOOLEANO, LITERAL_CARACTER, CADENA,
    SUMA, RESTA, MULTIPLICA, DIVIDE, MODULO,
    IGUAL_IGUAL, DIFERENTE, MENOR, MAYOR, MENOR_IGUAL, MAYOR_IGUAL,
    IGUAL, COMA,
    PAREN_IZ, PAREN_DE, LLAVE_IZ, LLAVE_DE, PUNTO_COMA,
    DECLARAR, BOOLEANO, CARACTER, VACIO, FUNCION, RETORNAR,
    SI, SINO, FIN_SI, MIENTRAS, FIN_MIENTRAS, MOSTRAR, LEER,
    VARIABLE, FIN
};

struct Token {
    TipoToken   tipo;
    std::string valor;
    int         linea;
};

// Utilidad para imprimir nombres de tokens en errores
inline std::string tipoATexto(TipoToken tipo) {
    switch(tipo) {
        case TipoToken::PAREN_IZ: return "(";
        case TipoToken::PAREN_DE: return ")";
        case TipoToken::LLAVE_IZ: return "{";
        case TipoToken::LLAVE_DE: return "}";
        case TipoToken::PUNTO_COMA: return ";";
        case TipoToken::COMA: return ",";
        case TipoToken::IGUAL: return "=";
        default: return "token";
    }
}
