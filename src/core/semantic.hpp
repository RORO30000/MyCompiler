#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <stdexcept>
#include <iostream>
#include "core/errors.hpp"
#include "core/lexer.hpp"

// ─── Variable simple ─────────────────────────────────────────────
struct Variable {
    std::string tipo;
    std::string valor;
    bool usada = false;
    std::string direccion;  // Su propia dirección de memoria virtual, ej: "0x7ffd0008"
};

// ─── Arreglo ─────────────────────────────────────────────────────
struct Arreglo {
    std::string tipo;               // tipo de elemento ("numero", etc.)
    std::vector<std::string> celdas; // valores como texto
    bool usada = false;

    int tamano() const { return (int)celdas.size(); }

    std::string obtener(int idx, int linea) const {
        if (idx < 0 || idx >= tamano())
            throw std::runtime_error(error_indice_fuera_rango(idx, tamano(), linea));
        return celdas[idx];
    }

    void asignar(int idx, const std::string& val, int linea) {
        if (idx < 0 || idx >= tamano())
            throw std::runtime_error(error_indice_fuera_rango(idx, tamano(), linea));
        celdas[idx] = val;
    }
};

// ─── Función ─────────────────────────────────────────────────────
struct ObjetoFuncion {
    std::string tipoRetorno;
    std::string nombre;
    std::vector<std::pair<std::string, std::string>> parametros; // {tipo, nombre}
    size_t posicionCuerpoTokens;
};

// ─── Tabla de Variables con Ámbitos ──────────────────────────────
class TablaVariables {
private:
    std::vector<std::unordered_map<std::string, Variable>> ambitos;
    std::vector<std::unordered_map<std::string, Arreglo>>  ambitosArr;

    std::unordered_map<std::string, Variable*> direccionAMemoria;
    std::unordered_map<std::string, std::string> direccionANombre; 
    uint32_t siguienteDireccion = 0x7ffd0000; // Dirección base de la simulación

public:
    TablaVariables() {
        ambitos.push_back({});
        ambitosArr.push_back({});
    }

    void entrarAmbito() {
        ambitos.push_back({});
        ambitosArr.push_back({});
    }

    // ── Variables simples ─────────────────────────────────────────
    void declarar(const std::string& nombre, const std::string& tipo, const std::string& valor, int linea) {
        if (ambitos.back().count(nombre)){
            throw std::runtime_error(error_variable_ya_existe(nombre, linea));                    
        }

        std::stringstream ss;
        ss << "0x" << std::hex << siguienteDireccion;
        std::string addrStr = ss.str();
        siguienteDireccion += 4; // Avanza 4 bytes simulados
                
        // Guardamos la variable con su dirección
        ambitos.back()[nombre] = {tipo, valor, false, addrStr};
                
        // Registramos en el mapa físico virtual para el rastreador de la derecha
        Variable* varRef = &ambitos.back()[nombre];
        direccionAMemoria[addrStr] = varRef;
        direccionANombre[addrStr] = nombre;
    }

    void asignar(const std::string& nombre, const std::string& valor, int linea) {
        for (auto it = ambitos.rbegin(); it != ambitos.rend(); ++it) {
            if (it->count(nombre)) { (*it)[nombre].valor = valor; return; }
        }
        throw std::runtime_error(error_variable_no_declarada(nombre, linea));
    }

    Variable obtener(const std::string& nombre, int linea) {
        for (auto it = ambitos.rbegin(); it != ambitos.rend(); ++it) {
            if (it->count(nombre)) { (*it)[nombre].usada = true; return (*it)[nombre]; }
        }
        throw std::runtime_error(error_variable_no_declarada(nombre, linea));
    }

    bool existeVariable(const std::string& nombre) const {
        for (auto it = ambitos.rbegin(); it != ambitos.rend(); ++it)
            if (it->count(nombre)) return true;
        return false;
    }

    // ── Arreglos ─────────────────────────────────────────────────
    void declararArreglo(const std::string& nombre, const std::string& tipo,
                         int tamano, int linea) {
        if (ambitosArr.back().count(nombre))
            throw std::runtime_error(error_variable_ya_existe(nombre, linea));
        Arreglo arr;
        arr.tipo = tipo;
        arr.celdas.assign(tamano, "0");
        ambitosArr.back()[nombre] = arr;
    }

    Arreglo& obtenerArreglo(const std::string& nombre, int linea) {
        for (auto it = ambitosArr.rbegin(); it != ambitosArr.rend(); ++it) {
            if (it->count(nombre)) { (*it)[nombre].usada = true; return (*it)[nombre]; }
        }
        throw std::runtime_error(error_variable_no_declarada(nombre, linea));
    }

    bool existeArreglo(const std::string& nombre) const {
        for (auto it = ambitosArr.rbegin(); it != ambitosArr.rend(); ++it)
            if (it->count(nombre)) return true;
        return false;
    }

    // ── Advertencias ─────────────────────────────────────────────
    std::string debugAmbitos() const {
        std::string r = "{";
        for (auto& a : ambitos) {
            r += " [";
            for (auto& [k, v] : a)
                r += k + ":" + v.tipo + "/" + v.valor + " ";
            r += "]";
        }
        r += " }";
        return r;
    }

    void revisarNoUsadasEnAmbitoActual() {
        for (auto& [nombre, var] : ambitos.back())
            if (!var.usada) std::cout << advertencia_variable_no_usada(nombre);
        for (auto& [nombre, arr] : ambitosArr.back())
            if (!arr.usada) std::cout << advertencia_variable_no_usada(nombre);
    }

    void revisarGlobalesNoUsadas() {
        for (auto& [nombre, var] : ambitos.front())
            if (!var.usada) std::cout << advertencia_variable_no_usada(nombre);
        for (auto& [nombre, arr] : ambitosArr.front())
            if (!arr.usada) std::cout << advertencia_variable_no_usada(nombre);
    }

    std::string obtenerDireccion(const std::string& nombre, int linea) {
        for (auto it = ambitos.rbegin(); it != ambitos.rend(); ++it) {
            if (it->count(nombre)) return (*it)[nombre].direccion;
        }
        throw std::runtime_error("Error: Variable no declarada '" + nombre + "' en línea " + std::to_string(linea));
    }

    Variable* obtenerPorDireccion(const std::string& addr, int linea) {
        if (direccionAMemoria.count(addr)) {
            return direccionAMemoria[addr];
        }
        throw std::runtime_error("[ERROR DE MEMORIA] Dirección inválida o inaccesible: " + addr + " en línea " + std::to_string(linea));
    }
        
    std::string obtenerNombrePorDireccion(const std::string& addr) {
        if (direccionANombre.count(addr)) return direccionANombre[addr];
        return "desconocido";
    }

    // 3. Modifica tu método salirAmbito() para limpiar el mapa cuando decolen variables:
    void salirAmbito() {
        if (ambitos.size() > 1) {
            revisarNoUsadasEnAmbitoActual();
            for (auto& [nombre, var] : ambitos.back()) {
                direccionAMemoria.erase(var.direccion);
                direccionANombre.erase(var.direccion);
            }
            ambitos.pop_back();
            ambitosArr.pop_back();
        }
    }
};

// ─── Tabla de Funciones ──────────────────────────────────────────
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

    bool existe(const std::string& nombre) { return funciones.count(nombre) > 0; }
};
