// parser.cpp — Analizador Sintáctico
// Recibe la lista de tokens del lexer y verifica que el programa
// esté bien formado según la gramática del lenguaje.
//
// Gramática soportada (notación informal):
//
//   programa      → sentencia* FIN
//   sentencia     → declaracion
//               | asignacion
//               | si_stmt
//               | mientras_stmt
//               | mostrar_stmt
//               | expr_stmt
//
//   declaracion   → "numero" VARIABLE "=" expresion ";"
//   asignacion    → VARIABLE "=" expresion ";"
//   si_stmt       → "si" "(" condicion ")" sentencia* ("sino" sentencia*)? "fin_si"
//   mientras_stmt → "mientras" "(" condicion ")" sentencia* "fin_mientras"
//   mostrar_stmt  → "mostrar" "(" (expresion | CADENA) ")" ";"
//   expr_stmt     → expresion ";"
//
//   condicion     → expresion OP_COMP expresion
//   OP_COMP       → "==" | "!=" | "<" | ">" | "<=" | ">="
//
//   expresion     → termino (("+"|"-") termino)*
//   termino       → primario (("*"|"/"|"%") primario)*
//   primario      → NUMERO | VARIABLE | "(" expresion ")" | "-" primario

#include <cmath>
#include <vector>
#include <string>
#include <stdexcept>
#include <iostream>
#include "lexer.hpp"
#include "errors.hpp"
#include "semantic.hpp"

// ─── Estado global del parser ─────────────────────────────────────
static size_t              pos  = 0;
static std::vector<Token>* tkns = nullptr;
static TablaVariables      tabla;

// ─── Prototipos ───────────────────────────────────────────────────
double parseExpresion(bool ejecutar = true);
void   parseSentencia(bool ejecutar);

// ─── Funciones auxiliares ─────────────────────────────────────────

Token& actual() { return (*tkns)[pos]; }

// Avanza si el token actual es del tipo esperado; lanza error si no.
Token consumir(TipoToken esperado) {
    if (actual().tipo != esperado)
        throw std::runtime_error(
            error_falta_token(tipoATexto(esperado), actual().linea)
        );
    return (*tkns)[pos++];
}

bool esTipo(TipoToken tipo) { return actual().tipo == tipo; }

// ─── Nivel 3: primario ────────────────────────────────────────────
double parsePrimario(bool ejecutar) {
    Token t = actual();

    if (t.tipo == TipoToken::NUMERO) {
        pos++;
        return std::stod(t.valor);
    }

    if (t.tipo == TipoToken::VARIABLE) {
        pos++;
        if (ejecutar) return tabla.obtener(t.valor, t.linea);
        // en modo "saltar" igual necesitamos un valor dummy
        return 0.0;
    }

    if (t.tipo == TipoToken::PAREN_IZ) {
        pos++;
        double val = parseExpresion();  // parseExpresion usa ejecutar=true siempre
        if (!esTipo(TipoToken::PAREN_DE))
            throw std::runtime_error(error_parentesis_abierto(t.linea));
        pos++;
        return val;
    }

    if (t.tipo == TipoToken::PAREN_DE)
        throw std::runtime_error(error_parentesis_cerrado(t.linea));

    if (t.tipo == TipoToken::RESTA) {
        pos++;
        return -parsePrimario(ejecutar);
    }

    throw std::runtime_error(error_token_inesperado(t.valor, t.linea));
}

// ─── Nivel 2: término ────────────────────────────────────────────
double parseTermino(bool ejecutar = true) {
    double izq = parsePrimario(ejecutar);
    while (esTipo(TipoToken::MULTIPLICA) ||
           esTipo(TipoToken::DIVIDE)     ||
           esTipo(TipoToken::MODULO)) {
        TipoToken op  = actual().tipo;
        int       lin = actual().linea;
        pos++;
        double der = parsePrimario(ejecutar);
        if (!ejecutar) continue;
        if (op == TipoToken::DIVIDE && der == 0.0)
            throw std::runtime_error(error_division_por_cero(lin));
        if      (op == TipoToken::MULTIPLICA) izq *= der;
        else if (op == TipoToken::DIVIDE)     izq /= der;
        else                                  izq  = std::fmod(izq, der);
    }
    return izq;
}

// ─── Nivel 1: expresión ───────────────────────────────────────────
double parseExpresion(bool ejecutar) {
    double izq = parseTermino(ejecutar);
    while (esTipo(TipoToken::SUMA) || esTipo(TipoToken::RESTA)) {
        TipoToken op = actual().tipo;
        pos++;
        double der = parseTermino(ejecutar);
        if (!ejecutar) continue;
        izq = (op == TipoToken::SUMA) ? izq + der : izq - der;
    }
    return izq;
}

// ─── Condición ───────────────────────────────────────────────────
// condicion → expresion OP_COMP expresion
double parseCondicion(bool ejecutar = true) {
    double izq = parseExpresion(ejecutar);

    TipoToken op = actual().tipo;
    if (op != TipoToken::IGUAL_IGUAL && op != TipoToken::DIFERENTE  &&
        op != TipoToken::MENOR       && op != TipoToken::MAYOR      &&
        op != TipoToken::MENOR_IGUAL && op != TipoToken::MAYOR_IGUAL)
        throw std::runtime_error(
            error_token_inesperado(actual().valor, actual().linea)
        );
    pos++;
    double der = parseExpresion(ejecutar);
    if (!ejecutar) return 0.0;

    switch (op) {
        case TipoToken::IGUAL_IGUAL: return (izq == der) ? 1.0 : 0.0;
        case TipoToken::DIFERENTE:   return (izq != der) ? 1.0 : 0.0;
        case TipoToken::MENOR:       return (izq <  der) ? 1.0 : 0.0;
        case TipoToken::MAYOR:       return (izq >  der) ? 1.0 : 0.0;
        case TipoToken::MENOR_IGUAL: return (izq <= der) ? 1.0 : 0.0;
        case TipoToken::MAYOR_IGUAL: return (izq >= der) ? 1.0 : 0.0;
        default: return 0.0;
    }
}

// ─── Sentencias ───────────────────────────────────────────────────

// declaracion → "numero" VARIABLE "=" expresion ";"
void parseDeclaracion(bool ejecutar) {
    pos++;  // consumir "numero"
    Token nombreTok = consumir(TipoToken::VARIABLE);
    consumir(TipoToken::IGUAL);
    double valor = parseExpresion(ejecutar);
    consumir(TipoToken::PUNTO_COMA);
    if (ejecutar) tabla.declarar(nombreTok.valor, valor, nombreTok.linea);
}

// asignacion → VARIABLE "=" expresion ";"
void parseAsignacion(const Token& varTok, bool ejecutar) {
    pos++;  // consumir "="
    double valor = parseExpresion(ejecutar);
    consumir(TipoToken::PUNTO_COMA);
    if (ejecutar) tabla.asignar(varTok.valor, valor, varTok.linea);
}

// si_stmt → "si" "(" condicion ")" sentencia* ("sino" sentencia*)? "fin_si"
void parseSi(bool ejecutar) {
    pos++;  // consumir "si"
    consumir(TipoToken::PAREN_IZ);
    double cond = parseCondicion(ejecutar);
    consumir(TipoToken::PAREN_DE);

    bool ramaCierta = ejecutar && (cond != 0.0);

    // cuerpo rama verdadera
    while (!esTipo(TipoToken::SINO)   &&
           !esTipo(TipoToken::FIN_SI) &&
           !esTipo(TipoToken::FIN))
        parseSentencia(ramaCierta);

    // rama falsa (opcional)
    if (esTipo(TipoToken::SINO)) {
        pos++;
        bool ramaFalsa = ejecutar && (cond == 0.0);
        while (!esTipo(TipoToken::FIN_SI) && !esTipo(TipoToken::FIN))
            parseSentencia(ramaFalsa);
    }

    consumir(TipoToken::FIN_SI);
}

// mientras_stmt → "mientras" "(" condicion ")" sentencia* "fin_mientras"
// Estrategia: guardamos la posición inicial del "mientras" para poder
// re-evaluar la condición y el cuerpo en cada vuelta.
void parseMientras(bool ejecutar) {
    size_t posMientras = pos;
    pos++;  // consumir "mientras"

    consumir(TipoToken::PAREN_IZ);
    double cond = parseCondicion(ejecutar);
    consumir(TipoToken::PAREN_DE);

    size_t posCuerpo = pos;

    // Si no se debe ejecutar, simplemente recorremos la estructura
    if (!ejecutar) {
        while (!esTipo(TipoToken::FIN_MIENTRAS) && !esTipo(TipoToken::FIN))
            parseSentencia(false);
        consumir(TipoToken::FIN_MIENTRAS);
        return;
    }

    int maxIter = 10000;   // límite de seguridad
    while (cond != 0.0 && maxIter-- > 0) {
        pos = posCuerpo;
        while (!esTipo(TipoToken::FIN_MIENTRAS) && !esTipo(TipoToken::FIN))
            parseSentencia(true);

        // Re-evaluar condición
        pos = posMientras + 1;  // saltar el token "mientras"
        consumir(TipoToken::PAREN_IZ);
        cond = parseCondicion(true);
        consumir(TipoToken::PAREN_DE);
        posCuerpo = pos;
    }

    // Avanzar hasta fin_mientras (caso: condición ya era falsa)
    if (cond == 0.0) {
        while (!esTipo(TipoToken::FIN_MIENTRAS) && !esTipo(TipoToken::FIN))
            parseSentencia(false);
    }

    consumir(TipoToken::FIN_MIENTRAS);
}

// mostrar_stmt → "mostrar" "(" (CADENA | expresion) ")" ";"
void parseMostrar(bool ejecutar) {
    pos++;  // consumir "mostrar"
    consumir(TipoToken::PAREN_IZ);

    if (esTipo(TipoToken::CADENA)) {
        if (ejecutar) std::cout << actual().valor << "\n";
        pos++;
    } else {
        double val = parseExpresion(ejecutar);
        if (ejecutar) std::cout << val << "\n";
    }

    consumir(TipoToken::PAREN_DE);
    consumir(TipoToken::PUNTO_COMA);
}

// ─── Despachador de sentencias ────────────────────────────────────
// El parámetro `ejecutar` indica si la sentencia debe correr (true)
// o simplemente recorrerse sin producir efectos (false), útil para
// saltar ramas de si/sino que no corresponden.
void parseSentencia(bool ejecutar) {
    if (esTipo(TipoToken::DECLARAR)) { parseDeclaracion(ejecutar); return; }
    if (esTipo(TipoToken::SI))       { parseSi(ejecutar);          return; }
    if (esTipo(TipoToken::MIENTRAS)) { parseMientras(ejecutar);    return; }
    if (esTipo(TipoToken::MOSTRAR))  { parseMostrar(ejecutar);     return; }

    // ¿VARIABLE seguida de "="?  →  asignación
    if (esTipo(TipoToken::VARIABLE)) {
        Token varTok = actual();
        if (pos + 1 < tkns->size() && (*tkns)[pos + 1].tipo == TipoToken::IGUAL) {
            pos++;  // consumir VARIABLE
            parseAsignacion(varTok, ejecutar);
            return;
        }
    }

    // Expresión suelta  (para uso futuro: llamadas a función, etc.)
    parseExpresion(ejecutar);
    consumir(TipoToken::PUNTO_COMA);
}

// ─── Punto de entrada del parser ─────────────────────────────────
double parsear(std::vector<Token>& tokens) {
    pos   = 0;
    tkns  = &tokens;
    tabla = TablaVariables();

    while (!esTipo(TipoToken::FIN))
        parseSentencia(true);

    tabla.revisarNoUsadas();
    return 0.0;
}

