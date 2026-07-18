#include <string>
#include <sstream>
#include <fstream>
#include <regex>
#include <stdexcept>
#include <vector>
#include "core/preprocesador.hpp"
#include "core/errors.hpp"

// Forward-declare the overload
void preprocesarBibliotecas(const std::string&, std::string&, std::vector<int>&);

std::string preprocesarBibliotecas(const std::string& codigoFuente) {
    std::string res;
    std::vector<int> ignored;
    preprocesarBibliotecas(codigoFuente, res, ignored);
    return res;
}

// Sobrecarga que además llena mapaExpToOrig:
//   mapaExpToOrig[k] = 0          → la línea expandida k pertenece a una librería
//   mapaExpToOrig[k] = origLine   → la línea expandida k corresponde a la línea origLine del fuente original
void preprocesarBibliotecas(const std::string& codigoFuente,
                            std::string& resultado,
                            std::vector<int>& mapaExpToOrig) {
    std::stringstream ss(codigoFuente);
    std::string lineaTexto;
    resultado = "";
    mapaExpToOrig.clear();
    mapaExpToOrig.push_back(0); // índice 0 no usado (1-indexed)
    int numeroLinea = 1;

    std::regex regexIncluir(R"(^\s*#incluir\s+\"([^\"]+)\"\s*$)");
    std::smatch match;

    while (std::getline(ss, lineaTexto)) {
        if (std::regex_search(lineaTexto, match, regexIncluir)) {
            std::string nombreArchivo = match[1].str();

            std::ifstream archivoBiblioteca;
            std::string rutas[] = {nombreArchivo, "librerias/" + nombreArchivo};
            for (auto& r : rutas) {
                archivoBiblioteca.open(r);
                if (archivoBiblioteca.is_open()) break;
            }
            if (!archivoBiblioteca.is_open()) {
                throw std::runtime_error(error_archivo_no_encontrado(nombreArchivo, numeroLinea));
            }

            std::stringstream contenidoBiblioteca;
            contenidoBiblioteca << archivoBiblioteca.rdbuf();
            archivoBiblioteca.close();

            // Recursivo: la biblioteca podría tener sus propios #incluir
            std::string libExp;
            std::vector<int> libMap;
            preprocesarBibliotecas(contenidoBiblioteca.str(), libExp, libMap);

            // Las líneas de la biblioteca son externas → marcar como 0
            int libLines = 0;
            for (char c : libExp)
                if (c == '\n') libLines++;
            for (int i = 0; i < libLines; i++)
                mapaExpToOrig.push_back(0);

            resultado += libExp;
            if (!libExp.empty() && libExp.back() != '\n')
                resultado += '\n';
        } else if (lineaTexto.find("#incluir") != std::string::npos) {
            throw std::runtime_error(error_directiva_biblioteca_mal(numeroLinea));
        } else {
            resultado += lineaTexto + "\n";
            mapaExpToOrig.push_back(numeroLinea);
        }
        numeroLinea++;
    }
}
