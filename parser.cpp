#include <vector>
#include <string>
#include <stdexcept>
#include "lexer.hpp"
#include "errors.hpp"

static size_t pos = 0;
static std::vector<Token>* tkns = nullptr;

Token& actual() { return (*tkns)[pos]; }

Token consumir(TipoToken esperado) {
    if (actual().tipo != esperado) {
        throw std::runtime_error(
            error_falta_token(
                esperado == TipoToken::IGUAL    ? "=" :
                esperado == TipoToken::PAREN_IZ ? "(" :
                esperado == TipoToken::PAREN_DE ? ")" : "token",
                actual().linea
            )
        );
    }
    return (*tkns)[pos++];
}

double parsePrimario();
double parseTermino();
double parseExpresion();

double parsePrimario() {
    Token t = actual();

    if (t.tipo == TipoToken::NUMERO) {
        pos++;
        return std::stod(t.valor);
    }

    if (t.tipo == TipoToken::PAREN_IZ) {
        pos++;
        double val = parseExpresion();
        if (actual().tipo != TipoToken::PAREN_DE)
            throw std::runtime_error(error_parentesis_abierto(t.linea));
        pos++;
        return val;
    }

    if (t.tipo == TipoToken::PAREN_DE)
        throw std::runtime_error(error_parentesis_cerrado(t.linea));

    throw std::runtime_error(error_token_inesperado(t.valor, t.linea));
}

double parseTermino() {
    double izq = parsePrimario();
    while (actual().tipo == TipoToken::MULTIPLICA ||
           actual().tipo == TipoToken::DIVIDE) {
        TipoToken op = actual().tipo;
        pos++;
        double der = parsePrimario();
        if (op == TipoToken::DIVIDE && der == 0)
            throw std::runtime_error(error_division_por_cero(actual().linea));
        izq = (op == TipoToken::MULTIPLICA) ? izq * der : izq / der;
    }
    return izq;
}

double parseExpresion() {
    double izq = parseTermino();
    while (actual().tipo == TipoToken::SUMA ||
           actual().tipo == TipoToken::RESTA) {
        TipoToken op = actual().tipo;
        pos++;
        double der = parseTermino();
        izq = (op == TipoToken::SUMA) ? izq + der : izq - der;
    }
    return izq;
}

double parsear(std::vector<Token>& tokens) {
    pos  = 0;
    tkns = &tokens;
    double resultado = parseExpresion();
    if (actual().tipo != TipoToken::FIN)
        throw std::runtime_error(
            error_token_inesperado(actual().valor, actual().linea)
        );
    return resultado;
}
