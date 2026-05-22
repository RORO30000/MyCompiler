#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include "lexer.hpp"
#include "errors.hpp"
#include "semantic.hpp"

using namespace std;

// Declaradas en lexer.cpp y parser.cpp
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
        // Ahora parsear() ejecuta las sentencias internamente (p.ej. imprime con mostrar)
        parsear(tokens); 
        cout << "\n✓ Ejecución finalizada sin errores críticos.\n";
    } catch (const exception& e) {
        cout << e.what();
    }
    cout << "==================================================\n";
}

int main() {
    // 1. Prueba Estructurada Válida (Bucle mientras + Condicional si/sino)
    // Nota: Adaptado estrictamente a las palabras clave de tu parser.cpp (fin_mientras, fin_si)
    string programa_gramatical = 
        "numero x = 1;\n"
        "numero maximo = 4;\n"
        "mientras (x < maximo)\n"
        "    mostrar(x);\n"
        "    x = x + 1;\n"
        "fin_mientras\n"
        "si (x == maximo)\n"
        "    mostrar(999);\n"
        "sino\n"
        "    mostrar(111);\n"
        "fin_si\n";

    compilarYEjecutar("Programa Estructurado Válido", programa_gramatical);

    // 2. Prueba de Error Semántico: Variable no declarada
    string error_semantico_1 = 
        "y = 10;\n";
    compilarYEjecutar("Error Semántico - Variable No Declarada", error_semantico_1);

    // 3. Prueba de Error Semántico: Redeclaración de variable
    string error_semantico_2 = 
        "numero a = 5;\n"
        "numero a = 10;\n";
    compilarYEjecutar("Error Semántico - Duplicado", error_semantico_2);

    // 4. Prueba de Advertencia: Variable no usada (Activará tu revisarNoUsadas())
    string aviso_no_usada = 
        "numero variableInutil = 42;\n"
        "mostrar(10);\n";
    compilarYEjecutar("Aviso - Variable Creada pero No Usada", aviso_no_usada);

    // 5. Prueba de Error Sintáctico: Estructura de control mal cerrada
    string error_sintactico = 
        "si (1 == 1)\n"
        "    mostrar(5);\n"
        "/* Falta el fin_si reglamentario */\n";
    compilarYEjecutar("Error Sintáctico - Estructura Abierta", error_sintactico);
    
    // 6. Prueba de Error Léxico: Caracteres no reconocidos o tokens mal formados
    string error_lexico = 
        "numero x = 5 @;\n"          // El caracter '@' no pertenece al lenguaje
        "numero y = 3.14.15;\n"      // Un número con dos puntos decimales está mal formado
        "numero $falsa_var = 10;\n"; // El símbolo '$' no es válido para identificadores
    compilarYEjecutar("Error Léxico - Caracteres Inválidos", error_lexico);

    // 7. Prueba de Error Léxico: Cadenas de texto (Strings) sin cerrar
    string error_lexico_string =
        "texto mensaje = \"Hola, esto es una prueba;\n" // Falta la comilla de cierre "
        "mostrar(mensaje);\n";
    compilarYEjecutar("Error Léxico - String Literal Abierto", error_lexico_string);
    
    // 8. Prueba de Error Léxico: Comentarios de bloque sin cerrar (EOF inesperado)
    string error_lexico_comentario = 
        "numero a = 5;\n"
        "/* Inicia un comentario largo\n"
        "   pero el programador olvidó cerrarlo...\n"
        "numero b = 10;\n"; // El lexer se traga todo el código buscando el "*/" hasta el fin del archivo
    compilarYEjecutar("Error Léxico - Comentario de Bloque Abierto", error_lexico_comentario);
    
    string nuevo = 
        "numero a = 4;\n"
        "numero b = 5;\n"
        "numero c = 6;\n"
        "a + b * c";
    compilarYEjecutar(" Prueba simple para ;", nuevo);




    return 0;
}

