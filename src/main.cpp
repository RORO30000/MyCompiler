#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include "core/lexer.hpp"
#include "core/errors.hpp"
#include "core/semantic.hpp"
#include "core/eventos.hpp"
#include "core/preprocesador.hpp"

using namespace std;

vector<Token> tokenizar(const string& fuente);
double parsear(vector<Token>& tokens, vector<EventoPaso>* cola = nullptr);

void compilarYEjecutar(const string& titulo, const string& codigo) {
    cout << "\n==================================================\n";
    cout << " PRUEBA: " << titulo << "\n";
    cout << "==================================================\n";
    cout << "Codigo Fuente Original:\n" << codigo << "\n";
    cout << "--------------------------------------------------\n";
    cout << "Salida / Resultado:\n";

    try {
        string codigoExpandido = preprocesarBibliotecas(codigo);
        auto tokens = tokenizar(codigoExpandido);
        parsear(tokens);
        cout << "\n✓ Fin de la seccion de pruebas sin caidas criticas.\n";
    } catch (const exception& e) {
        cout << e.what();
    }
    cout << "==================================================\n";
}

int main() {
    // ── Pruebas con la nueva gramática ───────────────────────────

    // 1. si / sino si / sino / fin_si
    string prueba_sino_si =
        "entero x = 15;\n"
        "Principal() {\n"
        "    si (x > 20) {\n"
        "        mostrar(\"x > 20\");\n"
        "    } sino si (x > 10) {\n"
        "        mostrar(\"x > 10\");\n"
        "    } sino {\n"
        "        mostrar(\"x <= 10\");\n"
        "    }\n"
        "    fin_si\n"
        "}\n";
    compilarYEjecutar("1. si / sino si / sino / fin_si", prueba_sino_si);

    // 2. Principal() sin void, para/fin_para, ++
    string prueba_para =
        "Principal() {\n"
        "    para (entero i = 1; i <= 5; i++) {\n"
        "        mostrar(i);\n"
        "    }\n"
        "    fin_para\n"
        "}\n";
    compilarYEjecutar("2. para / fin_para con ++", prueba_para);

    // 3. Operadores compuestos +=, -=, *=
    string prueba_compuestos =
        "Principal() {\n"
        "    entero a = 10;\n"
        "    a += 5;\n"
        "    mostrar(a);\n"
        "    a -= 3;\n"
        "    mostrar(a);\n"
        "    a *= 2;\n"
        "    mostrar(a);\n"
        "}\n";
    compilarYEjecutar("3. += -= *=", prueba_compuestos);

    // 4. Operadores lógicos &&, ||, !
    string prueba_logicos =
        "Principal() {\n"
        "    entero a = 5;\n"
        "    entero b = 10;\n"
        "    si (a > 0 && b > 0) {\n"
        "        mostrar(\"ambos positivos\");\n"
        "    }\n"
        "    fin_si\n"
        "}\n";
    compilarYEjecutar("4. && logico", prueba_logicos);

    // 5. Cadena tipo
    string prueba_cadena =
        "Principal() {\n"
        "    cadena saludo = \"Hola Mundo\";\n"
        "    mostrar(saludo);\n"
        "}\n";
    compilarYEjecutar("5. Tipo cadena", prueba_cadena);

    // 6. Entero y Decimal
    string prueba_decimal =
        "Principal() {\n"
        "    entero ent = 42;\n"
        "    decimal dec = 3.14;\n"
        "    mostrar(ent);\n"
        "    mostrar(dec);\n"
        "}\n";
    compilarYEjecutar("6. Entero y Decimal", prueba_decimal);

    // 7. -- decremento
    string prueba_decremento =
        "Principal() {\n"
        "    entero c = 5;\n"
        "    c--;\n"
        "    mostrar(c);\n"
        "}\n";
    compilarYEjecutar("7. Decremento --", prueba_decremento);

    // 8. Si simple
    string prueba_si_simple =
        "Principal() {\n"
        "    entero x = 5;\n"
        "    si (x == 5) {\n"
        "        mostrar(\"es cinco\");\n"
        "    }\n"
        "    fin_si\n"
        "}\n";
    compilarYEjecutar("8. si simple", prueba_si_simple);

    // 9. Mientras con operador lógico
    string prueba_mientras_logico =
        "Principal() {\n"
        "    entero i = 0;\n"
        "    mientras (i < 3) {\n"
        "        mostrar(i);\n"
        "        i++;\n"
        "    }\n"
        "    fin_mientras\n"
        "}\n";
    compilarYEjecutar("9. mientras con ++", prueba_mientras_logico);

    // 10. Funcion con tipo de retorno (C++ style)
    string prueba_func_retorno =
        "entero doble(entero n) {\n"
        "    retornar n * 2;\n"
        "}\n"
        "Principal() {\n"
        "    entero r = doble(5);\n"
        "    mostrar(r);\n"
        "}\n";
    compilarYEjecutar("10. Funcion con tipo de retorno (C++ style)", prueba_func_retorno);

    // 11. break en mientras
    string prueba_break =
        "Principal() {\n"
        "    entero i = 0;\n"
        "    mientras (i < 10) {\n"
        "        si (i == 3) {\n"
        "            romper;\n"
        "        }\n"
        "        fin_si\n"
        "        mostrar(i);\n"
        "        i++;\n"
        "    }\n"
        "    fin_mientras\n"
        "}\n";
    compilarYEjecutar("11. break en mientras", prueba_break);

    // 12. continue en mientras
    string prueba_continue =
        "Principal() {\n"
        "    entero i = 0;\n"
        "    mientras (i < 5) {\n"
        "        i++;\n"
        "        si (i == 3) {\n"
        "            continuar;\n"
        "        }\n"
        "        fin_si\n"
        "        mostrar(i);\n"
        "    }\n"
        "    fin_mientras\n"
        "}\n";
    compilarYEjecutar("12. continue en mientras", prueba_continue);

    // 13. break en para
    string prueba_break_para =
        "Principal() {\n"
        "    para (entero i = 0; i < 10; i++) {\n"
        "        si (i == 4) {\n"
        "            romper;\n"
        "        }\n"
        "        fin_si\n"
        "        mostrar(i);\n"
        "    }\n"
        "    fin_para\n"
        "}\n";
    compilarYEjecutar("13. break en para", prueba_break_para);

    // 14. continue en para
    string prueba_continue_para =
        "Principal() {\n"
        "    para (entero i = 0; i < 6; i++) {\n"
        "        si (i % 2 == 0) {\n"
        "            continuar;\n"
        "        }\n"
        "        fin_si\n"
        "        mostrar(i);\n"
        "    }\n"
        "    fin_para\n"
        "}\n";
    compilarYEjecutar("14. continue en para (impares)", prueba_continue_para);

    // 15. Operador 'no' como alias de !
    string prueba_no =
        "booleano activo = falso;\n"
        "Principal() {\n"
        "    si (no activo) {\n"
        "        mostrar(\"no activo es verdadero\");\n"
        "    }\n"
        "    fin_si\n"
        "}\n";
    compilarYEjecutar("15. operador 'no' como !", prueba_no);

    // 16. Doble negacion con 'no'
    string prueba_doble_no =
        "booleano flag = verdadero;\n"
        "Principal() {\n"
        "    si (no no flag) {\n"
        "        mostrar(\"doble no es verdadero\");\n"
        "    }\n"
        "    fin_si\n"
        "}\n";
    compilarYEjecutar("16. doble negacion 'no no'", prueba_doble_no);

    // 17. Concatenacion de cadenas con +
    string prueba_concat =
        "Principal() {\n"
        "    cadena saludo = \"Hola\";\n"
        "    cadena mensaje = saludo + \" Mundo\";\n"
        "    mostrar(mensaje);\n"
        "}\n";
    compilarYEjecutar("17. concatenacion de cadenas con +", prueba_concat);

    // 18. hacer-mientras (do-while)
    string prueba_hacer_mientras =
        "Principal() {\n"
        "    entero i = 0;\n"
        "    hacer {\n"
        "        mostrar(i);\n"
        "        i++;\n"
        "    } mientras (i < 3) fin_mientras\n"
        "}\n";
    compilarYEjecutar("18. hacer-mientras (do-while)", prueba_hacer_mientras);

    // 19. Ternario si ... entonces ... sino
    string prueba_ternario =
        "Principal() {\n"
        "    entero a = 10;\n"
        "    entero b = 20;\n"
        "    entero max = si (a > b) entonces a sino b;\n"
        "    mostrar(max);\n"
        "}\n";
    compilarYEjecutar("19. ternario si...entonces...sino", prueba_ternario);

    // 20. Ternario con cadenas
    string prueba_ternario_cadena =
        "Principal() {\n"
        "    entero edad = 15;\n"
        "    cadena msg = si (edad >= 18) entonces \"Mayor\" sino \"Menor\";\n"
        "    mostrar(msg);\n"
        "}\n";
    compilarYEjecutar("20. ternario con cadenas", prueba_ternario_cadena);

    // 21. elegir / caso / defecto (switch)
    string prueba_elegir =
        "Principal() {\n"
        "    entero x = 2;\n"
        "    elegir (x) {\n"
        "        caso 1: mostrar(\"uno\"); parar;\n"
        "        caso 2: mostrar(\"dos\"); parar;\n"
        "        defecto: mostrar(\"otro\");\n"
        "    }\n"
        "}\n";
    compilarYEjecutar("21. elegir/caso/defecto", prueba_elegir);

    // 22. elegir con defecto
    string prueba_elegir_defecto =
        "Principal() {\n"
        "    entero x = 99;\n"
        "    elegir (x) {\n"
        "        caso 1: mostrar(\"uno\"); parar;\n"
        "        caso 2: mostrar(\"dos\"); parar;\n"
        "        defecto: mostrar(\"desconocido\");\n"
        "    }\n"
        "}\n";
    compilarYEjecutar("22. elegir con defecto", prueba_elegir_defecto);

    return 0;
}
