#pragma once
#include <string>
#include <vector>

enum class TipoToken {
    NUMERO, LITERAL_BOOLEANO, LITERAL_CARACTER, CADENA,
    SUMA, RESTA, MULTIPLICA, DIVIDE, MODULO,
    IGUAL_IGUAL, DIFERENTE, MENOR, MAYOR, MENOR_IGUAL, MAYOR_IGUAL,
    IGUAL, COMA,
    PAREN_IZ, PAREN_DE, LLAVE_IZ, LLAVE_DE, PUNTO_COMA,
    CORCHETE_IZ, CORCHETE_DE,
    ENTERO, DECIMAL, TIPO_CADENA, BOOLEANO, CARACTER, VACIO, FUNCION, RETORNAR,
    ARREGLO,
    SI, SINO, FIN_SI, HACER, MIENTRAS, FIN_MIENTRAS, PARA, FIN_PARA,
    ENTONCES,
    ELEGIR, CASO, DEFECTO, DOS_PUNTOS,
    MOSTRAR, LEER,
    INCREMENTO, DECREMENTO,
    MAS_IGUAL, MENOS_IGUAL, POR_IGUAL,
    AND_LOGICO, OR_LOGICO, NOT_LOGICO,
    BREAK, CONTINUE,
    VARIABLE, FIN,
    AMPERSAND //Para el operador &
};

struct Token {
    TipoToken   tipo;
    std::string valor;
    int         linea;
};

inline std::string tipoATexto(TipoToken tipo) {
    switch(tipo) {
        case TipoToken::PAREN_IZ:      return "(";
        case TipoToken::PAREN_DE:      return ")";
        case TipoToken::LLAVE_IZ:      return "{";
        case TipoToken::LLAVE_DE:      return "}";
        case TipoToken::CORCHETE_IZ:   return "[";
        case TipoToken::CORCHETE_DE:   return "]";
        case TipoToken::PUNTO_COMA:    return ";";
        case TipoToken::COMA:          return ",";
        case TipoToken::DOS_PUNTOS:    return ":";
        case TipoToken::IGUAL:         return "=";
        case TipoToken::FIN_SI:        return "fin_si";
        case TipoToken::FIN_MIENTRAS:  return "fin_mientras";
        case TipoToken::FIN_PARA:      return "fin_para";
        case TipoToken::ENTONCES:      return "entonces";
        case TipoToken::SINO:          return "sino";
        case TipoToken::MIENTRAS:      return "mientras";
        default:                       return "token";
    }
}
