#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>   // For memcpy, memset
#include <cstdint>   // For uint8_t
#include <stdexcept> // For exceptions
#include <vector>
#include <iomanip>   // For std::fixed, std::setprecision
#include <chrono>    // For timing
#include <cmath>     // For std::pow, std::abs, std::isfinite
#include <memory>    // For std::unique_ptr

// Include our classes (EXCETO distance_calculator.h)
#include "VectorFileReader.hpp" // Assumes this exists and works
#include "complex_object.h"     // Includes TComplexObject definition
// #include "distance_calculator.h" // REMOVIDO

using namespace std;
using namespace std::chrono;

// Forward declarations das funções que estavam no início (se necessário)
void writeComplexObjectsToPagedFile(const string& inputFile, const string& outputFile, size_t pageSize);
vector<TComplexObject> readComplexObjectsFromPagedFile(const string& inputFile, size_t pageSize, int& pageAccessCount);
// vector<TComplexObject> sequentialRangeSearch(...); // Declarado mais abaixo

//---------------------------------------------------------------------------
// Função de Cálculo de Distância (Integrada)
// Baseada na lógica de TComplexObjectDistanceEvaluator::GetDistance2
//---------------------------------------------------------------------------
/**
 * @brief Calcula a distância (Manhattan nos coeficientes de aproximação)
 * entre dois objetos TComplexObject.
 * Ajusta a resolução do obj1 para corresponder ao obj2, se necessário,
 * usando um clone temporário.
 *
 * @param obj1 Primeiro TComplexObject (pode ser clonado temporariamente).
 * @param obj2 Segundo TComplexObject (resolução alvo).
 * @param[in, out] distanceCounter Contador para registrar o número de cálculos de distância.
 * @return double A distância calculada.
 * @throws std::runtime_error Se os tamanhos de dados subjacentes forem diferentes,
 * ou se a clonagem/transformação falhar.
 */
double calculateComplexObjectDistance(TComplexObject& obj1, TComplexObject& obj2, long long& distanceCounter) {

    // --- Seção 1: Preparar Ponteiros de Dados e Lidar com Diferenças de Resolução ---

    const int targetResolution = obj2.GetResolution();
    const std::vector<double>& data2_obj = obj2.GetData();

    // Ponteiro para o vetor de dados de obj1 (ou seu clone temporário)
    const std::vector<double>* data1_ptr = &obj1.GetData();

    // Smart pointer para gerenciar a vida útil de um clone temporário de obj1
    std::unique_ptr<TComplexObject> obj1_temp_clone_ptr = nullptr;

    int currentResolution = obj1.GetResolution();

    // Verifica se obj1 precisa de ajuste de resolução
    if (currentResolution != targetResolution) {
        // Clona obj1 para evitar modificar o objeto original permanentemente
        try {
            // obj1.Clone() deve retornar TComplexObject*
            obj1_temp_clone_ptr.reset(obj1.Clone()); // Transfere posse para unique_ptr
        } catch (const std::bad_alloc& e) {
            throw std::runtime_error("Falha ao alocar memória para o clone de obj1.");
        }
        if (!obj1_temp_clone_ptr) {
            throw std::runtime_error("Falha ao clonar obj1 para cálculo de distância (resultado nulo).");
        }

        // Verifica incompatibilidade fundamental de tamanho antes de transformar
        // Assume que GetData() retorna um vetor válido mesmo após clone
        if (obj1_temp_clone_ptr->GetData().size() != data2_obj.size()) {
            // unique_ptr lida com a deleção do clone
            throw std::runtime_error("Objetos têm tamanhos de dados subjacentes diferentes, não podem ser comparados (pós-clone).");
        }

        // Calcula o nível de transformação necessário para o clone
        int levelsToTransform = targetResolution - currentResolution; // Positivo = comprime, Negativo = descomprime

        // Aplica a transformação *ao clone* usando o método público dataCompression
        // Este método deve tratar níveis positivos e negativos internamente.
        // Assume que dataCompression existe e é público em TComplexObject
        obj1_temp_clone_ptr->dataCompression(levelsToTransform);

        // Verifica se a transformação resultou na resolução alvo
        if (obj1_temp_clone_ptr->GetResolution() != targetResolution) {
             throw std::runtime_error("Falha ao ajustar clone de obj1 para resolução alvo. CloneRes="
                 + std::to_string(obj1_temp_clone_ptr->GetResolution()) + ", TargetRes=" + std::to_string(targetResolution));
        }

        // Atualiza o ponteiro para usar os dados transformados do clone
        data1_ptr = &obj1_temp_clone_ptr->GetData();

    } else {
         // Resoluções já coincidem, ainda verifica compatibilidade de tamanho
         if (obj1.GetData().size() != data2_obj.size()) {
             throw std::runtime_error("Objetos têm tamanhos de dados subjacentes diferentes, não podem ser comparados.");
         }
         // data1_ptr já aponta para obj1.GetData() - nenhum clone necessário
    }

    // --- Seção 2: Calcular Distância usando Coeficientes de Aproximação ---

    // Agora, data1_ptr aponta para os dados de obj1 (ou seu clone) na targetResolution
    // e data2_obj são os dados de obj2 na targetResolution. Seus tamanhos subjacentes coincidem.

    // Calcula o número de coeficientes de aproximação na resolução alvo
    if (data2_obj.empty()) {
         distanceCounter++; // Conta o cálculo de distância
         return 0.0; // Distância é 0 se os objetos estiverem vazios
    }
     // Verificação de segurança adicional para data1_ptr
    if (!data1_ptr || data1_ptr->empty()) {
         // Não deve acontecer se data2_obj não estiver vazio e os tamanhos coincidirem, mas verifica
         distanceCounter++; // Conta o cálculo de distância
         return 0.0; // Ou lançar erro? Retornar 0 se ambos vazios parece ok.
    }

    // Usa double para tamanho potencialmente grande ou resultado intermediário de pow
    double powerOfTwo = std::pow(2.0, static_cast<double>(targetResolution));
     // Verificações básicas para o resultado de pow
    if (powerOfTwo <= 0 || !std::isfinite(powerOfTwo)) {
         throw std::runtime_error("Cálculo inválido de potência para resolução.");
    }
     // Evita divisão por zero ou próximo de zero (se resolução for muito alta)
    if (powerOfTwo < 1.0) powerOfTwo = 1.0; // Resolução 0 ou negativa resulta em 2^0=1

    // Calcula o tamanho aproximado, garantindo que não exceda o tamanho real do vetor
    size_t vectorSize = data2_obj.size(); // Usa tamanho comum
    // Cuidado com divisão de inteiros aqui, use double no cálculo
    size_t approxSize = static_cast<size_t>(static_cast<double>(vectorSize) / powerOfTwo);
    approxSize = std::min(approxSize, vectorSize); // Garante não ser maior que o tamanho total

    if (approxSize == 0 && vectorSize > 0) {
         // Resolução pode ser muito alta para o tamanho dos dados
         std::cerr << "Aviso: Resolução " << targetResolution
                   << " resulta em zero coeficientes de aproximação para tamanho "
                   << vectorSize << std::endl;
         distanceCounter++; // Conta o cálculo de distância
         return 0.0; // Ou lançar std::runtime_error("Não é possível comparar neste nível de resolução.");
    }

    // Calcula a distância Manhattan usando apenas os coeficientes de aproximação
    double sumOfDiff = 0.0;
    for (size_t i = 0; i < approxSize; ++i) {
         // Acessa os dados usando desreferência do ponteiro (*data1_ptr)
         // Verifica limites (embora approxSize já deva ser seguro)
         if (i >= data1_ptr->size() || i >= data2_obj.size()) {
             throw std::runtime_error("Índice fora dos limites durante cálculo de distância - lógica interna falhou.");
         }
         double diff = (*data1_ptr)[i] - data2_obj[i];
         sumOfDiff += std::abs(diff); // Usa std::abs para double
    }

    distanceCounter++; // Atualiza o contador de distância
    return sumOfDiff;

    // unique_ptr (obj1_temp_clone_ptr) será destruído automaticamente ao sair do escopo,
    // liberando a memória do clone se ele foi criado.
}


/**
 * @brief Performs a sequential range search.
 * Finds all objects in the dataset that are within a given radius of the query object.
 * Uses the integrated distance calculation function.
 *
 * @param dataset The vector of TComplexObject representing the dataset to search within.
 * @param queryObject The query TComplexObject.
 * @param radius The search radius.
 * @param[in, out] distanceCounter Counter for distance calculations.
 * @return A vector containing copies of the TComplexObject instances found within the radius.
 */
vector<TComplexObject> sequentialRangeSearch(
    vector<TComplexObject>& dataset, // Non-const because distance func needs it
    TComplexObject& queryObject,     // Non-const because distance func needs it
    double radius,
    long long& distanceCounter)      // Pass counter by reference
{
    vector<TComplexObject> results;
    results.reserve(dataset.size() / 10); // Pre-allocate some space (heuristic)

    for (TComplexObject& dataObject : dataset) {
        try {
            // Calculate distance using the integrated function
            double distance = calculateComplexObjectDistance(queryObject, dataObject, distanceCounter);

            // Check if the object is within the specified radius
            if (distance <= radius) {
                results.push_back(dataObject); // Add a copy to the results
            }
        } catch (const std::exception& e) {
            // Log error during distance calculation for a specific pair
            cerr << "ERRO no cálculo de distância entre Query(" << queryObject.GetLabel()
                 << ") e Data(" << dataObject.GetLabel() << "): " << e.what() << endl;
            // Continue searching with the next object in the dataset
        }
    }
    return results;
}

void writeComplexObjectsToPagedFile(const string& inputFile, const string& outputFile, size_t pageSize) {
    // 1. Read data using VectorFileReader
    VectorFileReader reader;
    cout << "INFO: Reading input file '" << inputFile << "'..." << endl;
    if (!reader.loadFromFile(inputFile)) {
        cerr << "ERRO: Falha ao ler o arquivo de entrada com VectorFileReader." << endl;
        return;
    }

    vector<TComplexObject> objects = reader.createVectorTComplexObject();
    if (objects.empty()) {
        cout << "AVISO: Nenhum objeto carregado do arquivo de entrada. Arquivo de saída não será criado." << endl;
        return;
    }
    cout << "INFO: " << objects.size() << " objetos carregados." << endl;

    // 2. Prepare for binary writing
    ofstream outFile(outputFile, ios::binary | ios::trunc); // Truncate if exists
    if (!outFile) {
        cerr << "ERRO: Não foi possível abrir o arquivo de saída binário '" << outputFile << "'!" << endl;
        return;
    }

    // 3. Write objects page by page
    vector<uint8_t> pageBuffer(pageSize, 0); // Initialize buffer with zeros (padding)
    size_t bufferIdx = 0; // Current position within the page buffer

    cout << "INFO: Escrevendo objetos serializados no arquivo binário '" << outputFile << "'..." << endl;
    for (TComplexObject& obj : objects) { // Iterate through objects (needs non-const for Serialize potentially)
        const uint8_t* serialized_obj = obj.Serialize(); // Get serialized data
        size_t obj_size = obj.GetSerializedSize();       // Get its size

        // Sanity check: Object must fit within a page
        if (obj_size > pageSize) {
            cerr << "ERRO: Objeto serializado (Label: " << obj.GetLabel()
                 << ", Size: " << obj_size << " bytes) é maior que o tamanho da página ("
                 << pageSize << " bytes). Abortando." << endl;
            outFile.close();
            // Consider removing the partially written file
            remove(outputFile.c_str());
            return;
        }

        // Check if the object fits in the remaining space of the current page
        if (bufferIdx + obj_size <= pageSize) {
            // Fits: Copy object into the buffer
            memcpy(pageBuffer.data() + bufferIdx, serialized_obj, obj_size);
            bufferIdx += obj_size;
        } else {
            // Doesn't fit:
            // 1. Pad the rest of the current buffer (optional, already zeros)
            //    memset(pageBuffer.data() + bufferIdx, 0, pageSize - bufferIdx); // Already zeroed

            // 2. Write the current (full) page to disk
            outFile.write(reinterpret_cast<char*>(pageBuffer.data()), pageSize);
            if (!outFile) {
                 cerr << "ERRO: Falha ao escrever página no disco!" << endl;
                 return; // Abort on write error
            }

            // 3. Reset buffer (fill with zeros again for padding)
            memset(pageBuffer.data(), 0, pageSize);
            bufferIdx = 0;

            // 4. Copy the current object to the beginning of the new buffer
            memcpy(pageBuffer.data() + bufferIdx, serialized_obj, obj_size);
            bufferIdx += obj_size;
        }
    } // End of loop through objects

    // Write the last partially filled (or full) page if it contains data
    if (bufferIdx > 0) {
        // Optional: Pad the remainder if not already padded
        // memset(pageBuffer.data() + bufferIdx, 0, pageSize - bufferIdx);
        outFile.write(reinterpret_cast<char*>(pageBuffer.data()), pageSize);
         if (!outFile) {
             cerr << "ERRO: Falha ao escrever a última página no disco!" << endl;
             // Consider cleanup
             return;
         }
    }

    cout << "INFO: Escrita no arquivo binário concluída." << endl;
    outFile.close();
}

/**
 * @brief Reads serialized TComplexObject data from a page-aligned binary file.
 * @param inputFile Path to the binary input file created by writeComplexObjectsToPagedFile.
 * @param pageSize The simulated disk page size in bytes (must match writer).
 * @param[out] pageAccessCount Reference to store the number of pages read.
 * @return A vector containing the deserialized TComplexObject instances.
 */
 vector<TComplexObject> readComplexObjectsFromPagedFile(const string& inputFile, size_t pageSize, int& pageAccessCount) {
    pageAccessCount = 0; // Initialize page count

    ifstream inFile(inputFile, ios::binary);
    if (!inFile) {
        cerr << "ERRO: Não foi possível abrir o arquivo binário de entrada '" << inputFile << "'!" << endl;
        return {}; // Return empty vector on error
    }

    vector<TComplexObject> loadedObjects;
    vector<uint8_t> pageBuffer(pageSize); // Buffer to hold one page
    size_t pageBufferIdx = pageSize; // Start as if buffer is "empty" or fully processed

    // cout << "INFO: Lendo objetos serializados do arquivo binário '" << inputFile << "'..." << endl;

    // Loop reading page by page
    while (true) {
        // If the internal page buffer pointer is at the end, read a new page
        if (pageBufferIdx >= pageSize) {
            inFile.read(reinterpret_cast<char*>(pageBuffer.data()), pageSize);
            if (inFile.gcount() == 0) { // Check if read failed or reached EOF immediately
                 if(inFile.eof()){
                    // Expected EOF if file size is multiple of page size and fully processed
                     // cout << "DEBUG: EOF reached after processing full page." << endl;
                 } else {
                     cerr << "ERRO: Falha ao ler página do arquivo binário!" << endl;
                 }
                 break; // Stop reading
            }
             if (inFile.gcount() < static_cast<std::streamsize>(pageSize)) {
                // Read less than a full page - likely the last partial page.
                // Adjust effective buffer size for this last read.
                // cout << "DEBUG: Read last partial page (" << inFile.gcount() << " bytes)." << endl;
                 // For simplicity in this reader, we might just process what we can
                 // or decide how to handle potentially incomplete objects at EOF.
                 // If the writer *always* pads to pageSize, gcount() should always be pageSize until EOF.
                 if (!inFile.eof()) { // If not EOF, it's a read error
                    cerr << "ERRO: Leitura incompleta de página antes do EOF!" << endl;
                    break;
                 }
            }
            pageAccessCount++; // Count successful page read
            pageBufferIdx = 0;  // Reset internal pointer to start of new page
        }

        // Try to deserialize objects from the current position within the page buffer
        bool deserializedSomething = false;
        while (pageBufferIdx < pageSize) {
             // Check if enough bytes remain for the *fixed size header* of TComplexObject
             // Header: Resolution (int), Data Size (size_t), Label Length (size_t)
             const size_t HEADER_SIZE = sizeof(int) + 2 * sizeof(size_t);
             if (pageBufferIdx + HEADER_SIZE > pageSize) {
                 // Not enough space left in this page for even a header, need next page
                 // cout << "DEBUG: Not enough space for header, breaking inner loop." << endl;
                 pageBufferIdx = pageSize; // Force reading next page
                 break;
             }

             // "Peek" at the header info without deserializing yet
             int tempResolution;
             size_t dataSize, labelLen;
             size_t currentReadPos = pageBufferIdx;
             memcpy(&tempResolution, pageBuffer.data() + currentReadPos, sizeof(int));
             currentReadPos += sizeof(int);
             memcpy(&dataSize, pageBuffer.data() + currentReadPos, sizeof(size_t));
             currentReadPos += sizeof(size_t);
             memcpy(&labelLen, pageBuffer.data() + currentReadPos, sizeof(size_t));
             // currentReadPos += sizeof(size_t); // No need to advance read pos yet

             // Calculate the full expected size of this object
             size_t expectedObjSize = HEADER_SIZE + labelLen + (dataSize * sizeof(double));

             // Check if the object starts with resolution 0 AND has 0 size/length.
             // This *might* indicate padding if we used zeros. Be cautious with this check.
             // A safer approach is needed if valid objects can have resolution 0 and empty data/label.
             // Let's assume valid objects have *some* size or non-zero resolution for now.
             // If resolution is 0 AND expectedObjSize == HEADER_SIZE, it's likely padding.
             if (tempResolution == 0 && expectedObjSize == HEADER_SIZE) {
                 // Potential padding detected, skip to next page.
                 // This assumes padding starts with 0 for resolution.
                 // If valid objects can have resolution 0, this logic fails.
                 // cout << "DEBUG: Potential padding detected, breaking inner loop." << endl;
                 pageBufferIdx = pageSize; // Force reading next page
                 break;
             }

             // Check if the *entire* object fits within the bounds of the current page buffer
             if (pageBufferIdx + expectedObjSize <= pageSize) {
                 // Object fits entirely within the current page buffer segment
                 try {
                     TComplexObject obj;
                     obj.Unserialize(pageBuffer.data() + pageBufferIdx, expectedObjSize);
                     loadedObjects.push_back(obj);
                     pageBufferIdx += expectedObjSize; // Advance pointer past the object
                     deserializedSomething = true;
                 } catch (const std::exception& e) {
                     cerr << "ERRO: Falha ao deserializar objeto na posição " << pageBufferIdx
                          << " da página " << pageAccessCount << ". Erro: " << e.what() << endl;
                     // Critical error - stop processing this page / file?
                     pageBufferIdx = pageSize; // Move to next page, hoping it recovers
                     break;
                 }
             } else {
                 // Object calculated size extends beyond the current page buffer.
                 // Since the writer ensures objects don't span pages, this signifies
                 // the end of valid data in this page (or a read error/corruption).
                 // cout << "DEBUG: Object spans page boundary (or end of data in page), breaking inner loop." << endl;
                 pageBufferIdx = pageSize; // Force reading next page
                 break;
             }
        } // End while(pageBufferIdx < pageSize)

        // If we iterated through the whole page buffer and didn't deserialize anything,
        // it might mean we only read padding on the last read. Stop.
        // (This check might be redundant depending on EOF handling).
        // if (!deserializedSomething && pageBufferIdx >= pageSize) {
            // break;
        //}

    } // End while(true) reading pages

    // cout << "INFO: Leitura do arquivo binário concluída." << endl;
    // cout << "INFO: Total de objetos deserializados: " << loadedObjects.size() << endl;

    inFile.close();
    return loadedObjects;
}


//===========================================================================
//                           FUNÇÃO MAIN
//===========================================================================
int main(int argc, char* argv[]) {

    // --- Argument Parsing ---
    if (argc < 5) {
        cerr << "Uso: " << argv[0] << " <pageSize> <searchRadius> <queryFilePath>" << endl;
        cerr << "Exemplo: " << argv[0] << " 4096 100.0 ../data/queries/query_10.txt" << endl;
         cerr << "   <pageSize>: Tamanho da página de disco simulada em bytes (ex: 4096, 8192, 131072)." << endl;
         cerr << "   <searchRadius>: Raio para a busca de vizinhos (ex: 50.0, 1000.0)." << endl;
         cerr << "   <dataFilePath>: Caminho para o arquivo texto contendo os objetos do dataset." << endl;
         cerr << "   <queryFilePath>: Caminho para o arquivo texto contendo os objetos de consulta (mesmo formato do dataset)." << endl;
        return 1;
    }

    size_t pageSize = 4096; // Default page size
    double searchRadius = 0.0;
    string queryFilePath, dataInputFile;

    try {
        pageSize = std::stoul(argv[1]); // Use stoul for unsigned long (size_t)
        searchRadius = std::stod(argv[2]); // Use stod for double
        dataInputFile = argv[3];
        queryFilePath = argv[4];
    } catch (const std::invalid_argument& e) {
        cerr << "ERRO: Argumento inválido. Verifique se pageSize é um inteiro positivo e searchRadius é um número." << endl;
        return 1;
    } catch (const std::out_of_range& e) {
        cerr << "ERRO: Argumento fora do intervalo válido." << endl;
        return 1;
    }

    if (pageSize == 0) {
        cerr << "ERRO: pageSize deve ser maior que zero." << endl;
        return 1;
    }
     if (searchRadius < 0) {
         cerr << "AVISO: searchRadius é negativo (" << searchRadius <<"). A busca pode não retornar resultados esperados, pois distâncias são não-negativas." << endl;
     }

    // --- Configuration ---
    string dataOutputFile = "complex_objects_paged.dat"; // Output binary file for dataset

    // --- Writing Dataset (Optional) ---
    cout << "========= ESCREVENDO DADOS DO DATASET EM PÁGINAS =========" << endl;
    writeComplexObjectsToPagedFile(dataInputFile, dataOutputFile, pageSize);
    cout << "=========================================================\n" << endl;

    // --- Reading Query Data ---
    cout << "========= LENDO DADOS DE CONSULTA =========" << endl;
    VectorFileReader queryReader;
    cout << "INFO: Lendo arquivo de consulta '" << queryFilePath << "'..." << endl;
    if (!queryReader.loadFromFile(queryFilePath)) {
        cerr << "ERRO: Falha ao ler o arquivo de consulta com VectorFileReader." << endl;
        return 1;
    }
    vector<TComplexObject> queryData = queryReader.createVectorTComplexObject();
    if (queryData.empty()) {
        cout << "AVISO: Nenhum objeto carregado do arquivo de consulta. Nenhuma busca será realizada." << endl;
        return 0;
    }
    cout << "INFO: " << queryData.size() << " objetos de consulta carregados." << endl;
    cout << "===========================================\n" << endl;


    // --- Performing Sequential Range Search ---
    cout << "========= REALIZANDO BUSCA SEQUENCIAL POR RAIO =========" << endl;
    cout << "Raio de busca: " << fixed << setprecision(4) << searchRadius << endl;

    // TComplexObjectDistanceEvaluator distEval; // REMOVIDO
    long long totalDistanceCalculations = 0;    // Contador local
    size_t totalFoundObjects = 0;

    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

    int queryCount = 0;
    int pagesReadTotal = 0;

    for (TComplexObject& queryObj : queryData) {
        int pagesRead = 0;

        vector<TComplexObject> loadedData = readComplexObjectsFromPagedFile(dataOutputFile, pageSize, pagesRead);
        pagesReadTotal += pagesRead;
        queryCount++;

        // Perform the search for the current query object, passing the counter
        vector<TComplexObject> foundObjects = sequentialRangeSearch(loadedData, queryObj, searchRadius, totalDistanceCalculations);

        // Report results for this query
        // cout << "Consulta " << queryCount << " (Label: " << queryObj.GetLabel() << "): Encontrados " << foundObjects.size() << " objetos dentro do raio." << endl;
        totalFoundObjects += foundObjects.size();
    } // End loop through query objects

    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    long long duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();

    cout << "\n--- Estatísticas da Busca Sequencial ---" << endl;
    cout << "Número total de consultas realizadas: " << queryData.size() << endl;
    cout << "Número total de objetos encontrados (soma de todas as consultas): " << totalFoundObjects << endl;
    // Reporta o contador local
    cout << "Número total de cálculos de distância realizados: " << totalDistanceCalculations << endl;
    cout << "Tempo total da busca: " << duration_ms << " ms" << endl;
    cout << "====================================================\n" << endl;

    std::cout << "\n================JSON================\n";
    std::cout << "{\n";
    std::cout << "\t\"" << "avg_time" << "\" : " << duration_ms/queryData.size() << "," << std::endl;
    std::cout << "\t\"" << "disk_access" << "\" : " << double(pagesReadTotal)/queryData.size() << "," << std::endl;
    std::cout << "\t\"" << "avg_dist_calc" << "\" : " << double(totalDistanceCalculations)/queryData.size() << "," << std::endl;
    std::cout << "\t\"" << "avg_obj_result" << "\" : " << double(totalFoundObjects)/queryData.size() << "," << std::endl;
    std::cout << "\t\"" << "radius" << "\" : " << searchRadius << "," << std::endl;
    std::cout << "\t\"" << "num_consults" << "\" : " << queryData.size() << std::endl;
    std::cout << "}";
    std::cout << "\n================JSON================\n";

    return 0;
}