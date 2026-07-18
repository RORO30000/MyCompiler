#include <cmath>
#include <vector>
#include <string>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include "core/lexer.hpp"
#include "core/errors.hpp"
#include "core/semantic.hpp"
#include "core/eventos.hpp"

// ─── Estado global del parser ────────────────────────────────────
static size_t              pos  = 0;
static std::vector<Token>* tkns = nullptr;
static TablaVariables      tabla;
static TablaFunciones      tablaFunciones;

static bool        solicitudRetorno = false;
static std::string valorRetornado   = "0";
static bool        solicitudBreak    = false;
static bool        solicitudContinue = false;
static std::string tipoFuncionActual = "vacio";

// Cola de eventos para la animación (null en modo consola)
static std::vector<EventoPaso>* colaEventos = nullptr;

// Hook para lectura de entrada (null → usa std::cin)
static std::string (*inputHook)() = nullptr;

// ─── Emitir un evento a la cola ──────────────────────────────────
static void emitir(EventoPaso ev) {
    if (colaEventos) colaEventos->push_back(std::move(ev));
}

// ─── Helpers ─────────────────────────────────────────────────────
std::string parseExpresion(bool ejecutar = true);
void        parseSentencia(bool ejecutar);

void setInputHook(std::string (*hook)()) {
    inputHook = hook;
}

Token& actual() { return (*tkns)[pos]; }

Token consumir(TipoToken esperado) {
    if (actual().tipo != esperado)
        throw std::runtime_error(error_falta_token(tipoATexto(esperado), actual().linea));
    return (*tkns)[pos++];
}

bool esTipo(TipoToken tipo) { return actual().tipo == tipo; }

// ─── Saltar bloque { ... } por brace counting ────────────────────
static void saltarBloqueLlaves() {
    if (!esTipo(TipoToken::LLAVE_IZ)) return;
    int depth = 1; pos++;
    while (depth > 0 && !esTipo(TipoToken::FIN)) {
        if (esTipo(TipoToken::LLAVE_IZ)) depth++;
        if (esTipo(TipoToken::LLAVE_DE)) depth--;
        if (depth > 0) pos++;
    }
}

std::string tokenATipoTexto(TipoToken tipo) {
    if (tipo == TipoToken::ENTERO)      return "entero";
    if (tipo == TipoToken::DECIMAL)     return "decimal";
    if (tipo == TipoToken::TIPO_CADENA) return "cadena";
    if (tipo == TipoToken::BOOLEANO)    return "booleano";
    if (tipo == TipoToken::CARACTER)    return "caracter";
    if (tipo == TipoToken::VACIO)       return "vacio";
    return "desconocido";
}

bool esTipoDeclaracion(TipoToken t) {
    return t == TipoToken::ENTERO || t == TipoToken::DECIMAL ||
           t == TipoToken::TIPO_CADENA || t == TipoToken::BOOLEANO ||
           t == TipoToken::CARACTER;
}

std::string formatearNumero(double v) {
    std::string s = std::to_string(v);
    // Normalizar separador decimal: algunos locales usan ',' en vez de '.'
    for (auto& c : s) if (c == ',') c = '.';
    auto pos = s.find_last_not_of('0');
    if (s[pos] == '.') pos--;
    return s.substr(0, pos + 1);
}

// ─── Tipado estricto ─────────────────────────────────────────────
static std::string inferirTipoValor(const std::string& valor) {
    try {
        std::stod(valor);
        size_t dot = valor.find('.');
        if (dot != std::string::npos) {
            // Check that all chars after '.' are digits
            bool soloDigitos = true;
            for (size_t i = dot + 1; i < valor.size(); i++)
                if (!isdigit(valor[i])) { soloDigitos = false; break; }
            if (soloDigitos) return "decimal";
        } else if (valor.find_first_not_of("0123456789-") == std::string::npos) {
            return "entero";
        }
    } catch (...) {}

    if (valor == "verdadero" || valor == "falso")
        return "booleano";

    if (valor.size() == 1)
        return "caracter";

    return "cadena";
}

static void validarTipo(const std::string& esperado, const std::string& valor, int linea) {
    if (esperado == "vacio" || esperado == "desconocido") return;

    // ─── NUEVO: PERMITIR DIRECCIONES Y NULOS EN PUNTEROS ────────────────
    if (esperado.back() == '*') {
        // Si el valor empieza con "0x" (dirección) o es "nulo", es válido
        if (valor.rfind("0x", 0) == 0 || valor == "nulo") {
            return; 
        }
    }
    // ───────────────────────────────────────────────────────────────────

    std::string inferido = inferirTipoValor(valor);

    // cadena acepta cualquier valor (conversion implicita)
    if (esperado == "cadena") return;

    // decimal acepta entero (conversion implicita)
    if (esperado == "decimal" && inferido == "entero") return;

    if (esperado != inferido)
        throw std::runtime_error(error_tipos_incompatibles(esperado, inferido, linea));
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
        if (ejecutar)
            validarTipo(target.parametros[i].first, argumentosEvaluados[i], actual().linea);
        tabla.declarar(target.parametros[i].second, target.parametros[i].first,
                       argumentosEvaluados[i], actual().linea);
        emitir({TipoEvento::VAR_DECLARADA, actual().linea,
                target.parametros[i].second, argumentosEvaluados[i]});
    }

    std::string tipoFuncionAnterior = tipoFuncionActual;
    tipoFuncionActual = target.tipoRetorno;

    pos = target.posicionCuerpoTokens;
    consumir(TipoToken::LLAVE_IZ);

    solicitudRetorno = false;
    valorRetornado   = "0";
    solicitudBreak    = false;
    solicitudContinue = false;

    while (!esTipo(TipoToken::LLAVE_DE) && !esTipo(TipoToken::FIN)) {
        parseSentencia(true);
        if (solicitudRetorno) break;
        if (solicitudBreak)   { solicitudBreak = false; break; }
        if (solicitudContinue) { solicitudContinue = false; break; }
    }

    while (!esTipo(TipoToken::LLAVE_DE) && !esTipo(TipoToken::FIN)) pos++;
    consumir(TipoToken::LLAVE_DE);

    tabla.salirAmbito();
    pos = posRetorno;
    tipoFuncionActual = tipoFuncionAnterior;
    solicitudRetorno = false;

    emitir({TipoEvento::FUNCION_RETORNO, actual().linea, nombreFunc, valorRetornado});
    return valorRetornado;
}

// ─── Forward declarations ────────────────────────────────────────
bool evaluarCondicionCompleta(bool ejecutar);

// ─── Expresiones ─────────────────────────────────────────────────
std::string parsePrimario(bool ejecutar) {
    Token t = actual();
    // 1. Operador Dirección de (&nombre)
    if (esTipo(TipoToken::AMPERSAND)) {
        pos++; // consume '&'
        Token varTok = consumir(TipoToken::VARIABLE);
        if (ejecutar) {
            std::string addr = tabla.obtenerDireccion(varTok.valor, varTok.linea);
            // Genera evento visual de lectura de dirección
            emitir({TipoEvento::VAR_LEIDA, varTok.linea, varTok.valor, addr});
            return addr; // Devuelve "0x7ffdXXXX"
        }
        return "0";
    }

    // 2. Operador Desreferenciación (*puntero) como expresión de lectura
    if (esTipo(TipoToken::MULTIPLICA)) { // '*' unario en este contexto
        pos++; // consume '*'
        // Evaluamos lo que sigue (que debería resultar en una dirección de memoria)
        std::string exprVal = parsePrimario(ejecutar); 
        if (ejecutar) {
            if (exprVal == "0" || exprVal == "nulo") {
                throw std::runtime_error("[ERROR] Intento de desreferenciar un puntero nulo en línea " + std::to_string(actual().linea));
            }
            Variable* varApuntada = tabla.obtenerPorDireccion(exprVal, actual().linea);
            std::string nombreApuntado = tabla.obtenerNombrePorDireccion(exprVal);
            
            // Emitir evento especializado para actualizar el panel de visualización
            emitir({TipoEvento::VAR_LEIDA, actual().linea, "*(" + nombreApuntado + ")", varApuntada->valor});
            return varApuntada->valor;
        }
        return "0";
    }  

    if (t.tipo == TipoToken::NUMERO || t.tipo == TipoToken::CADENA ||
        t.tipo == TipoToken::LITERAL_BOOLEANO || t.tipo == TipoToken::LITERAL_CARACTER) {
        pos++; return t.valor;
    }

    if (t.tipo == TipoToken::NOT_LOGICO) {
        pos++;
        std::string val = parsePrimario(ejecutar);
        if (!ejecutar) return "falso";
        return (val == "falso" || val == "0" || val.empty()) ? "verdadero" : "falso";
    }

    if (t.tipo == TipoToken::RESTA) {
        pos++;
        double v = std::stod(parsePrimario(ejecutar));
        return formatearNumero(-v);
    }

    if (t.tipo == TipoToken::VARIABLE) {
        std::string nombre = t.valor;
        pos++;

        // Incremento/decremento postfijo: nombre++ o nombre--
        if (esTipo(TipoToken::INCREMENTO)) {
            pos++;
            if (ejecutar) {
                std::string val = tabla.obtener(nombre, t.linea).valor;
                double d = std::stod(val) + 1.0;
                tabla.asignar(nombre, formatearNumero(d), t.linea);
                emitir({TipoEvento::VAR_MODIFICADA, t.linea, nombre, formatearNumero(d)});
                return formatearNumero(d - 1.0); // post-incremento: devuelve valor anterior
            }
            return "0";
        }
        if (esTipo(TipoToken::DECREMENTO)) {
            pos++;
            if (ejecutar) {
                std::string val = tabla.obtener(nombre, t.linea).valor;
                double d = std::stod(val) - 1.0;
                tabla.asignar(nombre, formatearNumero(d), t.linea);
                emitir({TipoEvento::VAR_MODIFICADA, t.linea, nombre, formatearNumero(d)});
                return formatearNumero(d + 1.0); // post-decremento
            }
            return "0";
        }

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
            if (tabla.existeArreglo(nombre)) {
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

    // Ternario: si (cond) entonces v sino f
    if (t.tipo == TipoToken::SI) {
        int lineaSi = t.linea;
        pos++; // consume 'si'
        consumir(TipoToken::PAREN_IZ);
        bool cond = evaluarCondicionCompleta(ejecutar);
        consumir(TipoToken::PAREN_DE);
        consumir(TipoToken::ENTONCES);
        std::string valTrue = parseExpresion(ejecutar);
        consumir(TipoToken::SINO);
        std::string valFalse = parseExpresion(ejecutar);
        if (!ejecutar) return "0";
        emitir({TipoEvento::LINEA_ACTIVA, lineaSi});
        emitir({TipoEvento::CONDICION_SI, lineaSi, "", cond ? "verdadero" : "falso"});
        return cond ? valTrue : valFalse;
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
        if (op == TipoToken::MULTIPLICA) izqStr = formatearNumero(izq * der);
        else if (op == TipoToken::DIVIDE) izqStr = formatearNumero(izq / der);
        else izqStr = formatearNumero(std::fmod(izq, der));
    }
    return izqStr;
}

std::string parseExpresion(bool ejecutar) {
    std::string izqStr = parseTermino(ejecutar);
    while (esTipo(TipoToken::SUMA) || esTipo(TipoToken::RESTA)) {
        TipoToken op = actual().tipo; pos++;
        std::string derStr = parseTermino(ejecutar);
        if (!ejecutar) continue;
        if (op == TipoToken::SUMA) {
            double izqN, derN;
            bool izqEsNum = true, derEsNum = true;
            try { izqN = std::stod(izqStr); } catch (...) { izqEsNum = false; }
            try { derN = std::stod(derStr); } catch (...) { derEsNum = false; }
            if (izqEsNum && derEsNum) {
                izqStr = formatearNumero(izqN + derN);
            } else {
                izqStr = izqStr + derStr;
            }
        } else {
            double izq = std::stod(izqStr), der = std::stod(derStr);
            izqStr = formatearNumero(izq - der);
        }
    }
    return izqStr;
}

// ─── Condición con operadores lógicos ────────────────────────────
std::string parseCondicion(bool ejecutar) {
    std::string izqStr = parseExpresion(ejecutar);

    // Si no viene operador de comparación → condición booleana directa
    TipoToken t = actual().tipo;
    if (t != TipoToken::IGUAL_IGUAL && t != TipoToken::DIFERENTE &&
        t != TipoToken::MENOR && t != TipoToken::MAYOR &&
        t != TipoToken::MENOR_IGUAL && t != TipoToken::MAYOR_IGUAL) {
        if (!ejecutar) return "falso";
        return (izqStr == "verdadero") ? "verdadero"
             : (izqStr == "falso")     ? "falso"
             : (std::stod(izqStr) != 0.0) ? "verdadero" : "falso";
    }

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

// ─── Condición completa con && y || ──────────────────────────────
// Retorna "verdadero" o "falso"
bool evaluarCondicionCompleta(bool ejecutar) {
    bool resultado = false;
    bool primera = true;
    TipoToken operador = TipoToken::FIN;

    while (true) {
        if (!primera) {
            if (esTipo(TipoToken::AND_LOGICO)) {
                operador = TipoToken::AND_LOGICO; pos++;
            } else if (esTipo(TipoToken::OR_LOGICO)) {
                operador = TipoToken::OR_LOGICO; pos++;
            } else {
                break;
            }
        }

        std::string val = parseCondicion(ejecutar);
        bool cmpResult = (val == "verdadero");

        if (primera) {
            resultado = cmpResult;
            primera = false;
        } else if (operador == TipoToken::AND_LOGICO) {
            resultado = resultado && cmpResult;
        } else if (operador == TipoToken::OR_LOGICO) {
            resultado = resultado || cmpResult;
        }

        // Short-circuit: skip remaining conditions if result is determined
        if (ejecutar) {
            if ((operador == TipoToken::AND_LOGICO && !resultado) ||
                (operador == TipoToken::OR_LOGICO && resultado)) {
                while (!esTipo(TipoToken::FIN)) {
                    if (esTipo(TipoToken::PAREN_DE) || esTipo(TipoToken::PUNTO_COMA) ||
                        esTipo(TipoToken::LLAVE_DE))
                        break;
                    pos++;
                }
                return resultado;
            }
        }
    }
    return resultado;
}

// ─── Declaración de variable simple ──────────────────────────────
void parseDeclaracion(bool ejecutar) {
    std::string tipoVar = tokenATipoTexto(actual().tipo); pos++;

    // ─── NUEVO: SI VE UN '*', ES UN TIPO PUNTERO ─────────────────
    if (esTipo(TipoToken::MULTIPLICA)) {
        pos++; // Consume el '*'
        tipoVar += "*"; // Convierte "entero" en "entero*"
    }

    Token nombreTok = consumir(TipoToken::VARIABLE);
    std::string valor;
    if (esTipo(TipoToken::IGUAL)) {
        pos++;
        valor = parseExpresion(ejecutar);
    } else if (ejecutar) {
        if      (tipoVar == "entero")   valor = "0";
        else if (tipoVar == "decimal")  valor = "0";
        else if (tipoVar == "booleano") valor = "falso";
        else if (tipoVar == "caracter") valor = " ";

        else if (tipoVar.back() == '*') valor = "nulo";

        else                            valor = "";
    }
    consumir(TipoToken::PUNTO_COMA);
    if (ejecutar) {
        validarTipo(tipoVar, valor, nombreTok.linea);
        tabla.declarar(nombreTok.valor, tipoVar, valor, nombreTok.linea);
        emitir({TipoEvento::LINEA_ACTIVA,  nombreTok.linea});
        emitir({TipoEvento::VAR_DECLARADA, nombreTok.linea, nombreTok.valor, valor});
    }
}

// ─── Declaración de arreglo ──────────────────────────────────────
void parseDeclaracionArreglo(bool ejecutar) {
    int lineaArr = actual().linea;
    pos++;

    std::string tipoElem = tokenATipoTexto(actual().tipo); pos++;

    Token nombreTok = consumir(TipoToken::VARIABLE);

    consumir(TipoToken::CORCHETE_IZ);
    std::string tamStr = parseExpresion(ejecutar);
    consumir(TipoToken::CORCHETE_DE);

    int tamano = ejecutar ? (int)std::stod(tamStr) : 0;

    std::vector<std::string> valoresIniciales;

    if (esTipo(TipoToken::IGUAL)) {
        pos++;
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

        for (int i = 0; i < (int)valoresIniciales.size() && i < tamano; i++) {
            validarTipo(tipoElem, valoresIniciales[i], nombreTok.linea);
            arr.asignar(i, valoresIniciales[i], nombreTok.linea);
        }

        emitir({TipoEvento::LINEA_ACTIVA,      lineaArr});
        emitir({TipoEvento::ARREGLO_DECLARADO, lineaArr, nombreTok.valor,
                formatearNumero(tamano), -1, "", arr.celdas});
    }
}

// ─── Definición de función ───────────────────────────────────────
void parseDefinicionFuncion() {
    std::string tipoRetorno = "vacio";
    bool esSubrutinaVacia = esTipo(TipoToken::VACIO);

    if (esSubrutinaVacia) {
        pos++;
    } else {
        pos++;
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
        throw std::runtime_error(error_falta_llave_izq(nombreFuncTok.linea));
    }

    ObjetoFuncion nuevaFunc = {tipoRetorno, nombreFuncTok.valor, parametros, posCuerpo};
    tablaFunciones.registrar(nuevaFunc, nombreFuncTok.linea);
}

// ─── Definición de función con sintaxis nueva: nombre() { ... } ──
void parseDefinicionFuncionSimple() {
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

    size_t posCuerpo = pos;

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
        throw std::runtime_error(error_falta_llave_izq(nombreFuncTok.linea));
    }

    ObjetoFuncion nuevaFunc = {"vacio", nombreFuncTok.valor, parametros, posCuerpo};
    tablaFunciones.registrar(nuevaFunc, nombreFuncTok.linea);
}

// ─── Asignación de variable simple ───────────────────────────────
void parseAsignacion(const Token& varTok, bool ejecutar) {
    pos++;
    std::string valor = parseExpresion(ejecutar);
    consumir(TipoToken::PUNTO_COMA);
    if (ejecutar) {
        std::string tipoVar = tabla.obtener(varTok.valor, varTok.linea).tipo;
        validarTipo(tipoVar, valor, varTok.linea);
        tabla.asignar(varTok.valor, valor, varTok.linea);
        emitir({TipoEvento::LINEA_ACTIVA,   varTok.linea});
        emitir({TipoEvento::VAR_MODIFICADA, varTok.linea, varTok.valor, valor});
    }
}

// ─── Asignación compuesta: +=, -=, *= ────────────────────────────
void parseAsignacionCompuesta(const Token& varTok, TipoToken opToken, bool ejecutar) {
    pos++;
    std::string derStr = parseExpresion(ejecutar);
    consumir(TipoToken::PUNTO_COMA);
    if (ejecutar) {
        std::string tipoVar = tabla.obtener(varTok.valor, varTok.linea).tipo;
        if (tipoVar != "entero" && tipoVar != "decimal")
            throw std::runtime_error(error_tipos_incompatibles(tipoVar, "numero", varTok.linea));
        double izq = std::stod(tabla.obtener(varTok.valor, varTok.linea).valor);
        double der = std::stod(derStr);
        double r;
        if (opToken == TipoToken::MAS_IGUAL) r = izq + der;
        else if (opToken == TipoToken::MENOS_IGUAL) r = izq - der;
        else r = izq * der;
        std::string val = formatearNumero(r);
        validarTipo(tipoVar, val, varTok.linea);
        tabla.asignar(varTok.valor, val, varTok.linea);
        emitir({TipoEvento::LINEA_ACTIVA,   varTok.linea});
        emitir({TipoEvento::VAR_MODIFICADA, varTok.linea, varTok.valor, val});
    }
}

// ─── Asignación a celda de arreglo ───────────────────────────────
void parseAsignacionArreglo(const Token& varTok, bool ejecutar) {
    pos++;
    std::string idxStr = parseExpresion(ejecutar);
    consumir(TipoToken::CORCHETE_DE);
    consumir(TipoToken::IGUAL);
    std::string valor = parseExpresion(ejecutar);
    consumir(TipoToken::PUNTO_COMA);
    if (ejecutar) {
        int idx = (int)std::stod(idxStr);
        Arreglo& arr = tabla.obtenerArreglo(varTok.valor, varTok.linea);
        validarTipo(arr.tipo, valor, varTok.linea);
        arr.asignar(idx, valor, varTok.linea);
        emitir({TipoEvento::LINEA_ACTIVA,   varTok.linea});
        emitir({TipoEvento::ARREGLO_ESCRITO, varTok.linea, varTok.valor, valor, idx, "", arr.celdas});
    }
}

// ─── si / sino / sino si / fin_si ────────────────────────────────
void parseSi(bool ejecutar) {
    int lineaSi = actual().linea;
    pos++; consumir(TipoToken::PAREN_IZ);
    bool condBool = evaluarCondicionCompleta(ejecutar);
    std::string cond = condBool ? "verdadero" : "falso";
    consumir(TipoToken::PAREN_DE);

    // 'entonces' opcional en si (cond) entonces { ... }
    if (esTipo(TipoToken::ENTONCES)) pos++;

    if (ejecutar)
        emitir({TipoEvento::CONDICION_SI, lineaSi, "", cond});

    bool ramaCierta = ejecutar && (cond == "verdadero");
    bool saltado = ramaCierta;

    // Cuerpo de la rama si (o sino si)
    if (solicitudRetorno || solicitudBreak || solicitudContinue) {
        saltarBloqueLlaves();
    } else {
        while (!esTipo(TipoToken::SINO) && !esTipo(TipoToken::FIN_SI) && !esTipo(TipoToken::FIN)) {
            parseSentencia(ramaCierta);
            if (solicitudRetorno) break;
            if (solicitudBreak || solicitudContinue) break;
        }
    }
    // Si break/continue/retorno ocurrió dentro de {…}, saltar } restantes
    while (esTipo(TipoToken::LLAVE_DE)) pos++;

    // Manejar cadenas sino / sino si
    while (esTipo(TipoToken::SINO)) {
        pos++; // consumir 'sino'

        // sino si (cond) { ... }
        if (esTipo(TipoToken::SI)) {
            int lineaSinoSi = actual().linea;
            pos++; consumir(TipoToken::PAREN_IZ);
            std::string condSino = parseCondicion(ejecutar);
            consumir(TipoToken::PAREN_DE);

            if (ejecutar)
                emitir({TipoEvento::CONDICION_SI, lineaSinoSi, "", condSino});

            bool ramaSinoSi = ejecutar && !saltado && (condSino == "verdadero");
            if (ramaSinoSi) saltado = true;

            if (solicitudRetorno || solicitudBreak || solicitudContinue) {
                saltarBloqueLlaves();
            } else {
                while (!esTipo(TipoToken::SINO) && !esTipo(TipoToken::FIN_SI) && !esTipo(TipoToken::FIN)) {
                    parseSentencia(ramaSinoSi);
                    if (solicitudRetorno || solicitudBreak || solicitudContinue) break;
                }
            }
            while (esTipo(TipoToken::LLAVE_DE)) pos++;
        } else {
            // sino { ... }
            bool ramaFalsa = ejecutar && !saltado;
            saltado = true;

            if (solicitudRetorno || solicitudBreak || solicitudContinue) {
                saltarBloqueLlaves();
            } else {
                while (!esTipo(TipoToken::FIN_SI) && !esTipo(TipoToken::FIN)) {
                    parseSentencia(ramaFalsa);
                    if (solicitudRetorno || solicitudBreak || solicitudContinue) break;
                }
            }
            while (esTipo(TipoToken::LLAVE_DE)) pos++;
            break;
        }
    }

    consumir(TipoToken::FIN_SI);
}

// ─── mientras / fin_mientras ─────────────────────────────────────
void parseMientras(bool ejecutar) {
    size_t posMientras = pos; pos++;
    int lineaMientras  = actual().linea;
    consumir(TipoToken::PAREN_IZ);
    bool condBool = evaluarCondicionCompleta(ejecutar);
    std::string cond = condBool ? "verdadero" : "falso";
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
    bool salir = false;
    while (cond == "verdadero" && maxIter-- > 0 && !salir) {
        pos = posCuerpo;
        tabla.entrarAmbito();
        while (!esTipo(TipoToken::FIN_MIENTRAS) && !esTipo(TipoToken::FIN)) {
            parseSentencia(true);
            if (solicitudRetorno)  { salir = true; break; }
            if (solicitudBreak)    { solicitudBreak = false; salir = true; break; }
            if (solicitudContinue) { solicitudContinue = false; break; }
        }
        tabla.salirAmbito();
        if (salir) break;

        size_t posFinCuerpo = pos;
        pos = posMientras + 1;
        consumir(TipoToken::PAREN_IZ);
        condBool = evaluarCondicionCompleta(true);
        cond = condBool ? "verdadero" : "falso";
        consumir(TipoToken::PAREN_DE);
        posCuerpo = pos;
        if (!condBool) pos = posFinCuerpo;
        emitir({TipoEvento::BUCLE_CONDICION, lineaMientras, "", cond});
    }

    if (salir || !condBool || solicitudRetorno) {
        while (!esTipo(TipoToken::FIN_MIENTRAS) && !esTipo(TipoToken::FIN)) pos++;
    }
    int lineaFin = actual().linea;
    consumir(TipoToken::FIN_MIENTRAS);
    emitir({TipoEvento::BUCLE_FIN, lineaFin});
}

// ─── para / fin_para ─────────────────────────────────────────────
void parsePara(bool ejecutar) {
    pos++; consumir(TipoToken::PAREN_IZ);

    // Inicialización: variable = expr  o  tipo variable = expr  o  vacío
    std::string varInicNombre;
    std::string varInicValor;

    if (!esTipo(TipoToken::PUNTO_COMA)) {
        if (esTipoDeclaracion(actual().tipo)) {
            std::string tipoVar = tokenATipoTexto(actual().tipo); pos++;
            Token nombreTok = consumir(TipoToken::VARIABLE);
            varInicNombre = nombreTok.valor;
            consumir(TipoToken::IGUAL);
            varInicValor = parseExpresion(ejecutar);
            if (ejecutar) {
                tabla.declarar(nombreTok.valor, tipoVar, varInicValor, nombreTok.linea);
            }
        } else {
            Token nombreTok = consumir(TipoToken::VARIABLE);
            varInicNombre = nombreTok.valor;
            consumir(TipoToken::IGUAL);
            varInicValor = parseExpresion(ejecutar);
            if (ejecutar) {
                tabla.asignar(nombreTok.valor, varInicValor, nombreTok.linea);
            }
        }
    }
    if (!esTipo(TipoToken::PUNTO_COMA))
        throw std::runtime_error(error_falta_punto_coma(actual().linea));
    pos++;

    // Condición
    size_t posCond = pos;
    if (esTipo(TipoToken::PUNTO_COMA)) {
        // sin condición → siempre verdadero
        pos++;
    } else {
        pos++; // la condicion será re-evaluada en el bucle
    }
    // Saltar hasta el ; de la condición
    while (!esTipo(TipoToken::PUNTO_COMA) && !esTipo(TipoToken::FIN)) pos++;
    if (!esTipo(TipoToken::PUNTO_COMA))
        throw std::runtime_error(error_falta_punto_coma(actual().linea));
    pos++;

    // Incremento
    size_t posInc = pos;
    while (!esTipo(TipoToken::PAREN_DE) && !esTipo(TipoToken::FIN)) pos++;
    if (!esTipo(TipoToken::PAREN_DE))
        throw std::runtime_error(error_falta_parentesis_der(actual().linea));
    pos++;

    size_t posCuerpo = pos;

    if (!ejecutar) {
        while (!esTipo(TipoToken::FIN_PARA) && !esTipo(TipoToken::FIN))
            parseSentencia(false);
        consumir(TipoToken::FIN_PARA); return;
    }

    // Evaluar condición una vez
    size_t posCondEval = posCond;
    bool condResult = true;
    int maxIter = 10000;
    bool salir = false;

    while (condResult && maxIter-- > 0 && !salir) {
        // Evaluar condición
        pos = posCondEval;
        if (esTipo(TipoToken::PUNTO_COMA)) {
            condResult = true;
        } else {
            condResult = evaluarCondicionCompleta(true);
        }
        // Ir al ;
        while (!esTipo(TipoToken::PUNTO_COMA) && !esTipo(TipoToken::FIN)) pos++;
        if (!esTipo(TipoToken::PUNTO_COMA)) break;
        pos++;

        // Saltar incremento e ir al cuerpo
        pos = posInc;
        while (!esTipo(TipoToken::PAREN_DE) && !esTipo(TipoToken::FIN)) pos++;
        if (!esTipo(TipoToken::PAREN_DE)) break;
        pos = posCuerpo;

        if (!condResult) break;

        // Cuerpo
        tabla.entrarAmbito();
        while (!esTipo(TipoToken::FIN_PARA) && !esTipo(TipoToken::FIN)) {
            parseSentencia(true);
            if (solicitudRetorno)  { salir = true; break; }
            if (solicitudBreak)    { solicitudBreak = false; salir = true; break; }
            if (solicitudContinue) { solicitudContinue = false; break; }
        }
        tabla.salirAmbito();
        if (salir) break;

        // Incremento (sin ; al final, a diferencia de una sentencia normal)
        pos = posInc;
        while (!esTipo(TipoToken::PAREN_DE) && !esTipo(TipoToken::FIN)) {
            Token varTok = actual();
            if (pos + 1 < tkns->size()) {
                TipoToken sig = (*tkns)[pos + 1].tipo;
                if (sig == TipoToken::INCREMENTO) {
                    pos += 2;
                    if (ejecutar) {
                        double d = std::stod(tabla.obtener(varTok.valor, varTok.linea).valor) + 1.0;
                        tabla.asignar(varTok.valor, formatearNumero(d), varTok.linea);
                    }
                } else if (sig == TipoToken::DECREMENTO) {
                    pos += 2;
                    if (ejecutar) {
                        double d = std::stod(tabla.obtener(varTok.valor, varTok.linea).valor) - 1.0;
                        tabla.asignar(varTok.valor, formatearNumero(d), varTok.linea);
                    }
                } else if (sig == TipoToken::MAS_IGUAL || sig == TipoToken::MENOS_IGUAL || sig == TipoToken::POR_IGUAL) {
                    TipoToken op = sig; pos += 2;
                    std::string der = parseExpresion(ejecutar);
                    if (ejecutar) {
                        double izq = std::stod(tabla.obtener(varTok.valor, varTok.linea).valor);
                        double dr = std::stod(der);
                        double r = (op == TipoToken::MAS_IGUAL) ? izq + dr : (op == TipoToken::MENOS_IGUAL) ? izq - dr : izq * dr;
                        tabla.asignar(varTok.valor, formatearNumero(r), varTok.linea);
                    }
                } else if (sig == TipoToken::IGUAL) {
                    pos += 2;
                    std::string val = parseExpresion(ejecutar);
                    if (ejecutar) {
                        tabla.asignar(varTok.valor, val, varTok.linea);
                    }
                } else {
                    pos++;
                }
            } else {
                pos++;
            }
        }
    }

    while (!esTipo(TipoToken::FIN_PARA) && !esTipo(TipoToken::FIN)) pos++;
    int lineaFinPara = actual().linea;
    consumir(TipoToken::FIN_PARA);
    emitir({TipoEvento::BUCLE_FIN, lineaFinPara});
}

// ─── hacer / mientras (do-while) ─────────────────────────────────
void parseHacerMientras(bool ejecutar) {
    pos++; // consume 'hacer'

    if (!esTipo(TipoToken::LLAVE_IZ))
        throw std::runtime_error(error_falta_llave_izq(actual().linea));
    size_t posCuerpo = pos + 1;
    pos++; // consume '{'

    // Skip body (navigate without executing)
    while (!esTipo(TipoToken::LLAVE_DE) && !esTipo(TipoToken::FIN))
        parseSentencia(false);
    consumir(TipoToken::LLAVE_DE);

    if (!esTipo(TipoToken::MIENTRAS))
        throw std::runtime_error(error_falta_palabra_mientras(actual().linea));
    int lineaMientras = actual().linea;
    pos++; // consume 'mientras'

    size_t posCond = pos;
    consumir(TipoToken::PAREN_IZ);
    bool condBool = ejecutar ? evaluarCondicionCompleta(true) : false;
    std::string cond = condBool ? "verdadero" : "falso";
    consumir(TipoToken::PAREN_DE);

    consumir(TipoToken::FIN_MIENTRAS);

    if (!ejecutar) return;

    emitir({TipoEvento::BUCLE_CONDICION, lineaMientras, "", cond});

    int maxIter = 10000;
    bool salir = false;
    do {
        // Execute body
        pos = posCuerpo;
        tabla.entrarAmbito();
        while (true) {
            if (esTipo(TipoToken::LLAVE_DE)) { pos++; break; }
            parseSentencia(true);
            if (solicitudRetorno) { salir = true; break; }
            if (solicitudBreak) { solicitudBreak = false; salir = true; break; }
            if (solicitudContinue) { solicitudContinue = false; break; }
        }
        tabla.salirAmbito();
        if (salir) break;

        // Re-evaluate condition
        pos = posCond;
        consumir(TipoToken::PAREN_IZ);
        condBool = evaluarCondicionCompleta(true);
        cond = condBool ? "verdadero" : "falso";
        consumir(TipoToken::PAREN_DE);

        emitir({TipoEvento::BUCLE_CONDICION, lineaMientras, "", cond});

    } while (cond == "verdadero" && maxIter-- > 0);

    if (salir || !condBool || solicitudRetorno)
        while (!esTipo(TipoToken::FIN_MIENTRAS) && !esTipo(TipoToken::FIN)) pos++;
    int lineaFinMientras = actual().linea;
    consumir(TipoToken::FIN_MIENTRAS);
    emitir({TipoEvento::BUCLE_FIN, lineaFinMientras});
}

// ─── elegir / caso / defecto (switch) ────────────────────────────
void parseElegir(bool ejecutar) {
    int lineaElegir = actual().linea;
    pos++; // consume 'elegir'

    consumir(TipoToken::PAREN_IZ);
    std::string exprVal = ejecutar ? parseExpresion(true) : "0";
    consumir(TipoToken::PAREN_DE);

    consumir(TipoToken::LLAVE_IZ);

    if (ejecutar)
        emitir({TipoEvento::LINEA_ACTIVA, lineaElegir});

    bool casoEjecutado = false;
    bool salirSwitch = false;

    while (!esTipo(TipoToken::LLAVE_DE) && !esTipo(TipoToken::FIN)) {
        if (esTipo(TipoToken::CASO)) {
            int lineaCaso = actual().linea;
            pos++;
            std::string casoVal = parseExpresion(ejecutar);
            consumir(TipoToken::DOS_PUNTOS);

            bool coincide = ejecutar && !casoEjecutado && (exprVal == casoVal);
            if (coincide) casoEjecutado = true;

            if (ejecutar && coincide) {
                emitir({TipoEvento::LINEA_ACTIVA, lineaCaso});
                emitir({TipoEvento::ELEGIR_CASO,  lineaCaso, "", casoVal});
            }

            while (!esTipo(TipoToken::CASO) && !esTipo(TipoToken::DEFECTO) &&
                   !esTipo(TipoToken::LLAVE_DE) && !esTipo(TipoToken::FIN)) {
                if (esTipo(TipoToken::BREAK)) {
                    int linParar = actual().linea;
                    pos++;
                    consumir(TipoToken::PUNTO_COMA);
                    if (ejecutar && (coincide || casoEjecutado)) {
                        emitir({TipoEvento::ROMPER, linParar});
                        solicitudBreak = true;
                        salirSwitch = true;
                    }
                    break;
                }
                parseSentencia(coincide || casoEjecutado);
                if (solicitudRetorno) { salirSwitch = true; break; }
                if (solicitudBreak) { salirSwitch = true; break; }
            }
            if (salirSwitch || solicitudRetorno) break;

        } else if (esTipo(TipoToken::DEFECTO)) {
            int lineaDefecto = actual().linea;
            pos++;
            consumir(TipoToken::DOS_PUNTOS);

            bool ejecutarDefecto = ejecutar && !casoEjecutado;

            if (ejecutar && ejecutarDefecto) {
                emitir({TipoEvento::LINEA_ACTIVA, lineaDefecto});
                emitir({TipoEvento::ELEGIR_CASO,  lineaDefecto, "", "defecto"});
            }

            while (!esTipo(TipoToken::LLAVE_DE) && !esTipo(TipoToken::FIN)) {
                if (esTipo(TipoToken::BREAK)) {
                    int linParar = actual().linea;
                    pos++;
                    consumir(TipoToken::PUNTO_COMA);
                    if (ejecutarDefecto)
                        emitir({TipoEvento::ROMPER, linParar});
                    break;
                }
                parseSentencia(ejecutarDefecto);
                if (solicitudRetorno) break;
                if (solicitudBreak) break;
            }
            break;

        } else {
            pos++;
        }
    }

    if (salirSwitch || solicitudRetorno)
        while (!esTipo(TipoToken::LLAVE_DE) && !esTipo(TipoToken::FIN)) pos++;

    consumir(TipoToken::LLAVE_DE);
    solicitudBreak = false;
}

// ─── mostrar ─────────────────────────────────────────────────────
void parseMostrar(bool ejecutar) {
    int linMostrar = actual().linea;
    pos++;
    consumir(TipoToken::PAREN_IZ);
    std::string val = parseExpresion(ejecutar);
    if (ejecutar) std::cout << val << "\n";
    consumir(TipoToken::PAREN_DE);
    consumir(TipoToken::PUNTO_COMA);
    if (ejecutar)
        emitir({TipoEvento::MOSTRAR_SALIDA, linMostrar, "", val});
}

// ─── leer ────────────────────────────────────────────────────────
void parseLeer(bool ejecutar) {
    int linea = actual().linea;
    pos++;
    consumir(TipoToken::PAREN_IZ);
    Token varTok = consumir(TipoToken::VARIABLE);
    consumir(TipoToken::PAREN_DE);
    consumir(TipoToken::PUNTO_COMA);
    if (ejecutar) {
        emitir({TipoEvento::LEER_SOLICITUD, linea, varTok.valor});
        std::string input;
        if (inputHook)
            input = inputHook();
        else
            std::getline(std::cin, input);
        tabla.asignar(varTok.valor, input, linea);
        emitir({TipoEvento::VAR_MODIFICADA, linea, varTok.valor, input});
    }
}

// ─── Sentencia general ───────────────────────────────────────────
bool esDefinicionFuncionConRetorno() {
    if (!esTipoDeclaracion(actual().tipo)) return false;
    if (pos + 2 >= tkns->size()) return false;
    if ((*tkns)[pos + 1].tipo != TipoToken::VARIABLE) return false;
    if ((*tkns)[pos + 2].tipo != TipoToken::PAREN_IZ) return false;
    size_t tmp = pos + 3;
    int depth = 1;
    while (tmp < tkns->size() && depth > 0) {
        if ((*tkns)[tmp].tipo == TipoToken::PAREN_IZ) depth++;
        if ((*tkns)[tmp].tipo == TipoToken::PAREN_DE) depth--;
        if (depth > 0) tmp++;
    }
    if (tmp + 1 >= tkns->size()) return false;
    return (*tkns)[tmp + 1].tipo == TipoToken::LLAVE_IZ;
}

void parseDefinicionFuncionConRetorno() {
    std::string tipoRetorno = tokenATipoTexto(actual().tipo); pos++;
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

    size_t posCuerpo = pos;

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
        throw std::runtime_error(error_falta_llave_izq(nombreFuncTok.linea));
    }

    ObjetoFuncion nuevaFunc = {tipoRetorno, nombreFuncTok.valor, parametros, posCuerpo};
    tablaFunciones.registrar(nuevaFunc, nombreFuncTok.linea);
}

void parseSentencia(bool ejecutar) {
    if (solicitudRetorno || solicitudBreak || solicitudContinue) return;

    if (esTipo(TipoToken::MULTIPLICA)) { // Unary '*' al inicio de una sentencia indica: *p = valor;
        int lineaAsig = actual().linea;
        pos++; // consume '*'
        
        // Obtenemos el nombre del puntero
        Token ptrTok = consumir(TipoToken::VARIABLE);
        consumir(TipoToken::IGUAL);
        
        std::string nuevoVal = parseExpresion(ejecutar);
        consumir(TipoToken::PUNTO_COMA);
        
        if (ejecutar) {
            // 1. Obtener la dirección que almacena el puntero (ej. "0x7ffd0000")
            std::string direccion = tabla.obtener(ptrTok.valor, ptrTok.linea).valor;
            
            if (direccion == "0" || direccion == "nulo") {
                throw std::runtime_error("[ERROR] Intento de asignación mediante desreferenciación de un puntero nulo.");
            }
            
            // 2. Obtener la variable apuntada en la memoria física simulada
            Variable* varApuntada = tabla.obtenerPorDireccion(direccion, ptrTok.linea);
            std::string nombreApuntado = tabla.obtenerNombrePorDireccion(direccion);
            
            // 3. Validar tipos estrictos (ej. si el puntero es 'entero*' apunta a 'entero')
            // El tipo del puntero es "entero*", el tipo base apuntado es "entero"
            std::string tipoBaseEsperado = tabla.obtener(ptrTok.valor, ptrTok.linea).tipo;
            if (tipoBaseEsperado.back() == '*') {
                tipoBaseEsperado.pop_back(); // "entero*" -> "entero"
            }
            validarTipo(tipoBaseEsperado, nuevoVal, ptrTok.linea);
            
            // 4. Escribir el valor en la variable real
            varApuntada->valor = nuevoVal;
            
            // 5. Emitir eventos para la GUI:
            // Hacemos que la GUI entienda que la variable destino (ej: 'x') ha sido modificada.
            emitir({TipoEvento::LINEA_ACTIVA, lineaAsig});
            emitir({TipoEvento::VAR_MODIFICADA, lineaAsig, nombreApuntado, nuevoVal});
        }
        return;
    }
    


    if (esTipo(TipoToken::LLAVE_IZ) || esTipo(TipoToken::LLAVE_DE)) { pos++; return; }
    if (esTipo(TipoToken::MOSTRAR))  { parseMostrar(ejecutar); return; }

    if (esTipo(TipoToken::ARREGLO)) { parseDeclaracionArreglo(ejecutar); return; }

    if (esTipoDeclaracion(actual().tipo)) {
        if (esDefinicionFuncionConRetorno()) {
            parseDefinicionFuncionConRetorno(); return;
        }
        parseDeclaracion(ejecutar); return;
    }

    if (esTipo(TipoToken::FUNCION) || esTipo(TipoToken::VACIO)) {
        parseDefinicionFuncion(); return;
    }

    if (esTipo(TipoToken::RETORNAR)) {
        int linRet = actual().linea;
        pos++;
        std::string val = parseExpresion(ejecutar);
        consumir(TipoToken::PUNTO_COMA);
        if (ejecutar) {
            validarTipo(tipoFuncionActual, val, linRet);
            solicitudRetorno = true;
            valorRetornado   = val;
            emitir({TipoEvento::LINEA_ACTIVA,    linRet});
            emitir({TipoEvento::FUNCION_RETORNO, linRet, "", val});
        }
        return;
    }

    if (esTipo(TipoToken::BREAK)) {
        int linBreak = actual().linea; pos++;
        consumir(TipoToken::PUNTO_COMA);
        if (ejecutar) {
            solicitudBreak = true;
            emitir({TipoEvento::ROMPER, linBreak});
        }
        return;
    }

    if (esTipo(TipoToken::CONTINUE)) {
        int linCont = actual().linea; pos++;
        consumir(TipoToken::PUNTO_COMA);
        if (ejecutar) {
            solicitudContinue = true;
            emitir({TipoEvento::CONTINUAR, linCont});
        }
        return;
    }

    if (esTipo(TipoToken::LEER))     { parseLeer(ejecutar);       return; }
    if (esTipo(TipoToken::SI)) {
        // Lookahead: si (cond) entonces expr → ternario, si (cond) { → condicional
        bool esTernario = false;
        if (pos + 1 < tkns->size() && (*tkns)[pos + 1].tipo == TipoToken::PAREN_IZ) {
            size_t tmp = pos + 2;
            int depth = 1;
            while (tmp < tkns->size() && depth > 0) {
                if ((*tkns)[tmp].tipo == TipoToken::PAREN_IZ) depth++;
                if ((*tkns)[tmp].tipo == TipoToken::PAREN_DE) depth--;
                if (depth > 0) tmp++;
            }
            tmp++;
            if (tmp < tkns->size() && (*tkns)[tmp].tipo == TipoToken::ENTONCES) {
                // si después de 'entonces' viene '{' → es condicional, no ternario
                if (tmp + 1 < tkns->size() && (*tkns)[tmp + 1].tipo != TipoToken::LLAVE_IZ)
                    esTernario = true;
            }
        }
        if (!esTernario) {
            parseSi(ejecutar);
            return;
        }
        // Ternario a nivel de sentencia → lo agarra parseExpresion abajo
    }
    if (esTipo(TipoToken::ELEGIR))   { parseElegir(ejecutar);       return; }
    if (esTipo(TipoToken::HACER))    { parseHacerMientras(ejecutar); return; }
    if (esTipo(TipoToken::MIENTRAS)) { parseMientras(ejecutar);    return; }
    if (esTipo(TipoToken::PARA))     { parsePara(ejecutar);       return; }

    if (esTipo(TipoToken::VARIABLE)) {
        Token varTok = actual();

        if (pos + 1 < tkns->size() && (*tkns)[pos + 1].tipo == TipoToken::CORCHETE_IZ) {
            pos++; parseAsignacionArreglo(varTok, ejecutar); return;
        }
        if (pos + 1 < tkns->size() && (*tkns)[pos + 1].tipo == TipoToken::PAREN_IZ) {
            // Mirar si después de (...) viene { → definición de función
            size_t tmp = pos + 2;
            int depth = 1;
            while (tmp < tkns->size() && depth > 0) {
                if ((*tkns)[tmp].tipo == TipoToken::PAREN_IZ) depth++;
                if ((*tkns)[tmp].tipo == TipoToken::PAREN_DE) depth--;
                if (depth > 0) tmp++;
            }
            bool esDefinicion = (tmp + 1 < tkns->size() && (*tkns)[tmp + 1].tipo == TipoToken::LLAVE_IZ);
            if (esDefinicion) {
                parseDefinicionFuncionSimple(); return;
            }
            pos++; ejecutarLlamadaFuncion(varTok.valor, ejecutar);
            consumir(TipoToken::PUNTO_COMA); return;
        }
        if (pos + 1 < tkns->size() && (*tkns)[pos + 1].tipo == TipoToken::IGUAL) {
            pos++; parseAsignacion(varTok, ejecutar); return;
        }
        // Asignación compuesta
        if (pos + 1 < tkns->size()) {
            TipoToken sig = (*tkns)[pos + 1].tipo;
            if (sig == TipoToken::MAS_IGUAL || sig == TipoToken::MENOS_IGUAL || sig == TipoToken::POR_IGUAL) {
                pos++; parseAsignacionCompuesta(varTok, sig, ejecutar); return;
            }
        }
        // Incremento/decremento como sentencia
        if (pos + 1 < tkns->size() && (*tkns)[pos + 1].tipo == TipoToken::INCREMENTO) {
            pos += 2;
            if (ejecutar) {
                std::string v = tabla.obtener(varTok.valor, varTok.linea).valor;
                double d = std::stod(v) + 1.0;
                tabla.asignar(varTok.valor, formatearNumero(d), varTok.linea);
                emitir({TipoEvento::VAR_MODIFICADA, varTok.linea, varTok.valor, formatearNumero(d)});
            }
            consumir(TipoToken::PUNTO_COMA); return;
        }
        if (pos + 1 < tkns->size() && (*tkns)[pos + 1].tipo == TipoToken::DECREMENTO) {
            pos += 2;
            if (ejecutar) {
                std::string v = tabla.obtener(varTok.valor, varTok.linea).valor;
                double d = std::stod(v) - 1.0;
                tabla.asignar(varTok.valor, formatearNumero(d), varTok.linea);
                emitir({TipoEvento::VAR_MODIFICADA, varTok.linea, varTok.valor, formatearNumero(d)});
            }
            consumir(TipoToken::PUNTO_COMA); return;
        }
    }

    parseExpresion(ejecutar);
    consumir(TipoToken::PUNTO_COMA);
}

// ─── Punto de entrada principal ──────────────────────────────────
double parsear(std::vector<Token>& tokens, std::vector<EventoPaso>* cola) {
    pos              = 0;
    tkns             = &tokens;
    tabla            = TablaVariables();
    tablaFunciones   = TablaFunciones();
    solicitudRetorno = false;
    solicitudBreak    = false;
    solicitudContinue = false;
    colaEventos      = cola;

    // Fase 1: Registrar funciones y variables globales
    while (!esTipo(TipoToken::FIN))
        parseSentencia(true);

    // Fase 2: Ejecutar 'Principal' si existe
    if (tablaFunciones.existe("Principal")) {
        size_t tokenFinal = pos;

        Token tokParenIz; tokParenIz.tipo = TipoToken::PAREN_IZ; tokParenIz.valor = "("; tokParenIz.linea = 0;
        Token tokParenDe; tokParenDe.tipo = TipoToken::PAREN_DE; tokParenDe.valor = ")"; tokParenDe.linea = 0;
        // Insertar al final (despues de FIN) para no desplazar tokens existentes
        size_t insertPos = tkns->size();
        tokens.insert(tokens.begin() + (long)insertPos, tokParenDe);
        tokens.insert(tokens.begin() + (long)insertPos, tokParenIz);
        pos = insertPos;

        ejecutarLlamadaFuncion("Principal", true);

        tokens.erase(tokens.begin() + (long)insertPos,
                     tokens.begin() + (long)insertPos + 2);
        pos = tokenFinal;
    } else {
        std::cout << "[INFO]: No se encontró la rutina 'Principal()'. El programa finalizó de forma secuencial.\n";
    }

    tabla.revisarGlobalesNoUsadas();
    colaEventos = nullptr;
    return 0.0;
}
