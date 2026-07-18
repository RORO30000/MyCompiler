#pragma once
#include <string>

// ══════════════════════════════════════════════════════════════════════
//  ERRORES LÉXICOS
//  El texto tiene un símbolo o carácter que el compilador no reconoce
//  o está mal formado a nivel de caracteres.
// ══════════════════════════════════════════════════════════════════════

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

inline std::string error_lexico_cadena_sin_cerrar(int linea) {
    return "\n[ERROR LÉXICO - Línea " + std::to_string(linea) + "]\n"
           "  La cadena de texto no tiene comilla doble de cierre '\"'.\n"
           "  Toda cadena debe empezar y terminar con '\"'.\n";
}

inline std::string error_lexico_caracter_sin_cerrar(int linea) {
    return "\n[ERROR LÉXICO - Línea " + std::to_string(linea) + "]\n"
           "  El carácter no tiene comilla simple de cierre \"'\" .\n"
           "  Todo carácter debe empezar y terminar con \"'\" .\n";
}

inline std::string error_lexico_comentario_sin_cerrar(int linea) {
    return "\n[ERROR LÉXICO - Línea " + std::to_string(linea) + "]\n"
           "  Se abrió un comentario en bloque '/*' que nunca se cerró con '*/'.\n"
           "  Asegúrate de cerrar el bloque antes del fin del archivo.\n";
}


// ══════════════════════════════════════════════════════════════════════
//  ERRORES SINTÁCTICOS
//  La estructura o el orden de la instrucción no es correcta.
// ══════════════════════════════════════════════════════════════════════

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

inline std::string error_falta_punto_coma(int linea) {
    return "\n[ERROR SINTÁCTICO - Línea " + std::to_string(linea) + "]\n"
           "  Falta un punto y coma ';' al final de la instrucción.\n"
           "  Toda instrucción debe terminar con ';'.\n";
}

inline std::string error_falta_parentesis_izq(int linea) {
    return "\n[ERROR SINTÁCTICO - Línea " + std::to_string(linea) + "]\n"
           "  Falta un paréntesis de apertura '('.\n"
           "  Revisa que la expresión esté bien formada.\n";
}

inline std::string error_falta_parentesis_der(int linea) {
    return "\n[ERROR SINTÁCTICO - Línea " + std::to_string(linea) + "]\n"
           "  Falta un paréntesis de cierre ')'.\n"
           "  Revisa que la expresión esté bien formada.\n";
}

inline std::string error_parentesis_abierto(int linea) {
    return "\n[ERROR SINTÁCTICO - Línea " + std::to_string(linea) + "]\n"
           "  Hay un paréntesis '(' que nunca se cerró con ')'.\n";
}

inline std::string error_parentesis_cerrado(int linea) {
    return "\n[ERROR SINTÁCTICO - Línea " + std::to_string(linea) + "]\n"
           "  Hay un ')' de más. No abriste el paréntesis antes.\n";
}

inline std::string error_falta_llave_izq(int linea) {
    return "\n[ERROR SINTÁCTICO - Línea " + std::to_string(linea) + "]\n"
           "  Falta una llave de apertura '{' para iniciar el bloque.\n"
           "  Los bloques de código deben ir entre '{' y '}'.\n";
}

inline std::string error_falta_llave_der(int linea) {
    return "\n[ERROR SINTÁCTICO - Línea " + std::to_string(linea) + "]\n"
           "  Falta una llave de cierre '}' para cerrar el bloque.\n"
           "  Los bloques de código deben ir entre '{' y '}'.\n";
}

inline std::string error_llave_abierta(int linea) {
    return "\n[ERROR SINTÁCTICO - Línea " + std::to_string(linea) + "]\n"
           "  Falta cerrar una llave '}' para completar el bloque de código.\n";
}

inline std::string error_llave_cerrada(int linea) {
    return "\n[ERROR SINTÁCTICO - Línea " + std::to_string(linea) + "]\n"
           "  Se encontró una llave de cierre '}' inesperada.\n";
}

inline std::string error_falta_estructura_si(int linea) {
    return "\n[ERROR SINTÁCTICO - Línea " + std::to_string(linea) + "]\n"
           "  La estructura 'si' está incompleta.\n"
           "  Debe seguir el formato: si (condicion) { ... } sino { ... } fin_si\n";
}

inline std::string error_falta_fin_si(int linea) {
    return "\n[ERROR SINTÁCTICO - Línea " + std::to_string(linea) + "]\n"
           "  Falta 'fin_si' para cerrar la estructura 'si'.\n"
           "  Todo 'si' debe terminar con 'fin_si'.\n";
}

inline std::string error_falta_estructura_mientras(int linea) {
    return "\n[ERROR SINTÁCTICO - Línea " + std::to_string(linea) + "]\n"
           "  La estructura 'mientras' está incompleta.\n"
           "  Debe seguir el formato: mientras (condicion) { ... } fin_mientras\n";
}

inline std::string error_falta_fin_mientras(int linea) {
    return "\n[ERROR SINTÁCTICO - Línea " + std::to_string(linea) + "]\n"
           "  Falta 'fin_mientras' para cerrar la estructura 'mientras'.\n"
           "  Todo 'mientras' debe terminar con 'fin_mientras'.\n";
}

inline std::string error_falta_estructura_para(int linea) {
    return "\n[ERROR SINTÁCTICO - Línea " + std::to_string(linea) + "]\n"
           "  La estructura 'para' está incompleta.\n"
           "  Debe seguir el formato: para (inic; cond; incr) { ... } fin_para\n";
}

inline std::string error_falta_fin_para(int linea) {
    return "\n[ERROR SINTÁCTICO - Línea " + std::to_string(linea) + "]\n"
           "  Falta 'fin_para' para cerrar la estructura 'para'.\n"
           "  Todo 'para' debe terminar con 'fin_para'.\n";
}

inline std::string error_falta_estructura_hacer(int linea) {
    return "\n[ERROR SINTÁCTICO - Línea " + std::to_string(linea) + "]\n"
           "  La estructura 'hacer' está incompleta.\n"
           "  Debe seguir el formato: hacer { ... } mientras (cond) fin_mientras\n";
}

inline std::string error_falta_palabra_mientras(int linea) {
    return "\n[ERROR SINTÁCTICO - Línea " + std::to_string(linea) + "]\n"
           "  Falta la palabra 'mientras' después del bloque 'hacer'.\n"
           "  Debe seguir el formato: hacer { ... } mientras (cond) fin_mientras\n";
}

inline std::string error_falta_estructura_elegir(int linea) {
    return "\n[ERROR SINTÁCTICO - Línea " + std::to_string(linea) + "]\n"
           "  La estructura 'elegir' está incompleta.\n"
           "  Debe seguir el formato: elegir (expr) { caso valor: ... parar; defecto: ... }\n";
}

inline std::string error_falta_entonces(int linea) {
    return "\n[ERROR SINTÁCTICO - Línea " + std::to_string(linea) + "]\n"
           "  Falta la palabra 'entonces' en el operador ternario.\n"
           "  Debe seguir el formato: si (cond) entonces v sino f\n";
}

// #incluir mal formado (preprocesador)
inline std::string error_directiva_biblioteca_mal(int linea) {
    return "\n[ERROR SINTÁCTICO - Línea " + std::to_string(linea) + "]\n"
           "  La directiva #incluir está mal escrita.\n"
           "  El formato correcto es: #incluir \"nombre_archivo.txt\"\n";
}

inline std::string error_directiva_mal_formada(int linea) {
    return "\n[ERROR SINTÁCTICO - Línea " + std::to_string(linea) + "]\n"
           "  La directiva #incluir no es correcta.\n"
           "  Debe seguir el formato: #incluir \"nombre_archivo.txt\"\n";
}


// ══════════════════════════════════════════════════════════════════════
//  ERRORES SEMÁNTICOS
//  El orden es correcto, pero lo que intenta hacer el código no tiene
//  sentido lógico o viola las reglas del lenguaje.
// ══════════════════════════════════════════════════════════════════════

// ── Variables ─────────────────────────────────────────────────────

inline std::string error_variable_no_declarada(const std::string& nombre, int linea) {
    return "\n[ERROR SEMÁNTICO - Línea " + std::to_string(linea) + "]\n"
           "  La variable '" + nombre + "' se usó antes de ser creada.\n"
           "  Debes declararla con su tipo antes de usarla.\n";
}

inline std::string error_variable_ya_existe(const std::string& nombre, int linea) {
    return "\n[ERROR SEMÁNTICO - Línea " + std::to_string(linea) + "]\n"
           "  La variable '" + nombre + "' ya fue creada en este ámbito.\n"
           "  No puedes crearla dos veces en el mismo lugar.\n";
}

inline std::string error_variable_no_inicializada(const std::string& nombre, int linea) {
    return "\n[ERROR SEMÁNTICO - Línea " + std::to_string(linea) + "]\n"
           "  La variable '" + nombre + "' no tiene un valor asignado.\n"
           "  Debes inicializarla con '=' al declararla.\n";
}

inline std::string error_puntero_nulo_desreferenciado(const std::string& nombrePtr, int linea) {
    return "\n[ERROR DE PUNTERO - Línea " + std::to_string(linea) + "]\n"
           "  Se intentó desreferenciar el puntero '" + nombrePtr + "' pero su valor es nulo (0).\n"
           "  Asegúrate de inicializar el puntero con la dirección de una variable (ej: " + nombrePtr + " = &variable;) antes de acceder.\n";
}

inline std::string error_puntero_huerfano(const std::string& direccion, int linea) {
    return "\n[ERROR DE MEMORIA - Línea " + std::to_string(linea) + "]\n"
           "  La dirección de memoria '" + direccion + "' ya no es válida o está fuera de alcance.\n"
           "  Esto sucede porque la variable que ocupaba este espacio de memoria ha sido liberada (Stack Frame deallocated).\n";
}

// ── Arreglos ──────────────────────────────────────────────────────

inline std::string error_indice_fuera_rango(int idx, int tamano, int linea) {
    return "\n[ERROR SEMÁNTICO - Línea " + std::to_string(linea) + "]\n"
           "  Índice " + std::to_string(idx) + " fuera del rango del arreglo (tamaño " +
           std::to_string(tamano) + ").\n"
           "  Los índices válidos van desde 0 hasta " + std::to_string(tamano - 1) + ".\n";
}

inline std::string error_arreglo_sin_indice(const std::string& nombre, int linea) {
    return "\n[ERROR SEMÁNTICO - Línea " + std::to_string(linea) + "]\n"
           "  La variable '" + nombre + "' es un arreglo y necesita un índice entre [ ].\n"
           "  Debes usar: " + nombre + "[indice]\n";
}

// ── Funciones ─────────────────────────────────────────────────────

inline std::string error_funcion_ya_existe(const std::string& nombre, int linea) {
    return "\n[ERROR SEMÁNTICO - Línea " + std::to_string(linea) + "]\n"
           "  La función o subrutina '" + nombre + "' ya fue definida previamente.\n"
           "  No puedes definir dos funciones con el mismo nombre.\n";
}

inline std::string error_funcion_no_declarada(const std::string& nombre, int linea) {
    return "\n[ERROR SEMÁNTICO - Línea " + std::to_string(linea) + "]\n"
           "  Estás intentando llamar a la función '" + nombre + "', pero no existe.\n"
           "  Revisa que el nombre esté bien escrito o defínela primero.\n";
}

inline std::string error_argumentos_invalidos(const std::string& nombre, int esperados, int recibidos, int linea) {
    return "\n[ERROR SEMÁNTICO - Línea " + std::to_string(linea) + "]\n"
           "  La función '" + nombre + "' requiere " + std::to_string(esperados) +
           " argumento" + (esperados == 1 ? "" : "s") + ", pero enviaste " +
           std::to_string(recibidos) + ".\n";
}

inline std::string error_retorno_fuera_funcion(int linea) {
    return "\n[ERROR SEMÁNTICO - Línea " + std::to_string(linea) + "]\n"
           "  Encontré 'retornar' fuera de una función.\n"
           "  'retornar' solo puede usarse dentro de una función o subrutina.\n";
}

inline std::string error_retorno_sin_valor(const std::string& funcion, int linea) {
    return "\n[ERROR SEMÁNTICO - Línea " + std::to_string(linea) + "]\n"
           "  La función '" + funcion + "' debe retornar un valor, pero falta la expresión.\n"
           "  Debes escribir: retornar <valor>;\n";
}

inline std::string error_retorno_en_vacio(int linea) {
    return "\n[ERROR SEMÁNTICO - Línea " + std::to_string(linea) + "]\n"
           "  Esta subrutina es 'vacio' y no puede retornar un valor.\n"
           "  Las subrutinas 'vacio' no usan 'retornar' con expresión.\n";
}

// ── Tipos ─────────────────────────────────────────────────────────

inline std::string error_tipos_incompatibles(const std::string& tipo1, const std::string& tipo2, int linea) {
    return "\n[ERROR SEMÁNTICO - Línea " + std::to_string(linea) + "]\n"
           "  Estás intentando mezclar un valor de tipo '" + tipo1 + "' con uno de tipo '" + tipo2 + "'.\n"
           "  Solo puedes operar o asignar valores del mismo tipo entre sí.\n";
}

inline std::string error_tipo_no_valido(const std::string& tipo, int linea) {
    return "\n[ERROR SEMÁNTICO - Línea " + std::to_string(linea) + "]\n"
           "  El tipo '" + tipo + "' no es válido.\n"
           "  Los tipos disponibles son: entero, decimal, cadena, booleano, caracter.\n";
}

// ── Operaciones ───────────────────────────────────────────────────

inline std::string error_division_por_cero(int linea) {
    return "\n[ERROR SEMÁNTICO - Línea " + std::to_string(linea) + "]\n"
           "  Estás intentando dividir entre cero.\n"
           "  Eso no es posible en matemáticas. Revisa el divisor.\n";
}

inline std::string error_modulo_no_entero(int linea) {
    return "\n[ERROR SEMÁNTICO - Línea " + std::to_string(linea) + "]\n"
           "  El operador '%' (módulo) solo funciona con números enteros.\n"
           "  Asegúrate de usar valores de tipo 'entero'.\n";
}

inline std::string error_operacion_no_valida(const std::string& op, const std::string& tipo, int linea) {
    return "\n[ERROR SEMÁNTICO - Línea " + std::to_string(linea) + "]\n"
           "  No se puede aplicar el operador '" + op + "' a un valor de tipo '" + tipo + "'.\n";
}

// ── Flujo (break / continue / return) ─────────────────────────────

inline std::string error_break_fuera_bucle(int linea) {
    return "\n[ERROR SEMÁNTICO - Línea " + std::to_string(linea) + "]\n"
           "  'romper' o 'parar' solo puede usarse dentro de un bucle (mientras, para, hacer)\n"
           "  o dentro de una estructura 'elegir'.\n";
}

inline std::string error_continue_fuera_bucle(int linea) {
    return "\n[ERROR SEMÁNTICO - Línea " + std::to_string(linea) + "]\n"
           "  'continuar' solo puede usarse dentro de un bucle (mientras, para, hacer).\n";
}

// ── Principal ─────────────────────────────────────────────────────

inline std::string error_principal_no_definido(int linea) {
    return "\n[ERROR SEMÁNTICO - Línea " + std::to_string(linea) + "]\n"
           "  No se encontró la función 'Principal'.\n"
           "  El programa necesita un punto de entrada 'Principal() { ... }'.\n";
}

inline std::string error_principal_con_parametros(int linea) {
    return "\n[ERROR SEMÁNTICO - Línea " + std::to_string(linea) + "]\n"
           "  La función 'Principal' no puede tener parámetros.\n"
           "  Debe ser: Principal()\n";
}

// ── Sistema (archivos, etc.) ──────────────────────────────────────

inline std::string error_archivo_no_encontrado(const std::string& nombre, int linea) {
    return "\n[ERROR DE SISTEMA - Línea " + std::to_string(linea) + "]\n"
           "  No se pudo abrir la biblioteca '" + nombre + "'.\n"
           "  Verifica que el archivo exista en la misma carpeta.\n";
}


// ══════════════════════════════════════════════════════════════════════
//  ADVERTENCIAS (No detienen la ejecución)
// ══════════════════════════════════════════════════════════════════════

inline std::string advertencia_variable_no_usada(const std::string& nombre) {
    return "\n[AVISO]\n"
           "  La variable '" + nombre + "' fue creada pero nunca se usó.\n"
           "  Puede que sea un error o simplemente la puedes eliminar.\n";
}

inline std::string advertencia_funcion_no_usada(const std::string& nombre) {
    return "\n[AVISO]\n"
           "  La función '" + nombre + "' fue definida pero nunca se llamó.\n"
           "  Puede que sea un error o simplemente la puedes eliminar.\n";
}

inline std::string advertencia_variable_sombreada(const std::string& nombre, int linea) {
    return "\n[AVISO - Línea " + std::to_string(linea) + "]\n"
           "  La variable '" + nombre + "' oculta a otra variable con el mismo nombre\n"
           "  de un ámbito exterior. Considera usar otro nombre.\n";
}

inline std::string advertencia_biblioteca_no_encontrada(const std::string& nombre, int linea) {
    return "\n[AVISO - Línea " + std::to_string(linea) + "]\n"
           "  La biblioteca '" + nombre + "' no se encontró.\n"
           "  La línea #incluir será ignorada. Si el programa usa funciones de esta\n"
           "  biblioteca, la compilación fallará más adelante.\n";
}


// ══════════════════════════════════════════════════════════════════════
//  MENSAJES DE ÉXITO
// ══════════════════════════════════════════════════════════════════════

inline std::string exito_compilacion() {
    return "\n✓ Compilación exitosa. No se encontraron errores.\n";
}

inline std::string exito_ejecucion(const std::string& resultado) {
    return "\n✓ Resultado: " + resultado + "\n";
}
