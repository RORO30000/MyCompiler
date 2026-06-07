#include <cmath>
#include <vector>
#include <string>
#include <stdexcept>
#include <iostream>
#include "lexer.hpp"
#include "errors.hpp"
#include "semantic.hpp"
#include "eventos.hpp"

// ─── Estado global del parser ────────────────────────────────────
static size_t              pos  = 0;
static std::vector<Token>* tkns = nullptr;
static TablaVariables      tabla;
static TablaFunciones      tablaFunciones;

static bool        solicitudRetorno = false;
static std::string valorRetornado   = "0";

// Cola de eventos para la animación (null en modo consola)
static std::vector<EventoPaso>* colaEventos = nullptr;

// ─── Emitir un evento a la cola ──────────────────────────────────
static void emitir(EventoPaso ev) {
    if (colaEventos) colaEventos->push_back(std::move(ev));
}

// ─── Helpers ─────────────────────────────────────────────────────
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

// ─── Llamada a función ───────────────────────────────────────────
std::string ejecutarLlamadaFuncion(const std::string& nombreFunc, bool ejecutar) {
    consumir(TipoToken::PAREN_IZ);
    std::vector<std::string> argumentosEvaluados;
    while (!esTipo(TipoToken::PAREN_DE)) {
        argumentosEvaluados.push_back(parseExpresion(ejecutar));
        if (esTipo(TipoToken::COMA)) pos++;
    }
    consumir(TipoToken::PAREN_DE);

    if (!ejecutar) return "0";

    if (ejecutar) {
        emitir({TipoEvento::FUNCION_ENTRADA, actual().linea, nombreFunc});
    }

    ObjetoFuncion target = tablaFunciones.obtener(nombreFunc, actual().linea);
    if (argumentosEvaluados.size() != target.parametros.size())
        throw std::runtime_error(error_argumentos_invalidos(
            nombreFunc, target.parametros.size(), argumentosEvaluados.size(), actual().linea));

    size_t posRetorno = pos;
    tabla.entrarAmbito();

    for (size_t i = 0; i < target.parametros.size(); i++) {
        tabla.declarar(target.parametros[i].second, target.parametros[i].first,
                       argumentosEvaluados[i], actual().linea);
        emitir({TipoEvento::VAR_DECLARADA, actual().linea,
                target.parametros[i].second, argumentosEvaluados[i]});
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
    pos = posRetorno;
    solicitudRetorno = false;

    emitir({TipoEvento::FUNCION_RETORNO, actual().linea, nombreFunc, valorRetornado});
    return valorRetornado;
}

// ─── Expresiones ─────────────────────────────────────────────────
std::string parsePrimario(bool ejecutar) {
    Token t = actual();

    if (t.tipo == TipoToken::NUMERO || t.tipo == TipoToken::CADENA ||
        t.tipo == TipoToken::LITERAL_BOOLEANO || t.tipo == TipoToken::LITERAL_CARACTER) {
        pos++; return t.valor;
    }

    if (t.tipo == TipoToken::VARIABLE) {
        std::string nombre = t.valor;
        pos++;

        // Acceso a arreglo: nombre[indice]
        if (esTipo(TipoToken::CORCHETE_IZ)) {
            pos++;
            std::string idxStr = parseExpresion(ejecutar);
            consumir(TipoToken::CORCHETE_DE);
            if (!ejecutar) return "0";
            int idx = (int)std::stod(idxStr);
            Arreglo& arr = tabla.obtenerArreglo(nombre, t.linea);
            std::string val = arr.obtener(idx, t.linea);
            emitir({TipoEvento::ARREGLO_LEIDO, t.linea, nombre, val, idx, "", arr.celdas});
            return val;
        }

        // Llamada a función: nombre(...)
        if (esTipo(TipoToken::PAREN_IZ)) {
            return ejecutarLlamadaFuncion(nombre, ejecutar);
        }

        // Variable simple
        if (ejecutar) {
            // Puede ser arreglo o variable simple
            if (tabla.existeArreglo(nombre)) {
                // No debería llegar aquí sin corchete, pero lo protegemos
                return "0";
            }
            std::string val = tabla.obtener(nombre, t.linea).valor;
            emitir({TipoEvento::VAR_LEIDA, t.linea, nombre, val});
            return val;
        }
        return "0";
    }

    if (t.tipo == TipoToken::PAREN_IZ) {
        pos++;
        std::string val = parseExpresion(ejecutar);
        consumir(TipoToken::PAREN_DE);
        return val;
    }

    if (t.tipo == TipoToken::RESTA) {
        pos++;
        double v = std::stod(parsePrimario(ejecutar));
        return std::to_string(-v);
    }

    throw std::runtime_error(error_token_inesperado(t.valor, t.linea));
}

std::string parseTermino(bool ejecutar) {
    std::string izqStr = parsePrimario(ejecutar);
    while (esTipo(TipoToken::MULTIPLICA) || esTipo(TipoToken::DIVIDE) || esTipo(TipoToken::MODULO)) {
        TipoToken op = actual().tipo; int lin = actual().linea; pos++;
        std::string derStr = parsePrimario(ejecutar);
        if (!ejecutar) continue;
        double izq = std::stod(izqStr), der = std::stod(derStr);
        if (op == TipoToken::DIVIDE && der == 0.0)
            throw std::runtime_error(error_division_por_cero(lin));
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
        double izq = std::stod(izqStr), der = std::stod(derStr);
        izqStr = (op == TipoToken::SUMA) ? std::to_string(izq + der)
                                         : std::to_string(izq - der);
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
        double izq = std::stod(izqStr), der = std::stod(derStr);
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

// ─── Declaración de variable simple ──────────────────────────────
void parseDeclaracion(bool ejecutar) {
    std::string tipoVar = tokenATipoTexto(actual().tipo); pos++;
    Token nombreTok = consumir(TipoToken::VARIABLE);
    consumir(TipoToken::IGUAL);
    std::string valor = parseExpresion(ejecutar);
    consumir(TipoToken::PUNTO_COMA);
    if (ejecutar) {
        tabla.declarar(nombreTok.valor, tipoVar, valor, nombreTok.linea);
        emitir({TipoEvento::LINEA_ACTIVA,  nombreTok.linea});
        emitir({TipoEvento::VAR_DECLARADA, nombreTok.linea, nombreTok.valor, valor});
    }
}

// ─── Declaración de arreglo ──────────────────────────────────────
// Sintaxis:  arreglo numero nombre[tamaño];
// Asignación inicial opcional: arreglo numero nums[3] = {1, 2, 3};
void parseDeclaracionArreglo(bool ejecutar) {
    int lineaArr = actual().linea;
    pos++; // consumir 'arreglo'

    // Tipo de elemento
    std::string tipoElem = tokenATipoTexto(actual().tipo); pos++;

    // Nombre
    Token nombreTok = consumir(TipoToken::VARIABLE);

    // Tamaño entre corchetes
    consumir(TipoToken::CORCHETE_IZ);
    std::string tamStr = parseExpresion(ejecutar);
    consumir(TipoToken::CORCHETE_DE);

    int tamano = ejecutar ? (int)std::stod(tamStr) : 0;

    std::vector<std::string> valoresIniciales;

    // Inicialización opcional: = { v1, v2, ... }
    if (esTipo(TipoToken::IGUAL)) {
        pos++; // consumir '='
        consumir(TipoToken::LLAVE_IZ);
        while (!esTipo(TipoToken::LLAVE_DE) && !esTipo(TipoToken::FIN)) {
            valoresIniciales.push_back(parseExpresion(ejecutar));
            if (esTipo(TipoToken::COMA)) pos++;
        }
        consumir(TipoToken::LLAVE_DE);
    }

    consumir(TipoToken::PUNTO_COMA);

    if (ejecutar) {
        tabla.declararArreglo(nombreTok.valor, tipoElem, tamano, nombreTok.linea);
        Arreglo& arr = tabla.obtenerArreglo(nombreTok.valor, nombreTok.linea);

        // Aplicar valores iniciales si los hay
        for (int i = 0; i < (int)valoresIniciales.size() && i < tamano; i++) {
            arr.asignar(i, valoresIniciales[i], nombreTok.linea);
        }

        emitir({TipoEvento::LINEA_ACTIVA,      lineaArr});
        emitir({TipoEvento::ARREGLO_DECLARADO, lineaArr, nombreTok.valor,
                std::to_string(tamano), -1, "", arr.celdas});
    }
}

// ─── Definición de función ───────────────────────────────────────
void parseDefinicionFuncion() {
    std::string tipoRetorno = "vacio";
    bool esSubrutinaVacia = esTipo(TipoToken::VACIO);

    if (esSubrutinaVacia) {
        pos++; // consumir 'vacio'
    } else {
        pos++; // consumir 'funcion'
    }

    Token nombreFuncTok = consumir(TipoToken::VARIABLE);
    consumir(TipoToken::PAREN_IZ);

    std::vector<std::pair<std::string, std::string>> parametros;
    while (!esTipo(TipoToken::PAREN_DE) && !esTipo(TipoToken::FIN)) {
        std::string tipoParam = tokenATipoTexto(actual().tipo); pos++;
        Token nombreParam = consumir(TipoToken::VARIABLE);
        parametros.push_back({tipoParam, nombreParam.valor});
        if (esTipo(TipoToken::COMA)) pos++;
    }
    consumir(TipoToken::PAREN_DE);

    if (!esSubrutinaVacia) {
        consumir(TipoToken::RETORNAR);
        tipoRetorno = tokenATipoTexto(actual().tipo); pos++;
    }

    size_t posCuerpo = pos;

    // Saltar el cuerpo contando llaves
    int llavesAbiertas = 0;
    if (esTipo(TipoToken::LLAVE_IZ)) {
        llavesAbiertas++; pos++;
        while (llavesAbiertas > 0 && !esTipo(TipoToken::FIN)) {
            if (esTipo(TipoToken::LLAVE_IZ)) llavesAbiertas++;
            if (esTipo(TipoToken::LLAVE_DE)) llavesAbiertas--;
            if (llavesAbiertas > 0) pos++;
        }
        if (llavesAbiertas > 0)
            throw std::runtime_error(error_llave_abierta(nombreFuncTok.linea));
        consumir(TipoToken::LLAVE_DE);
    } else {
        throw std::runtime_error(error_falta_token("{", nombreFuncTok.linea));
    }

    ObjetoFuncion nuevaFunc = {tipoRetorno, nombreFuncTok.valor, parametros, posCuerpo};
    tablaFunciones.registrar(nuevaFunc, nombreFuncTok.linea);
}

// ─── Asignación de variable simple ───────────────────────────────
void parseAsignacion(const Token& varTok, bool ejecutar) {
    pos++; // consumir '='
    std::string valor = parseExpresion(ejecutar);
    consumir(TipoToken::PUNTO_COMA);
    if (ejecutar) {
        tabla.asignar(varTok.valor, valor, varTok.linea);
        emitir({TipoEvento::LINEA_ACTIVA,   varTok.linea});
        emitir({TipoEvento::VAR_MODIFICADA, varTok.linea, varTok.valor, valor});
    }
}

// ─── Asignación a celda de arreglo: nombre[i] = expr ─────────────
void parseAsignacionArreglo(const Token& varTok, bool ejecutar) {
    pos++; // consumir '['
    std::string idxStr = parseExpresion(ejecutar);
    consumir(TipoToken::CORCHETE_DE);
    consumir(TipoToken::IGUAL);
    std::string valor = parseExpresion(ejecutar);
    consumir(TipoToken::PUNTO_COMA);
    if (ejecutar) {
        int idx = (int)std::stod(idxStr);
        Arreglo& arr = tabla.obtenerArreglo(varTok.valor, varTok.linea);
        arr.asignar(idx, valor, varTok.linea);
        emitir({TipoEvento::LINEA_ACTIVA,   varTok.linea});
        emitir({TipoEvento::ARREGLO_ESCRITO, varTok.linea, varTok.valor, valor, idx, "", arr.celdas});
    }
}

// ─── si / sino / fin_si ──────────────────────────────────────────
void parseSi(bool ejecutar) {
    int lineaSi = actual().linea;
    pos++; consumir(TipoToken::PAREN_IZ);
    std::string cond = parseCondicion(ejecutar);
    consumir(TipoToken::PAREN_DE);

    if (ejecutar)
        emitir({TipoEvento::CONDICION_SI, lineaSi, "", cond});

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

// ─── mientras / fin_mientras ─────────────────────────────────────
void parseMientras(bool ejecutar) {
    size_t posMientras = pos; pos++;
    int lineaMientras  = actual().linea;
    consumir(TipoToken::PAREN_IZ);
    std::string cond = parseCondicion(ejecutar);
    consumir(TipoToken::PAREN_DE);
    size_t posCuerpo = pos;

    if (!ejecutar) {
        while (!esTipo(TipoToken::FIN_MIENTRAS) && !esTipo(TipoToken::FIN))
            parseSentencia(false);
        consumir(TipoToken::FIN_MIENTRAS); return;
    }

    if (ejecutar)
        emitir({TipoEvento::BUCLE_CONDICION, lineaMientras, "", cond});

    int maxIter = 10000;
    while (cond == "verdadero" && maxIter-- > 0) {
        pos = posCuerpo;
        while (!esTipo(TipoToken::FIN_MIENTRAS) && !esTipo(TipoToken::FIN)) {
            parseSentencia(true); if (solicitudRetorno) break;
        }
        if (solicitudRetorno) break;

        pos = posMientras + 1;
        consumir(TipoToken::PAREN_IZ);
        cond = parseCondicion(true);
        consumir(TipoToken::PAREN_DE);
        posCuerpo = pos;

        emitir({TipoEvento::BUCLE_CONDICION, lineaMientras, "", cond});
    }

    if (cond == "falso" || solicitudRetorno) {
        while (!esTipo(TipoToken::FIN_MIENTRAS) && !esTipo(TipoToken::FIN)) pos++;
    }
    consumir(TipoToken::FIN_MIENTRAS);
    emitir({TipoEvento::BUCLE_FIN, lineaMientras});
}

// ─── mostrar ─────────────────────────────────────────────────────
void parseMostrar(bool ejecutar) {
    int linMostrar = actual().linea;
    pos++;
    consumir(TipoToken::PAREN_IZ);
    std::string val;
    if (esTipo(TipoToken::CADENA)) {
        val = actual().valor;
        if (ejecutar) std::cout << val << "\n";
        pos++;
    } else {
        val = parseExpresion(ejecutar);
        if (ejecutar) std::cout << val << "\n";
    }
    consumir(TipoToken::PAREN_DE);
    consumir(TipoToken::PUNTO_COMA);
    if (ejecutar)
        emitir({TipoEvento::MOSTRAR_SALIDA, linMostrar, "", val});
}

// ─── retornar ────────────────────────────────────────────────────
// ─── Sentencia general ───────────────────────────────────────────
void parseSentencia(bool ejecutar) {
    if (solicitudRetorno) return;

    if (esTipo(TipoToken::LLAVE_IZ) || esTipo(TipoToken::LLAVE_DE)) { pos++; return; }
    if (esTipo(TipoToken::MOSTRAR))  { parseMostrar(ejecutar); return; }

    // Declaración de arreglo
    if (esTipo(TipoToken::ARREGLO)) { parseDeclaracionArreglo(ejecutar); return; }

    // Declaración de variable simple
    if (esTipo(TipoToken::DECLARAR) || esTipo(TipoToken::BOOLEANO) || esTipo(TipoToken::CARACTER)) {
        parseDeclaracion(ejecutar); return;
    }

    // Definición de función o subrutina
    if (esTipo(TipoToken::FUNCION) || esTipo(TipoToken::VACIO)) {
        parseDefinicionFuncion(); return;
    }

    // retornar
    if (esTipo(TipoToken::RETORNAR)) {
        int linRet = actual().linea;
        pos++;
        std::string val = parseExpresion(ejecutar);
        consumir(TipoToken::PUNTO_COMA);
        if (ejecutar) {
            solicitudRetorno = true;
            valorRetornado   = val;
            emitir({TipoEvento::LINEA_ACTIVA,    linRet});
            emitir({TipoEvento::FUNCION_RETORNO, linRet, "", val});
        }
        return;
    }

    if (esTipo(TipoToken::SI))       { parseSi(ejecutar);      return; }
    if (esTipo(TipoToken::MIENTRAS)) { parseMientras(ejecutar); return; }

    if (esTipo(TipoToken::VARIABLE)) {
        Token varTok = actual();

        // Asignación a arreglo: nombre[i] = expr
        if (pos + 1 < tkns->size() && (*tkns)[pos + 1].tipo == TipoToken::CORCHETE_IZ) {
            pos++; parseAsignacionArreglo(varTok, ejecutar); return;
        }
        // Llamada a función como sentencia: nombre(...)
        if (pos + 1 < tkns->size() && (*tkns)[pos + 1].tipo == TipoToken::PAREN_IZ) {
            pos++; ejecutarLlamadaFuncion(varTok.valor, ejecutar);
            consumir(TipoToken::PUNTO_COMA); return;
        }
        // Asignación simple: nombre = expr
        if (pos + 1 < tkns->size() && (*tkns)[pos + 1].tipo == TipoToken::IGUAL) {
            pos++; parseAsignacion(varTok, ejecutar); return;
        }
    }

    parseExpresion(ejecutar);
    consumir(TipoToken::PUNTO_COMA);
}

// ─── Punto de entrada principal ──────────────────────────────────
// cola == nullptr → modo consola (comportamiento original)
// cola != nullptr → modo GUI, llena la cola sin animar nada en tiempo real
double parsear(std::vector<Token>& tokens, std::vector<EventoPaso>* cola) {
    pos              = 0;
    tkns             = &tokens;
    tabla            = TablaVariables();
    tablaFunciones   = TablaFunciones();
    solicitudRetorno = false;
    colaEventos      = cola;

    // Fase 1: Registrar funciones globales (ejecutar = false)
    while (!esTipo(TipoToken::FIN))
        parseSentencia(false);

    // Fase 2: Ejecutar 'principal' si existe
    if (tablaFunciones.existe("principal")) {
        size_t tokenFinal = pos;

        Token tokParenIz; tokParenIz.tipo = TipoToken::PAREN_IZ; tokParenIz.valor = "("; tokParenIz.linea = 0;
        Token tokParenDe; tokParenDe.tipo = TipoToken::PAREN_DE; tokParenDe.valor = ")"; tokParenDe.linea = 0;
        tokens.insert(tokens.begin() + (long)tokenFinal, tokParenDe);
        tokens.insert(tokens.begin() + (long)tokenFinal, tokParenIz);

        ejecutarLlamadaFuncion("principal", true);

        tokens.erase(tokens.begin() + (long)tokenFinal,
                     tokens.begin() + (long)tokenFinal + 2);
        pos = tokenFinal;
    } else {
        std::cout << "[INFO]: No se encontró la rutina 'principal()'. El programa finalizó de forma secuencial.\n";
    }

    tabla.revisarGlobalesNoUsadas();
    colaEventos = nullptr;
    return 0.0;
}
