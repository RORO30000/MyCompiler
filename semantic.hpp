#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <stdexcept>
#include <iostream>
#include "errors.hpp"
#include "lexer.hpp"

// ─── Estructura para Valores del Lenguaje ────────────────────────
struct Variable {
    std::string tipo;   // "numero", "booleano", "caracter", "vacio"
    std::string valor;  // Guardado como texto y casteado dinámicamente
    bool usada = false;
};

// ─── Estructura para almacenar Funciones ─────────────────────────
struct ObjetoFuncion {
    std::string tipoRetorno; // "numero", "booleano", "caracter", "vacio"
    std::string nombre;
    std::vector<std::pair<std::string, std::string>> parametros; // {tipo, nombre}
    size_t posicionCuerpoTokens; // Índice del token donde inicia el bloque '{'
};

// ─── Nueva Tabla de Variables con Ámbitos (Scopes) ────────────────
class TablaVariables {
private:
    // Pila de ámbitos locales (el índice 0 es el ámbito global)
    std::vector<std::unordered_map<std::string, Variable>> ambitos;

public:
    TablaVariables() {
        // Inicializar con el ámbito global
        ambitos.push_back(std::unordered_map<std::string, Variable>());
    }

    // Crear un nuevo ámbito (al entrar a una función)
    void entrarAmbito() {
        ambitos.push_back(std::unordered_map<std::string, Variable>());
    }

    // Destruir el ámbito actual (al salir de una función)
    void salirAmbito() {
        if (ambitos.size() > 1) {
            revisarNoUsadasEnAmbitoActual();
            ambitos.pop_back();
        }
    }

    // Declara una variable en el ámbito más interno activo
    void declarar(const std::string& nombre, const std::string& tipo, const std::string& valor, int linea) {
        if (ambitos.back().count(nombre))
            throw std::runtime_error(error_variable_ya_existe(nombre, linea));
        
        ambitos.back()[nombre] = {tipo, valor, false};
    }

    // Busca y asigna un valor a una variable (comienza desde el ámbito local al global)
    void asignar(const std::string& nombre, const std::string& valor, int linea) {
        for (auto it = ambitos.rbegin(); it != ambitos.rend(); ++it) {
            if (it->count(nombre)) {
                (*it)[nombre].valor = valor;
                return;
            }
        }
        throw std::runtime_error(error_variable_no_declarada(nombre, linea));
    }

    // Obtiene una variable buscando de adentro hacia afuera
    Variable obtener(const std::string& nombre, int linea) {
        for (auto it = ambitos.rbegin(); it != ambitos.rend(); ++it) {
            if (it->count(nombre)) {
                (*it)[nombre].usada = true;
                return (*it)[nombre];
            }
        }
        throw std::runtime_error(error_variable_no_declarada(nombre, linea));
    }

    void revisarNoUsadasEnAmbitoActual() {
        for (auto& [nombre, var] : ambitos.back()) {
            if (!var.usada) {
                std::cout << advertencia_variable_no_usada(nombre);
            }
        }
    }
    
    void revisarGlobalesNoUsadas() {
        for (auto& [nombre, var] : ambitos.front()) {
            if (!var.usada) {
                std::cout << advertencia_variable_no_usada(nombre);
            }
        }
    }
};

// ─── Tabla Global de Funciones Registradas ───────────────────────
class TablaFunciones {
    std::unordered_map<std::string, ObjetoFuncion> funciones;

public:
    void registrar(const ObjetoFuncion& func, int linea) {
        if (funciones.count(func.nombre))
            throw std::runtime_error(error_funcion_ya_existe(func.nombre, linea));
        funciones[func.nombre] = func;
    }

    ObjetoFuncion obtener(const std::string& nombre, int linea) {
        if (!funciones.count(nombre))
            throw std::runtime_error(error_funcion_no_declarada(nombre, linea));
        return funciones[nombre];
    }

    bool existe(const std::string& nombre) {
        return funciones.count(nombre) > 0;
    }
};
