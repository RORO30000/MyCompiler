#include <cmath>
#include <vector>
#include <string>
#include <stdexcept>
#include <iostream>
#include "lexer.hpp"
#include "errors.hpp"
#include "semantic.hpp"

static size_t              pos  = 0;
static std::vector<Token>* tkns = nullptr;
static TablaVariables      tabla;
static TablaFunciones      tablaFunciones;

static bool        solicitudRetorno = false;
static std::string valorRetornado   = "0";

std::string parseExpresion(bool ejecutar = true);
void        parseSentencia(bool ejecutar);

Token& actual() { return (*tkns)[pos]; }

Token consumir(TipoToken esperado) {
    if (actual().tipo != esperado)
        throw std::runtime_error(error_falta_token(tipoATexto(esperado), actual().linea));
    return (*tkns)[pos++];
}

bool esTipo(TipoToken tipo) { return actual().tipo == tipo; }

std::string tokenATipoTexto(TipoToken tipo) {
    if (tipo == TipoToken::DECLARAR) return "numero";
    if (tipo == TipoToken::BOOLEANO) return "booleano";
    if (tipo == TipoToken::CARACTER) return "caracter";
    if (tipo == TipoToken::VACIO)    return "vacio";
    return "desconocido";
}

std::string ejecutarLlamadaFuncion(const std::string& nombreFunc, bool ejecutar) {
    consumir(TipoToken::PAREN_IZ);
    std::vector<std::string> argumentosEvaluados;
    while (!esTipo(TipoToken::PAREN_DE)) {
        argumentosEvaluados.push_back(parseExpresion(ejecutar));
        if (esTipo(TipoToken::COMA)) pos++;
    }
    consumir(TipoToken::PAREN_DE);

    if (!ejecutar) return "0";

    ObjetoFuncion target = tablaFunciones.obtener(nombreFunc, actual().linea);
    if (argumentosEvaluados.size() != target.parametros.size()) {
        throw std::runtime_error(error_argumentos_invalidos(nombreFunc, target.parametros.size(), argumentosEvaluados.size(), actual().linea));
    }

    size_t posRetornoFlujoPrincipal = pos;
    tabla.entrarAmbito();

    for (size_t i = 0; i < target.parametros.size(); i++) {
        tabla.declarar(target.parametros[i].second, target.parametros[i].first, argumentosEvaluados[i], actual().linea);
    }

    pos = target.posicionCuerpoTokens;
    consumir(TipoToken::LLAVE_IZ);

    solicitudRetorno = false;
    valorRetornado   = "0";

    while (!esTipo(TipoToken::LLAVE_DE) && !esTipo(TipoToken::FIN)) {
        parseSentencia(true);
        if (solicitudRetorno) break;
    }

    while (!esTipo(TipoToken::LLAVE_DE) && !esTipo(TipoToken::FIN)) pos++;
    consumir(TipoToken::LLAVE_DE);

    tabla.salirAmbito();
    pos = posRetornoFlujoPrincipal;
    solicitudRetorno = false;

    return valorRetornado;
}

std::string parsePrimario(bool ejecutar) {
    Token t = actual();

    if (t.tipo == TipoToken::NUMERO || t.tipo == TipoToken::CADENA || t.tipo == TipoToken::LITERAL_BOOLEANO || t.tipo == TipoToken::LITERAL_CARACTER) {
        pos++; return t.valor;
    }
    if (t.tipo == TipoToken::VARIABLE) {
        if (pos + 1 < tkns->size() && (*tkns)[pos + 1].tipo == TipoToken::PAREN_IZ) {
            pos++; return ejecutarLlamadaFuncion(t.valor, ejecutar);
        }
        pos++;
        if (ejecutar) return tabla.obtener(t.valor, t.linea).valor;
        return "0";
    }
    if (t.tipo == TipoToken::PAREN_IZ) {
        pos++; std::string val = parseExpresion(ejecutar);
        consumir(TipoToken::PAREN_DE); return val;
    }
    if (t.tipo == TipoToken::RESTA) {
        pos++; double v = std::stod(parsePrimario(ejecutar)); return std::to_string(-v);
    }
    throw std::runtime_error(error_token_inesperado(t.valor, t.linea));
}

std::string parseTermino(bool ejecutar) {
    std::string izqStr = parsePrimario(ejecutar);
    while (esTipo(TipoToken::MULTIPLICA) || esTipo(TipoToken::DIVIDE) || esTipo(TipoToken::MODULO)) {
        TipoToken op  = actual().tipo; int lin = actual().linea; pos++;
        std::string derStr = parsePrimario(ejecutar);
        if (!ejecutar) continue;
        double izq = std::stod(izqStr); double der = std::stod(derStr);
        if (op == TipoToken::DIVIDE && der == 0.0) throw std::runtime_error(error_division_por_cero(lin));
        if (op == TipoToken::MULTIPLICA) izqStr = std::to_string(izq * der);
        else if (op == TipoToken::DIVIDE) izqStr = std::to_string(izq / der);
        else izqStr = std::to_string(std::fmod(izq, der));
    }
    return izqStr;
}

std::string parseExpresion(bool ejecutar) {
    std::string izqStr = parseTermino(ejecutar);
    while (esTipo(TipoToken::SUMA) || esTipo(TipoToken::RESTA)) {
        TipoToken op = actual().tipo; pos++;
        std::string derStr = parseTermino(ejecutar);
        if (!ejecutar) continue;
        double izq = std::stod(izqStr); double der = std::stod(derStr);
        izqStr = (op == TipoToken::SUMA) ? std::to_string(izq + der) : std::to_string(izq - der);
    }
    return izqStr;
}

std::string parseCondicion(bool ejecutar) {
    std::string izqStr = parseExpresion(ejecutar);
    TipoToken op = actual().tipo; pos++;
    std::string derStr = parseExpresion(ejecutar);
    if (!ejecutar) return "falso";

    bool resultado = false;
    try {
        double izq = std::stod(izqStr); double der = std::stod(derStr);
        if (op == TipoToken::IGUAL_IGUAL) resultado = (izq == der);
        if (op == TipoToken::DIFERENTE)   resultado = (izq != der);
        if (op == TipoToken::MENOR)       resultado = (izq < der);
        if (op == TipoToken::MAYOR)       resultado = (izq > der);
        if (op == TipoToken::MENOR_IGUAL) resultado = (izq <= der);
        if (op == TipoToken::MAYOR_IGUAL) resultado = (izq >= der);
    } catch (...) {
        if (op == TipoToken::IGUAL_IGUAL) resultado = (izqStr == derStr);
        if (op == TipoToken::DIFERENTE)   resultado = (izqStr != derStr);
    }
    return resultado ? "verdadero" : "falso";
}

void parseDeclaracion(bool ejecutar) {
    std::string tipoVar = tokenATipoTexto(actual().tipo); pos++;
    Token nombreTok = consumir(TipoToken::VARIABLE);
    consumir(TipoToken::IGUAL);
    std::string valor = parseExpresion(ejecutar);
    consumir(TipoToken::PUNTO_COMA);
    if (ejecutar) tabla.declarar(nombreTok.valor, tipoVar, valor, nombreTok.linea);
}

void parseDefinicionFuncion() {
    std::string tipoRetorno = "vacio";
    bool esSubrutinaVacia = esTipo(TipoToken::VACIO);
    pos++; // Consumir 'funcion' o 'vacio'
    
    Token nombreFuncTok = consumir(TipoToken::VARIABLE);
    consumir(TipoToken::PAREN_IZ);
    
    std::vector<std::pair<std::string, std::string>> parametros;
    while (!esTipo(TipoToken::PAREN_DE) && !esTipo(TipoToken::FIN)) {
        std::string tipoParam = tokenATipoTexto(actual().tipo); 
        pos++; // Consumir el tipo de dato
        Token nombreParam = consumir(TipoToken::VARIABLE);
        parametros.push_back({tipoParam, nombreParam.valor});
        if (esTipo(TipoToken::COMA)) pos++;
    }
    consumir(TipoToken::PAREN_DE);

    if (!esSubrutinaVacia) {
        consumir(TipoToken::RETORNAR);
        tipoRetorno = tokenATipoTexto(actual().tipo); 
        pos++;
    }

    // Guardamos la posición exacta donde inicia la LLAVE IZQUIERDA
    size_t posCuerpo = pos;

    // SALTAR EL CUERPO: Avanzar tokens respetando estrictamente los bloques
    int llavesAbiertas = 0;
    if (esTipo(TipoToken::LLAVE_IZ)) {
        llavesAbiertas++; 
        pos++;
        while (llavesAbiertas > 0 && !esTipo(TipoToken::FIN)) {
            if (esTipo(TipoToken::LLAVE_IZ)) llavesAbiertas++;
            if (esTipo(TipoToken::LLAVE_DE)) llavesAbiertas--;
            if (llavesAbiertas > 0) pos++; // Solo avanzamos si no es la llave de cierre final
        }
        if (llavesAbiertas > 0) {
            throw std::runtime_error(error_llave_abierta(nombreFuncTok.linea));
        }
        consumir(TipoToken::LLAVE_DE); // Consumimos la llave final de manera segura
    } else {
        throw std::runtime_error(error_falta_token("{", nombreFuncTok.linea));
    }

    // Registramos la función con la posición de inicio real de su cuerpo
    ObjetoFuncion nuevaFunc = {tipoRetorno, nombreFuncTok.valor, parametros, posCuerpo};
    tablaFunciones.registrar(nuevaFunc, nombreFuncTok.linea);
}

void parseAsignacion(const Token& varTok, bool ejecutar) {
    pos++; std::string valor = parseExpresion(ejecutar);
    consumir(TipoToken::PUNTO_COMA);
    if (ejecutar) tabla.asignar(varTok.valor, valor, varTok.linea);
}

void parseSi(bool ejecutar) {
    pos++; consumir(TipoToken::PAREN_IZ);
    std::string cond = parseCondicion(ejecutar);
    consumir(TipoToken::PAREN_DE);
    bool ramaCierta = ejecutar && (cond == "verdadero");

    while (!esTipo(TipoToken::SINO) && !esTipo(TipoToken::FIN_SI) && !esTipo(TipoToken::FIN)) {
        parseSentencia(ramaCierta); if (solicitudRetorno) break;
    }
    if (esTipo(TipoToken::SINO)) {
        pos++; bool ramaFalsa = ejecutar && (cond == "falso");
        while (!esTipo(TipoToken::FIN_SI) && !esTipo(TipoToken::FIN)) {
            parseSentencia(ramaFalsa); if (solicitudRetorno) break;
        }
    }
    consumir(TipoToken::FIN_SI);
}

void parseMientras(bool ejecutar) {
    size_t posMientras = pos; pos++;
    consumir(TipoToken::PAREN_IZ);
    std::string cond = parseCondicion(ejecutar);
    consumir(TipoToken::PAREN_DE);
    size_t posCuerpo = pos;

    if (!ejecutar) {
        while (!esTipo(TipoToken::FIN_MIENTRAS) && !esTipo(TipoToken::FIN)) parseSentencia(false);
        consumir(TipoToken::FIN_MIENTRAS); return;
    }

    int maxIter = 10000;
    while (cond == "verdadero" && maxIter-- > 0) {
        pos = posCuerpo;
        while (!esTipo(TipoToken::FIN_MIENTRAS) && !esTipo(TipoToken::FIN)) {
            parseSentencia(true); if (solicitudRetorno) break;
        }
        if (solicitudRetorno) break;
        pos = posMientras + 1; consumir(TipoToken::PAREN_IZ); cond = parseCondicion(true); consumir(TipoToken::PAREN_DE); posCuerpo = pos;
    }
    if (cond == "falso" || solicitudRetorno) {
        while (!esTipo(TipoToken::FIN_MIENTRAS) && !esTipo(TipoToken::FIN)) pos++;
    }
    consumir(TipoToken::FIN_MIENTRAS);
}

void parseMostrar(bool ejecutar) {
    pos++;
    consumir(TipoToken::PAREN_IZ);
    if (esTipo(TipoToken::CADENA)) {
        if (ejecutar) std::cout << actual().valor << "\n";
        pos++;
    } else {
        std::string val = parseExpresion(ejecutar);
        if (ejecutar) std::cout << val << "\n";
    }
    consumir(TipoToken::PAREN_DE);
    consumir(TipoToken::PUNTO_COMA);
}

void parseSentencia(bool ejecutar) {
    if (solicitudRetorno) return;

    if (esTipo(TipoToken::LLAVE_IZ) || esTipo(TipoToken::LLAVE_DE)) { pos++; return; }
    if (esTipo(TipoToken::MOSTRAR))  { parseMostrar(ejecutar);     return; }
    if (esTipo(TipoToken::DECLARAR) || esTipo(TipoToken::BOOLEANO) || esTipo(TipoToken::CARACTER)) { parseDeclaracion(ejecutar); return; }
    if (esTipo(TipoToken::FUNCION) || esTipo(TipoToken::VACIO)) { parseDefinicionFuncion(); return; }
    if (esTipo(TipoToken::RETORNAR)) {
        pos++; std::string val = parseExpresion(ejecutar); consumir(TipoToken::PUNTO_COMA);
        if (ejecutar) { solicitudRetorno = true; valorRetornado = val; }
        return;
    }
    if (esTipo(TipoToken::SI))       { parseSi(ejecutar);          return; }
    if (esTipo(TipoToken::MIENTRAS)) { parseMientras(ejecutar);    return; }

    if (esTipo(TipoToken::VARIABLE)) {
        Token varTok = actual();
        if (pos + 1 < tkns->size() && (*tkns)[pos + 1].tipo == TipoToken::PAREN_IZ) {
            pos++; ejecutarLlamadaFuncion(varTok.valor, ejecutar); consumir(TipoToken::PUNTO_COMA); return;
        }
        if (pos + 1 < tkns->size() && (*tkns)[pos + 1].tipo == TipoToken::IGUAL) {
            pos++; parseAsignacion(varTok, ejecutar); return;
        }
    }
    parseExpresion(ejecutar);
    consumir(TipoToken::PUNTO_COMA);
}

double parsear(std::vector<Token>& tokens) {
    pos   = 0;
    tkns  = &tokens;
    tabla = TablaVariables();
    tablaFunciones = TablaFunciones();
    solicitudRetorno = false;

    // Fase 1: Recolección global. Pasamos 'false' para registrar funciones y
    // variables globales SIN ejecutar sus cuerpos ni llamadas internas de forma prematura.
    while (!esTipo(TipoToken::FIN)) {
        parseSentencia(false);
    }

    // Fase 2: Si encontramos la rutina 'principal', la ejecutamos de verdad.
    if (tablaFunciones.existe("principal")) {
        size_t tokenFinal = pos;

        // ejecutarLlamadaFuncion espera que pos apunte a '('.
        // Como pos esta en FIN, inyectamos tokens virtuales '(' y ')' justo antes del token FIN.
        Token tokParenIz; tokParenIz.tipo = TipoToken::PAREN_IZ; tokParenIz.valor = "("; tokParenIz.linea = 0;
        Token tokParenDe; tokParenDe.tipo = TipoToken::PAREN_DE; tokParenDe.valor = ")"; tokParenDe.linea = 0;
        tokens.insert(tokens.begin() + (long)tokenFinal, tokParenDe);
        tokens.insert(tokens.begin() + (long)tokenFinal, tokParenIz);
        // pos sigue apuntando a tokParenIz

        ejecutarLlamadaFuncion("principal", true);

        // Eliminamos los tokens virtuales
        tokens.erase(tokens.begin() + (long)tokenFinal, tokens.begin() + (long)tokenFinal + 2);
        pos = tokenFinal;
    } else {
        std::cout << "[INFO]: No se encontró la rutina 'principal()'. El programa finalizó de forma secuencial.\n";
    }

    tabla.revisarGlobalesNoUsadas();
    return 0.0;
}
