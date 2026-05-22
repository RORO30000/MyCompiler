#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include "lexer.hpp"
#include "errors.hpp"
#include "semantic.hpp"

// Declaradas en lexer.cpp y parser.cpp
std::vector<Token> tokenizar(const std::string& fuente);
double parsear(std::vector<Token>& tokens);

void compilar(const std::string& codigo) {
    std::cout << "\n──────────────────────────────\n";
    std::cout << "Código: " << codigo << "\n";
    std::cout << "──────────────────────────────\n";
    try {
        auto tokens   = tokenizar(codigo);
        double result = parsear(tokens);
        std::cout << exito_ejecucion(std::to_string(result));
    } catch (const std::exception& e) {
        std::cout << e.what();
    }
}

int main() {
    compilar("3 + (4 * 2)");        // ✓ correcto → 11
    compilar("10 / 0");             // ✗ división por cero
    compilar("5 + @3");             // ✗ carácter inválido
    compilar("(2 + 3");             // ✗ paréntesis sin cerrar
    compilar("100 - 25 * 3 + 7");   // ✓ correcto → 32
    
    compilar("Hola mundo");
   



    return 0;
}
