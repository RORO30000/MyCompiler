#pragma once
#include <string>

// ─── Errores Léxicos ──────────────────────────────────────────────
// El texto tiene un símbolo o carácter que el compilador no reconoce

inline std::string error_lexico_caracter(char c, int linea) {
    return "\n[ERROR - Línea " + std::to_string(linea) + "]\n"
           "  El símbolo '" + c + "' no es válido en este lenguaje.\n"
           "  Revisa si escribiste algo por accidente.\n";
}

inline std::string error_lexico_numero_mal(const std::string& num, int linea) {
    return "\n[ERROR - Línea " + std::to_string(linea) + "]\n"
           "  El número '" + num + "' está mal escrito.\n"
           "  Un número no puede tener letras en el medio.\n";
}

// ─── Errores Sintácticos ──────────────────────────────────────────
// La estructura de la instrucción no es correcta

inline std::string error_falta_token(const std::string& esperado, int linea) {
    return "\n[ERROR - Línea " + std::to_string(linea) + "]\n"
           "  Falta '" + esperado + "' en esta parte del código.\n"
           "  Revisa que la instrucción esté completa.\n";
}

inline std::string error_token_inesperado(const std::string& encontrado, int linea) {
    return "\n[ERROR - Línea " + std::to_string(linea) + "]\n"
           "  No se esperaba '" + encontrado + "' en esta posición.\n"
           "  Puede que te hayas equivocado al escribir la instrucción.\n";
}

inline std::string error_parentesis_abierto(int linea) {
    return "\n[ERROR - Línea " + std::to_string(linea) + "]\n"
           "  Hay un paréntesis '(' que nunca se cerró con ')'.\n"
           "  Revisa que todos tus paréntesis estén en pares.\n";
}

inline std::string error_parentesis_cerrado(int linea) {
    return "\n[ERROR - Línea " + std::to_string(linea) + "]\n"
           "  Hay un ')' de más. No abriste el paréntesis antes.\n";
}

// ─── Errores Semánticos ───────────────────────────────────────────
// El código se entiende, pero lo que intenta hacer no tiene sentido

inline std::string error_variable_no_declarada(const std::string& nombre, int linea) {
    return "\n[ERROR - Línea " + std::to_string(linea) + "]\n"
           "  La variable '" + nombre + "' se usó antes de ser creada.\n"
           "  Debes declararla primero. Ejemplo:  numero " + nombre + " = 0\n";
}

inline std::string error_variable_ya_existe(const std::string& nombre, int linea) {
    return "\n[ERROR - Línea " + std::to_string(linea) + "]\n"
           "  La variable '" + nombre + "' ya fue creada antes.\n"
           "  No puedes crearla dos veces. Si quieres cambiar su valor, "
           "escribe directamente: " + nombre + " = nuevo_valor\n";
}

inline std::string error_tipos_incompatibles(const std::string& tipo1,
                                              const std::string& tipo2,
                                              int linea) {
    return "\n[ERROR - Línea " + std::to_string(linea) + "]\n"
           "  Estás intentando mezclar un valor de tipo '" + tipo1 +
           "' con uno de tipo '" + tipo2 + "'.\n"
           "  Solo puedes operar valores del mismo tipo entre sí.\n";
}

inline std::string error_division_por_cero(int linea) {
    return "\n[ERROR - Línea " + std::to_string(linea) + "]\n"
           "  Estás intentando dividir entre cero.\n"
           "  Eso no es posible en matemáticas. Revisa el divisor.\n";
}

// ─── Advertencias (no detienen el programa) ───────────────────────

inline std::string advertencia_variable_no_usada(const std::string& nombre) {
    return "\n[AVISO]\n"
           "  La variable '" + nombre + "' fue creada pero nunca se usó.\n"
           "  Puede que sea un error o simplemente la puedes eliminar.\n";
}

// ─── Mensajes de éxito ────────────────────────────────────────────

inline std::string exito_compilacion() {
    return "\n✓ Compilación exitosa. No se encontraron errores.\n";
}

inline std::string exito_ejecucion(const std::string& resultado) {
    return "\n✓ Resultado: " + resultado + "\n";
}
