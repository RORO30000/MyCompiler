#pragma once
#include <string>

// ─── Errores Léxicos ──────────────────────────────────────────────
// El texto tiene un símbolo o carácter que el compilador no reconoce o está mal formado.

inline std::string error_lexico_caracter(char c, int linea) {
    return "\n[ERROR LÉXICO - Línea " + std::to_string(linea) + "]\n"
           "  El símbolo '" + std::string(1, c) + "' no es válido en este lenguaje.\n"
           "  Revisa si escribiste algo por accidente.\n";
}

inline std::string error_lexico_numero_mal(const std::string& num, int linea) {
    return "\n[ERROR LÉXICO - Línea " + std::to_string(linea) + "]\n"
           "  El número '" + num + "' está mal escrito.\n"
           "  Un número no puede tener letras en el medio.\n";
}

// Captura si el usuario abrió un /* y el archivo terminó sin cerrarlo con */
inline std::string error_lexico_comentario_sin_cerrar(int linea) {
    return "\n[ERROR LÉXICO - Línea " + std::to_string(linea) + "]\n"
           "  Se abrió un comentario en bloque '/*' que nunca se cerró con '*/'.\n"
           "  Asegúrate de cerrar el bloque antes del fin del archivo.\n";
}


// ─── Errores Sintácticos ──────────────────────────────────────────
// La estructura o el orden de la instrucción no es correcta.

inline std::string error_falta_token(const std::string& esperado, int linea) {
    return "\n[ERROR SINTÁCTICO - Línea " + std::to_string(linea) + "]\n"
           "  Falta '" + esperado + "' en esta parte del código.\n"
           "  Revisa que la instrucción esté completa.\n";
}

inline std::string error_token_inesperado(const std::string& encontrado, int linea) {
    return "\n[ERROR SINTÁCTICO - Línea " + std::to_string(linea) + "]\n"
           "  No se esperaba '" + encontrado + "' en esta posición.\n"
           "  Puede que te hayas equivocado al escribir la instrucción.\n";
}

inline std::string error_parentesis_abierto(int linea) {
    return "\n[ERROR SINTÁCTICO - Línea " + std::to_string(linea) + "]\n"
           "  Hay un paréntesis '(' que nunca se cerró con ')'.\n";
}

inline std::string error_parentesis_cerrado(int linea) {
    return "\n[ERROR SINTÁCTICO - Línea " + std::to_string(linea) + "]\n"
           "  Hay un ')' de más. No abriste el paréntesis antes.\n";
}

// Errores sintácticos de llaves para delimitar bloques {} de funciones
inline std::string error_llave_abierta(int linea) {
    return "\n[ERROR SINTÁCTICO - Línea " + std::to_string(linea) + "]\n"
           "  Falta cerrar una llave '}' para completar el bloque de código.\n";
}

inline std::string error_llave_cerrada(int linea) {
    return "\n[ERROR SINTÁCTICO - Línea " + std::to_string(linea) + "]\n"
           "  Se encontró una llave de cierre '}' inesperada.\n";
}

// Error sintáctico para la estructura general de #incluir
inline std::string error_directiva_mal_formada(int linea) {
    return "\n[ERROR SINTÁCTICO - Línea " + std::to_string(linea) + "]\n"
           "  La directiva #incluir no es correcta.\n"
           "  Debe seguir el formato: #incluir \"nombre_archivo.txt\"\n";
}


// ─── Errores Semánticos (Lógica, Variables y Funciones) ──────────
// El orden es correcto, pero lo que intenta hacer el código no tiene sentido lógico.

inline std::string error_variable_no_declarada(const std::string& nombre, int linea) {
    return "\n[ERROR SEMÁNTICO - Línea " + std::to_string(linea) + "]\n"
           "  La variable '" + nombre + "' se usó antes de ser creada.\n";
}

inline std::string error_variable_ya_existe(const std::string& nombre, int linea) {
    return "\n[ERROR SEMÁNTICO - Línea " + std::to_string(linea) + "]\n"
           "  La variable '" + nombre + "' ya fue creada en este ámbito.\n"
           "  No puedes crearla dos veces en el mismo lugar.\n";
}

// Error al intentar definir dos funciones con el mismo nombre
inline std::string error_funcion_ya_existe(const std::string& nombre, int linea) {
    return "\n[ERROR SEMÁNTICO - Línea " + std::to_string(linea) + "]\n"
           "  La función o subrutina '" + nombre + "' ya fue definida previamente.\n";
}

// Error al invocar una función o subrutina que no existe
inline std::string error_funcion_no_declarada(const std::string& nombre, int linea) {
    return "\n[ERROR SEMÁNTICO - Línea " + std::to_string(linea) + "]\n"
           "  Estás intentando llamar a la función '" + nombre + "', pero no existe.\n";
}

// Error cuando envías más o menos parámetros de los requeridos por la función
inline std::string error_argumentos_invalidos(const std::string& nombre, int esperados, int recibidos, int linea) {
    return "\n[ERROR SEMÁNTICO - Línea " + std::to_string(linea) + "]\n"
           "  La función '" + nombre + "' requiere " + std::to_string(esperados) + 
           " argumentos, pero enviaste " + std::to_string(recibidos) + ".\n";
}

// Error de incompatibilidad de tipos (Ej: Meter texto en una variable numérica)
inline std::string error_tipos_incompatibles(const std::string& tipo1, const std::string& tipo2, int linea) {
    return "\n[ERROR SEMÁNTICO - Línea " + std::to_string(linea) + "]\n"
           "  Estás intentando mezclar un valor de tipo '" + tipo1 + "' con uno de tipo '" + tipo2 + "'.\n"
           "  Solo puedes operar o asignar valores del mismo tipo entre sí.\n";
}

inline std::string error_division_por_cero(int linea) {
    return "\n[ERROR SEMÁNTICO - Línea " + std::to_string(linea) + "]\n"
           "  Estás intentando dividir entre cero.\n"
           "  Eso no es posible en matemáticas. Revisa el divisor.\n";
}


// ─── Advertencias (No detienen el programa) ───────────────────────

inline std::string advertencia_variable_no_usada(const std::string& nombre) {
    return "\n[AVISO]\n"
           "  La variable '" + nombre + "' fue creada pero nunca se usó.\n"
           "  Puede que sea un error o simplemente la puedes eliminar.\n";
}


// ─── Mensajes de Éxito ────────────────────────────────────────────

inline std::string exito_compilacion() {
    return "\n✓ Compilación exitosa. No se encontraron errores.\n";
}

inline std::string exito_ejecucion(const std::string& resultado) {
    return "\n✓ Resultado: " + resultado + "\n";
}
