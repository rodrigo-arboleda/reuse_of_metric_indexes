#ifndef VECTOR_FILE_READER_HPP
#define VECTOR_FILE_READER_HPP

// #include "VectorData.hpp"
#include <vector>
#include <string>
#include <optional>

#include "complex_object.h"


// Struct para armazenar cada vetor com metadados
struct VectorEntry {
    char label[200]; // string de tamanho fixo
    int resolution;
    std::vector<double> data;
};


class VectorFileReader {
private:
    int numLines;                             // Número de linhas (vetores)
    int numElements;                          // Número de elementos em cada vetor
    std::vector<VectorEntry> vectors;       // Vetores com inforcao das imagens


    // Funções auxiliares privadas
    /**
     * @brief Tenta parsear uma única linha do arquivo para um VectorEntry.
     * @param line A string contendo a linha a ser parseada.
     * @param line_number O número da linha no arquivo (para mensagens de erro).
     * @return Um std::optional<VectorEntry> contendo a entrada parseada em caso de sucesso,
     * ou std::nullopt em caso de falha no parsing ou formato inválido.
     */
     std::optional<VectorEntry> parseLine(const std::string& line, int line_number) const; // Pode ser const

     /**
      * @brief Verifica a consistência do tamanho do vetor de dados e adiciona a entrada.
      * @param entry O VectorEntry parseado a ser verificado e potencialmente adicionado.
      * @param line A string original da linha (para mensagens de erro).
      * @param line_number O número da linha no arquivo.
      * @return true se a entrada for consistente (ou a primeira) e adicionada,
      * false se for encontrada uma inconsistência de tamanho.
      */
     bool checkSizeAndAdd(VectorEntry&& entry, const std::string& line, int line_number); // Não pode ser const, modifica membros

public:
    // Construtor
    VectorFileReader();

    // Método para carregar os dados do arquivo
    bool loadFromFile(const std::string& filename);

    // Método para exibir os vetores e suas resoluções
    void displayVectors() const;

    // Método para criar objetos VectorData
    std::vector<TComplexObject> createVectorTComplexObject() const;

    // Métodos de acesso
    int getNumLines() const;
    int getNumElements() const;
    const std::vector<VectorEntry>& getVectors() const;
};

#endif // VECTOR_FILE_READER_HPP