#pragma once
#include <string>
#include <vector>

std::string preprocesarBibliotecas(const std::string& codigoFuente);
void preprocesarBibliotecas(const std::string& codigoFuente,
                            std::string& resultado,
                            std::vector<int>& mapaExpToOrig);
