#include <string>
#include <sstream>
#include <fstream>
#include <regex>
#include <stdexcept>
#include "errors.hpp"

// Versión completa del preprocesador (usada tanto en consola como en GUI)
std::string preprocesarBibliotecas(const std::string& codigoFuente) {
    std::stringstream ss(codigoFuente);
    std::string lineaTexto;
    std::string codigoCompleto = "";
    int numeroLinea = 1;

    std::regex regexIncluir(R"(^\s*#incluir\s+\"([^\"]+)\"\s*$)");
    std::smatch match;

    while (std::getline(ss, lineaTexto)) {
        if (std::regex_search(lineaTexto, match, regexIncluir)) {
            std::string nombreArchivo = match[1].str();

            std::ifstream archivoBiblioteca(nombreArchivo);
            if (!archivoBiblioteca.is_open()) {
                throw std::runtime_error(error_archivo_no_encontrado(nombreArchivo, numeroLinea));
            }

            std::stringstream contenidoBiblioteca;
            contenidoBiblioteca << archivoBiblioteca.rdbuf();
            archivoBiblioteca.close();

            // Recursivo: la biblioteca podría tener sus propios #incluir
            codigoCompleto += preprocesarBibliotecas(contenidoBiblioteca.str()) + "\n";
        } else if (lineaTexto.find("#incluir") != std::string::npos) {
            throw std::runtime_error(error_directiva_biblioteca_mal(numeroLinea));
        } else {
            codigoCompleto += lineaTexto + "\n";
        }
        numeroLinea++;
    }
    return codigoCompleto;
}

