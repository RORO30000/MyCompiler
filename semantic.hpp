#pragma once
#include <string>
#include <unordered_map>
#include <stdexcept>
#include "errors.hpp"

// Tabla de variables: guarda nombre → valor
class TablaVariables {
    std::unordered_map<std::string, double> tabla;
    std::unordered_map<std::string, bool>   usadas;

public:
    // Declara una variable nueva
    void declarar(const std::string& nombre, double valor, int linea) {
        if (tabla.count(nombre))
            throw std::runtime_error(error_variable_ya_existe(nombre, linea));
        tabla[nombre]  = valor;
        usadas[nombre] = false;
    }

    // Asigna valor a variable existente
    void asignar(const std::string& nombre, double valor, int linea) {
        if (!tabla.count(nombre))
            throw std::runtime_error(error_variable_no_declarada(nombre, linea));
        tabla[nombre] = valor;
    }

    // Obtiene el valor de una variable
    double obtener(const std::string& nombre, int linea) {
        if (!tabla.count(nombre))
            throw std::runtime_error(error_variable_no_declarada(nombre, linea));
        usadas[nombre] = true;
        return tabla[nombre];
    }

    // Revisa variables declaradas pero nunca usadas
    void revisarNoUsadas() {
        for (auto& [nombre, usada] : usadas)
            if (!usada)
                std::cout << advertencia_variable_no_usada(nombre);
    }
};
