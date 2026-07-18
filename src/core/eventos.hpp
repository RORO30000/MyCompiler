#pragma once
#include <string>
#include <vector>

// ─── Tipos de evento que el parser puede emitir ──────────────────
enum class TipoEvento {
    LINEA_ACTIVA,       // El flujo llega a una nueva línea de código
    VAR_DECLARADA,      // numero x = 5;
    VAR_MODIFICADA,     // x = x + 1;
    VAR_LEIDA,          // se usa el valor de una variable en una expresión
    ARREGLO_DECLARADO,  // arreglo nums[5];
    ARREGLO_ESCRITO,    // nums[i] = valor;
    ARREGLO_LEIDO,      // se accede a nums[i] para leer
    BUCLE_CONDICION,    // evalúa la condición del mientras (verdadero/falso)
    BUCLE_FIN,          // termina el mientras
    FUNCION_ENTRADA,    // se entra al cuerpo de una función
    FUNCION_RETORNO,    // retornar valor;
    MOSTRAR_SALIDA,     // mostrar(...)
    CONDICION_SI,       // evalúa la condición del si
    LEER_SOLICITUD,
    PUNTERO_MODIFICADO, // Cuando cambia la dirección a la que apunta
    PUNTERO_DESREFERENCIADO     // leer(variable) — solicita entrada al usuario
};

// ─── Un paso individual de la traza de ejecución
struct EventoPaso {
    TipoEvento tipo;
    int linea;
    std::string nombre;
    std::string valor;
    int indice;
    std::string extra;
    std::vector<std::string> celdas;

    EventoPaso(
        TipoEvento t,
        int l,
        std::string n = "",
        std::string v = "",
        int i = -1,
        std::string e = "",
        std::vector<std::string> c = {}
    ) : tipo(t), linea(l), nombre(n), valor(v),
        indice(i), extra(e), celdas(c) {}
};
