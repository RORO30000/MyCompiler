#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include "lexer.hpp"
#include "errors.hpp"
#include "semantic.hpp"

using namespace std;

// Prototipos declarados en lexer.cpp y parser.cpp
vector<Token> tokenizar(const string& fuente);
double parsear(vector<Token>& tokens);

void compilarYEjecutar(const string& titulo, const string& codigo) {
    cout << "\n==================================================\n";
    cout << " PRUEBA: " << titulo << "\n";
    cout << "==================================================\n";
    cout << "Código Fuente:\n" << codigo << "\n";
    cout << "--------------------------------------------------\n";
    cout << "Salida / Resultado:\n";
    
    try {
        auto tokens = tokenizar(codigo);
        // El parser procesa las declaraciones y despacha 'principal()' si existe
        parsear(tokens); 
        cout << "\n✓ Fin de la sección de pruebas sin caídas críticas.\n";
    } catch (const exception& e) {
        cout << e.what();
    }
    cout << "==================================================\n";
}

int main() {
    // ----------------------------------------------------------------------
    // PROBANDO CARACTERÍSTICAS CORRECTAS / EXITOSAS
    // ----------------------------------------------------------------------

    // 1. Programa Completo Estructurado con Llamadas a Funciones y Ámbitos
    string prueba_funciones_exito = 
        "funcion calcularSuma(numero a, numero b) retornar numero {\n"
        "    retornar a + b;\n"
        "}\n"
        "\n"
        "vacio principal() {\n"
        "    numero x = 10;\n"
        "    numero y = 20;\n"
        "    numero resultado = calcularSuma(x, y);\n"
        "    mostrar(resultado);\n"
        "}\n";
    compilarYEjecutar("1. Declaración y Llamada de Funciones (Éxito)", prueba_funciones_exito);

    // 2. Comprobando los nuevos tipos Booleanos, Caracteres y Condicionales Alfanuméricos
    string prueba_tipos_nuevos = 
        "vacio principal() {\n"
        "    booleano bandera = verdadero;\n"
        "    caracter opcion = 'A';\n"
        "    si (bandera == verdadero)\n"
        "        mostrar(\"El estado es verdadero de forma correcta\");\n"
        "        mostrar(opcion);\n"
        "    fin_si\n"
        "}\n";
    compilarYEjecutar("2. Uso de Booleanos y Caracteres (Éxito)", prueba_tipos_nuevos);

    // 3. Ejecución Secuencial Clásica (Sin función principal)
    string prueba_secuencial = 
        "numero a = 5;\n"
        "numero b = 15;\n"
        "mostrar(a * b);\n";
    compilarYEjecutar("3. Modo de Ejecución Secuencial Estándar", prueba_secuencial);


    // ----------------------------------------------------------------------
    // PROBANDO ERRORES LÉXICOS (NUEVOS Y ANTIGUOS)
    // ----------------------------------------------------------------------

    // 4. Error Léxico - Comentario en bloque abierto (EOF Inesperado)
    string error_lexico_comentario = 
        "numero x = 10;\n"
        "/* Esto es un comentario que inicia bien\n"
        "   pero al programador se le olvidó poner el cierre de bloque\n"
        "numero y = 20;\n";
    compilarYEjecutar("4. Error Léxico - Comentario Abierto", error_lexico_comentario);

    // 5. Error Léxico - Formatos Numéricos Letras O Símbolos Inválidos
    string error_lexico_caracter = 
        "vacio principal() {\n"
        "    numero val = 45.12.3;\n" // Número mal formado
        "    numero @invalido = 7;\n"  // Carácter '@' no pertenece al lenguaje
        "}\n";
    compilarYEjecutar("5. Error Léxico - Mal Formato Dinámico", error_lexico_caracter);


    // ----------------------------------------------------------------------
    // PROBANDO ERRORES SINTÁCTICOS
    // ----------------------------------------------------------------------

    // 6. Error Sintáctico - Parentización Desbalanceada en Expresiones
    string error_sintactico_parentesis = 
        "vacio principal() {\n"
        "    numero calculo = (5 + 3 * 2;\n" // Falta cerrar )
        "}\n";
    compilarYEjecutar("6. Error Sintáctico - Paréntesis Faltante", error_sintactico_parentesis);

    // 7. Error Sintáctico - Estructuras de Bloques o Llaves Mal Cerradas
    string error_sintactico_llaves = 
        "funcion prueba() retornar numero {\n"
        "    retornar 1;\n"
        "// Falta cerrar la llave de la función aquí \n";
    compilarYEjecutar("7. Error Sintáctico - Llaves Abiertas de Subrutina", error_sintactico_llaves);


    // ----------------------------------------------------------------------
    // PROBANDO ERRORES SEMÁNTICOS (Aislamiento de Ámbitos y Firmas)
    // ----------------------------------------------------------------------

    // 8. Error Semántico - Intento de Acceso a Variables Locales de otra Función (Scope)
    string error_semantico_ambito = 
        "funcion externa() retornar numero {\n"
        "    numero variableOculta = 99;\n"
        "    retornar 0;\n"
        "}\n"
        "\n"
        "vacio principal() {\n"
        "    externa();\n"
        "    mostrar(variableOculta);\n" // ERROR: No existe en el ámbito de principal
        "}\n";
    compilarYEjecutar("8. Error Semántico - Violación de Ámbito (Scope)", error_semantico_ambito);

    // 9. Error Semántico - Duplicación y Colisión de Firmas de Funciones
    string error_semantico_duplicado = 
        "vacio miFuncion() {\n"
        "    mostrar(1);\n"
        "}\n"
        "\n"
        "funcion miFuncion(numero a) retornar numero {\n" // ERROR: Ya existe 'miFuncion'
        "    retornar a;\n"
        "}\n";
    compilarYEjecutar("9. Error Semántico - Redefinición de Funciones", error_semantico_duplicado);

    // 10. Error Semántico - Argumentos Inválidos en Conteo de Parámetros
    string error_semantico_argumentos = 
        "funcion procesar(numero a, numero b) retornar numero {\n"
        "    retornar a * b;\n"
        "}\n"
        "\n"
        "vacio principal() {\n"
        "    numero test = procesar(5);\n" // ERROR: Falta el segundo parámetro
        "}\n";
    compilarYEjecutar("10. Error Semántico - Desajuste de Argumentos", error_semantico_argumentos);


    // ----------------------------------------------------------------------
    // ADVERTENCIAS Y AVISOS DE LIMPIEZA DE CÓDIGO
    // ----------------------------------------------------------------------
    
    // Extra: Prueba de Variables Creadas pero Nunca Usadas
    string aviso_limpieza = 
        "numero globalInutil = 100;\n"
        "vacio principal() {\n"
        "    numero localInutil = 200;\n"
        "    mostrar(\"Hola Mundo limpio\");\n"
        "}\n";
    compilarYEjecutar("EXTRA: Verificación de Advertencias por Variables No Usadas", aviso_limpieza);

    return 0;
}
