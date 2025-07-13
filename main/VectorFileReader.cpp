#include <fstream>             // Para std::ifstream
#include <sstream>             // Para std::stringstream
#include <iostream>            // Para std::cerr, std::cout
#include <vector>              // Para std::vector
#include <string>              // Para std::string
#include <cstdio>              // Para snprintf
#include <optional>            // Para std::optional (C++17)

#include "VectorFileReader.hpp"
#include "complex_object.h"

// --- Implementação do Construtor ---
VectorFileReader::VectorFileReader() : numLines(0), numElements(-1) {
    // Inicializa os contadores. numElements = -1 indica que o tamanho ainda não foi definido.
}


std::optional<VectorEntry> VectorFileReader::parseLine(const std::string& line, int line_number) const {
     // Ignora linhas vazias ou que contenham apenas espaços em branco (verificação extra)
    if (line.empty() || line.find_first_not_of(" \t\n\v\f\r") == std::string::npos) {
        return std::nullopt; // Linha vazia/whitespace, não é um erro, apenas ignora
    }

    std::stringstream ss(line);
    std::string tempLabel;
    int tempResolution;

    // Tenta ler o nome (string) e a resolução (int) da linha
    if (!(ss >> tempLabel >> tempResolution)) {
        // Formato inicial incorreto
        std::cerr << "Aviso: Linha " << line_number << " mal formatada (label/resolution). Ignorando." << std::endl;
        return std::nullopt; // Falha no parsing inicial
    }

    // Cria uma nova entrada temporária
    VectorEntry entry;
    entry.resolution = tempResolution;
    snprintf(entry.label, sizeof(entry.label), "%s", tempLabel.c_str());

    // Lê os valores double restantes na linha
    double dataValue;
    while (ss >> dataValue) {
        entry.data.push_back(dataValue);
    }

    // Verifica se ocorreu algum erro *depois* de ler os doubles
    if (!ss.eof() && ss.fail()) {
        // Encontrou algo não numérico após os doubles.
        std::cerr << "Aviso: Linha " << line_number << " ignorada. Encontrado dado não numérico após os doubles." << std::endl;
        return std::nullopt; // Falha no parsing dos dados
    }

    // Se chegou até aqui, o parsing foi bem-sucedido
    return entry; // Retorna o VectorEntry dentro de um optional
}

bool VectorFileReader::checkSizeAndAdd(VectorEntry&& entry, const std::string& line, int line_number) {
    // Obtém o tamanho do vetor de dados da entrada atual
    size_t current_data_size = entry.data.size();

    if (this->numElements == -1) {
        // É a primeira linha válida encontrada. Define o tamanho esperado (numElements).
        this->numElements = static_cast<int>(current_data_size); // Assume que size_t cabe em int
        // Adiciona a primeira entrada válida ao vetor principal
        this->vectors.push_back(std::move(entry)); // Move a entrada para o vetor
        return true; // Sucesso (primeira entrada)
    } else {
        // Já temos um tamanho esperado (numElements). Compara com o tamanho atual.
        if (static_cast<int>(current_data_size) != this->numElements) {
            // Erro: Tamanho inconsistente!
            std::cerr << "\nErro Crítico: Tamanho de dados inconsistente." << std::endl;
            std::cerr << "  Linha " << line_number << ": Encontrado " << current_data_size << " elementos de dados." << std::endl;
            std::cerr << "  Esperado (com base na primeira linha válida): " << this->numElements << " elementos." << std::endl;
            std::cerr << "  Conteúdo da linha " << line_number << ": \"" << line << "\"" << std::endl;
            return false; // Falha (inconsistência)
        } else {
            // Tamanho consistente. Adiciona a entrada ao vetor principal.
            this->vectors.push_back(std::move(entry)); // Move a entrada para o vetor
            return true; // Sucesso (consistente)
        }
    }
}


bool VectorFileReader::loadFromFile(const std::string& filename) {
    std::ifstream inputFile(filename);
    if (!inputFile) {
        std::cerr << "Erro ao abrir o arquivo: " << filename << std::endl;
        return false;
    }

    // Limpa/Reseta o estado antes de carregar novos dados
    this->vectors.clear();
    this->numLines = 0;
    this->numElements = -1; // -1 indica que o tamanho ainda não foi determinado

    std::string line;
    int line_number = 0;

    while (std::getline(inputFile, line)) {
        line_number++;

        // Tenta parsear a linha usando a função auxiliar
        std::optional<VectorEntry> parsed_entry = parseLine(line, line_number);

        // Verifica se o parsing foi bem-sucedido
        if (parsed_entry.has_value()) {
            // Se sim, verifica a consistência do tamanho e tenta adicionar
            // Passamos a entrada usando std::move pois ela não será mais usada aqui
            // se checkSizeAndAdd retornar true.
            if (!checkSizeAndAdd(std::move(parsed_entry.value()), line, line_number)) {
                // Erro de inconsistência detectado por checkSizeAndAdd
                // Mensagem de erro já foi impressa pela função auxiliar.
                this->vectors.clear();    // Limpa o vetor
                this->numLines = 0;       // Reseta contadores
                this->numElements = -1;
                inputFile.close();      // Fecha o arquivo
                std::cerr << "  Abortando leitura. Nenhum dado foi carregado devido à inconsistência." << std::endl;
                return false;           // Retorna falha
            }
            // Se checkSizeAndAdd retornou true, a entrada foi adicionada com sucesso.
        }
        // Se parseLine retornou std::nullopt, a linha foi ignorada (vazia, erro formato, etc.)
        // A mensagem de aviso já foi impressa por parseLine (se aplicável). Continua para próxima linha.

    } // Fim do while(getline)

    inputFile.close();

    // Atualiza o número de linhas válidas carregadas
    this->numLines = static_cast<int>(this->vectors.size());

    // Se nenhuma linha válida foi carregada, numElements ainda será -1.
    if (this->numLines == 0) {
        this->numElements = 0;
    }

    // Mensagem final de sucesso ou aviso
    if (this->numLines == 0) {
         std::cout << "Aviso: Arquivo '" << filename << "' lido, mas nenhuma entrada de dados válida foi encontrada ou carregada." << std::endl;
    }

    return true; // Leitura concluída sem erro crítico de inconsistência
}


// Métodos de acesso
const std::vector<VectorEntry>& VectorFileReader::getVectors() const { return vectors; }

int VectorFileReader::getNumLines() const {
    return this->numLines;
}

int VectorFileReader::getNumElements() const {
    // Retorna 0 se nenhuma linha foi carregada, ou o número de elementos por linha caso contrário.
    return (this->numElements == -1) ? 0 : this->numElements;
}


void VectorFileReader::displayVectors() const {
    // (Implementação de displayVectors como na resposta anterior)
     if (vectors.empty()) {
        std::cout << "Nenhum vetor carregado para exibir." << std::endl;
        return;
    }
     std::cout << "--- Exibindo Vetores e Metadados Carregados ---" << std::endl;
    std::cout << "Total de entradas: " << this->numLines << " | Elementos por vetor: " << this->numElements << std::endl;
    std::cout << "---------------------------------------------" << std::endl;
     for (const auto& entry : vectors) {
        std::cout << "Label:      \"" << entry.label << "\"" << std::endl;
        std::cout << "Resolution: " << entry.resolution << std::endl;
        std::cout << "Data:       [";
        bool first = true;
        for (double val : entry.data) {
            if (!first) {
                std::cout << ", ";
            }
            std::cout << val;
            first = false;
        }
        std::cout << "]" << std::endl;
        std::cout << "---------------------------------------------" << std::endl;
    }
}

/**
 * @brief Cria e retorna um vetor de objetos TComplexObject a partir dos dados carregados.
 *
 * Itera sobre os VectorEntry armazenados internamente e instancia um TComplexObject
 * para cada entrada, usando o label, resolution e o vetor de dados correspondente.
 *
 * @return std::vector<TComplexObject> Um vetor contendo os objetos TComplexObject criados.
 * Retorna um vetor vazio se nenhum dado foi carregado previamente.
 */
 std::vector<TComplexObject> VectorFileReader::createVectorTComplexObject() const {
    std::vector<TComplexObject> complexObjects; // Vetor para armazenar os resultados

    // Opcional: Adiciona um aviso se tentar criar objetos sem ter carregado dados
    if (vectors.empty()) {
        if (numElements == -1) { // Verifica se loadFromFile foi chamado alguma vez
             std::cerr << "Aviso: Tentando criar TComplexObjects sem ter carregado dados do arquivo primeiro." << std::endl;
        } else {
            // loadFromFile foi chamado, mas não encontrou linhas válidas. Isso é ok.
             // std::cout << "Info: Nenhum dado válido carregado, retornando vetor de TComplexObjects vazio." << std::endl;
        }
        return complexObjects; // Retorna vetor vazio
    }

    // Otimização: Reserva espaço no vetor para evitar realocações múltiplas
    complexObjects.reserve(vectors.size());

    // Itera sobre cada VectorEntry armazenado na classe
    for (const auto& entry : this->vectors) {
        // Converte o label char[] para std::string para o construtor de TComplexObject
        std::string labelStr(entry.label);

        // Cria o TComplexObject usando os dados da entry atual
        // e o adiciona ao vetor de resultados usando emplace_back (eficiente)
        complexObjects.emplace_back(labelStr, entry.resolution, entry.data);
    }

    // Retorna o vetor preenchido com os objetos TComplexObject
    return std::move(complexObjects);
}