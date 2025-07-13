/* Copyright 2003-2017 GBDI-ICMC-USP <caetano@icmc.usp.br>
* (License text omitted for brevity)
*/
//---------------------------------------------------------------------------
// app.cpp - Implementation of the application using TComplexObject.
//---------------------------------------------------------------------------
#include <iostream>
#include <fstream>  // Ainda necessário para ifstream em VectorFileReader (indiretamente)
#include <vector>
#include <string>
#include <stdexcept> // Para std::runtime_error (potencialmente)

#pragma hdrstop // Manter se usar C++Builder
#include "app.h" // Inclui todas as definições e headers necessários

std::string dataset_file_var = "../data/dados-hist/dataHist20k-3.txt";     // Arquivo com o dataset principal
std::string query_file_var = "../data/dados-hist/dataHist20k-3-500.txt";    // Arquivo com os objetos de consulta
double range_query_var = 10000;

int disk_page_size = 131072;

//---------------------------------------------------------------------------
#pragma package(smart_init) // Manter se usar C++Builder
//---------------------------------------------------------------------------
// Class TApp
//------------------------------------------------------------------------------
void TApp::CreateTree() {
    // create for Slim-Tree using the typedef defined in app.h
    // which now uses TComplexObject and TComplexObjectDistanceEvaluator
    if (PageManager) {
         SlimTree = new mySlimTree(PageManager);
         std::cout << "INFO: Instância mySlimTree criada." << std::endl;
    } else {
         std::cerr << "ERRO: PageManager não inicializado antes de CreateTree!" << std::endl;
         // Considerar lançar uma exceção ou tratar o erro de forma mais robusta
    }

} //end TApp::CreateTree

//------------------------------------------------------------------------------
void TApp::CreateDiskPageManager() {
    // Cria o page manager em disco para o SlimTree
    // O nome do arquivo pode ser alterado se desejado.
    PageManager = new stPlainDiskPageManager("SlimTreeComplex.dat", disk_page_size); // Nome do arquivo alterado opcionalmente
     std::cout << "INFO: stPlainDiskPageManager criado ('SlimTreeComplex.dat')." << std::endl;
} //end TApp::CreateDiskPageManager

//------------------------------------------------------------------------------
void TApp::Run() {
    // Carrega os objetos do arquivo de dataset e constrói a árvore
    std::cout << "\nConstruindo a SlimTree a partir de: " << dataset_file_var << std::endl;
    LoadTree(dataset_file_var); // Usa a nova define

    // Carrega os objetos do arquivo de consulta para o vetor queryObjects
    std::cout << "\nCarregando objetos de consulta de: " << query_file_var << std::endl;
    LoadQueryObjects(query_file_var); // Usa a nova define e nova função

    // Executa as consultas se houver objetos de consulta carregados
    if (!queryObjects.empty()) {
        std::cout << "\nExecutando Consultas..." << std::endl;
        PerformQueries();
    } else {
        std::cout << "\nNenhum objeto de consulta carregado. Consultas não serão executadas." << std::endl;
    }
    // Mensagem final
    std::cout << "\n\nProcesso concluído!" << std::endl;
} //end TApp::Run

//------------------------------------------------------------------------------
void TApp::Done() {
    // Libera a memória da árvore (que deve liberar suas páginas através do page manager)
    if (this->SlimTree != nullptr) {
        delete this->SlimTree;
        this->SlimTree = nullptr; // Boa prática: zerar ponteiro após delete
         std::cout << "INFO: Instância SlimTree liberada." << std::endl;
    }
    // Libera a memória do page manager
    if (this->PageManager != nullptr) {
        delete this->PageManager;
        this->PageManager = nullptr; // Boa prática
         std::cout << "INFO: Instância PageManager liberada." << std::endl;
    }

    // Libera a memória dos objetos de consulta alocados no heap
    if (!queryObjects.empty()) {
        std::cout << "INFO: Liberando memória dos objetos de consulta (" << queryObjects.size() << ")..." ;
        for (TComplexObject* obj : queryObjects) {
             if (obj) { // Verifica se ponteiro não é nulo antes de deletar
                delete obj;
             }
        }
        queryObjects.clear(); // Limpa o vetor de ponteiros
        std::cout << " Ok." << std::endl;
    }
} //end TApp::Done

//------------------------------------------------------------------------------
// Assume que SlimTree->Add CLONA o objeto, como implícito no código original.
void TApp::LoadTree(const std::string& fileName) {
    if (!SlimTree) {
        std::cerr << "ERRO: SlimTree não inicializada antes de LoadTree!" << std::endl;
        return;
    }

    VectorFileReader reader;
    std::cout << "INFO: Lendo arquivo de dataset '" << fileName << "'..." << std::endl;

    if (!reader.loadFromFile(fileName)) {
        std::cerr << "ERRO: Falha ao carregar dados de '" << fileName << "' usando VectorFileReader." << std::endl;
        return;
    }

    // Cria os TComplexObjects a partir dos dados lidos
    std::vector<TComplexObject> objects = reader.createVectorTComplexObject();

    if (objects.empty()) {
         std::cout << "AVISO: Nenhum objeto válido encontrado/criado a partir de '" << fileName << "'. A árvore permanecerá vazia." << std::endl;
         return;
    }

    std::cout << "INFO: Adicionando " << objects.size() << " objetos à SlimTree ";
    long w = 0;
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

    for (const auto& obj : objects) {

        bool added = SlimTree->Add(const_cast<TComplexObject*>(&obj)); // Tentativa direta

        if (!added) {
             std::cerr << "\nAVISO: Falha ao adicionar objeto com label '" << obj.GetLabel() << "' à árvore." << std::endl;
             // Decidir se deve continuar ou abortar
        }

        w++;
        if (w % 100 == 0) { // Imprime um ponto a cada 100 objetos adicionados
            std::cout << '.';
            std::cout.flush(); // Garante que o ponto seja exibido imediatamente
        }
    }

    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    long long duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();

    std::cout << " Concluído." << std::endl;
    std::cout << "INFO: Total de objetos na árvore: " << SlimTree->GetNumberOfObjects() << std::endl;
    std::cout << "INFO: Tempo para adicionar objetos: " << duration_ms << " ms" << std::endl;

} //end TApp::LoadTree

//------------------------------------------------------------------------------
void TApp::LoadQueryObjects(const std::string& fileName) {
   // Limpa vetor antigo (se houver - Done() cuida da desalocação)
   if (!queryObjects.empty()) {
        std::cerr << "AVISO: Vetor queryObjects não estava vazio antes de carregar. Conteúdo anterior perdido (memória deve ser liberada por Done)." << std::endl;
        // Não deletamos aqui, pois Done é responsável pela limpeza final.
        // Apenas limpamos o vetor de ponteiros.
        queryObjects.clear();
   }


   VectorFileReader reader;
    std::cout << "INFO: Lendo arquivo de consulta '" << fileName << "'..." << std::endl;

   if (!reader.loadFromFile(fileName)) {
       std::cerr << "ERRO: Falha ao carregar dados de consulta de '" << fileName << "'." << std::endl;
       return;
   }

   // Obtém os VectorEntry's lidos
   const std::vector<VectorEntry>& entries = reader.getVectors();

    if (entries.empty()) {
        std::cout << "AVISO: Nenhum objeto de consulta válido encontrado/criado a partir de '" << fileName << "'." << std::endl;
        return;
    }

   // Reserva espaço no vetor de ponteiros
   queryObjects.reserve(entries.size());

   std::cout << "INFO: Criando objetos de consulta na memória..." << std::endl;
   int count = 0;
   for (const auto& entry : entries) {
       // Cria um NOVO objeto TComplexObject no HEAP para cada consulta
       TComplexObject* queryObj = new TComplexObject(std::string(entry.label), entry.resolution, entry.data);
       // Adiciona o PONTEIRO ao vetor
       queryObjects.push_back(queryObj);
       count++;
   }

   std::cout << "INFO: " << queryObjects.size() << " objetos de consulta carregados." << std::endl;

} //end TApp::LoadQueryObjects

//------------------------------------------------------------------------------
void TApp::PerformQueries() {
    if (!SlimTree) {
         std::cerr << "ERRO: Tentando executar consultas sem uma árvore inicializada!" << std::endl;
         return;
    }
     if (queryObjects.empty()) {
         std::cout << "INFO: Nenhum objeto de consulta para executar." << std::endl;
         return;
     }

    std::cout << "\n--- Iniciando Consultas por Faixa (Range Query) ---";
    PerformRangeQuery();
    std::cout << "\n--- Consultas por Faixa Concluídas ---";

    // std::cout << "\n\n--- Iniciando Consultas de Vizinho Mais Próximo (Nearest Query) ---";
    // PerformNearestQuery();
    // std::cout << "\n--- Consultas de Vizinho Mais Próximo Concluídas ---";

} //end TApp::PerformQueries

//------------------------------------------------------------------------------
void TApp::PerformRangeQuery() {
    if (!SlimTree || queryObjects.empty()) return;

    myResult* result = nullptr; // Use o typedef myResult
    const double radius = range_query_var; // Raio da consulta por faixa (ajuste conforme necessário)
    unsigned int size = queryObjects.size();
    unsigned int totalResultSize = 0; // Para estatística opcional

    std::cout << "\n  Raio da consulta: " << radius;
    std::cout << "\n  Número de consultas: " << size;

    // Reseta estatísticas antes do loop de consultas
    PageManager->ResetStatistics();
    SlimTree->GetMetricEvaluator()->ResetStatistics();

    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

    for (unsigned int i = 0; i < size; ++i) {
        // queryObjects[i] agora é TComplexObject*
        result = SlimTree->RangeQuery(queryObjects[i], radius);
        if (result) {
            totalResultSize += result->GetNumOfEntries(); // Acumula o número de resultados encontrados
            delete result; // Libera a memória do objeto de resultado
        } else {
             std::cerr << "\nAVISO: RangeQuery retornou nullptr para o objeto de consulta " << i << std::endl;
        }
    }

    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    long long duration_us = std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();
    long long duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();

    std::cout << "\n  Tempo total: " << duration_ms << " ms (" << duration_us << " µs)";
    if (size > 0) {
        std::cout << "\n  Tempo médio por consulta: " << static_cast<double>(duration_us) / size << " µs";
        std::cout << "\n  Média de Acessos a Disco (Leitura): " << static_cast<double>(PageManager->GetReadCount()) / size;
        // Pode adicionar PageManager->GetWriteCount() se relevante
        std::cout << "\n  Média de Cálculos de Distância: " << static_cast<double>(SlimTree->GetMetricEvaluator()->GetDistanceCount()) / size;
        std::cout << "\n  Média de Objetos Retornados por Consulta: " << static_cast<double>(totalResultSize) / size;

        std::cout << "\n================JSON================\n";
        std::cout << "{\n";
        std::cout << "\t\"" << "avg_time" << "\" : " << static_cast<double>(duration_ms) / (size) << "," << std::endl;
        std::cout << "\t\"" << "disk_access" << "\" : " << static_cast<double>(PageManager->GetReadCount()) / size << "," << std::endl;
        std::cout << "\t\"" << "avg_dist_calc" << "\" : " << static_cast<double>(SlimTree->GetMetricEvaluator()->GetDistanceCount()) / size << "," << std::endl;
        std::cout << "\t\"" << "avg_obj_result" << "\" : " << static_cast<double>(totalResultSize) / size << "," << std::endl;
        std::cout << "\t\"" << "radius" << "\" : " << radius << "," << std::endl;
        std::cout << "\t\"" << "num_consults" << "\" : " << size << std::endl;
        std::cout << "}";
        std::cout << "\n================JSON================\n";
    }

} //end TApp::PerformRangeQuery

//------------------------------------------------------------------------------
void TApp::PerformNearestQuery() {
    if (!SlimTree || queryObjects.empty()) return;

    myResult* result = nullptr; // Use o typedef myResult
    const int k = 15; // Número de vizinhos (k) para buscar (ajuste conforme necessário)
    unsigned int size = queryObjects.size();
    unsigned int totalResultSize = 0;

     std::cout << "\n  Número de vizinhos (k): " << k;
     std::cout << "\n  Número de consultas: " << size;

    // Reseta estatísticas
    PageManager->ResetStatistics();
    SlimTree->GetMetricEvaluator()->ResetStatistics();

    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

    for (unsigned int i = 0; i < size; ++i) {
        // queryObjects[i] agora é TComplexObject*
        result = SlimTree->NearestQuery(queryObjects[i], k);
         if (result) {
            totalResultSize += result->GetNumOfEntries();
            delete result; // Libera a memória do objeto de resultado
        } else {
             std::cerr << "\nAVISO: NearestQuery retornou nullptr para o objeto de consulta " << i << std::endl;
        }
    }

    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    long long duration_us = std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();
    long long duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();


    std::cout << "\n  Tempo total: " << duration_ms << " ms (" << duration_us << " µs)";
     if (size > 0) {
        std::cout << "\n  Tempo médio por consulta: " << static_cast<double>(duration_us) / size << " µs";
        std::cout << "\n  Média de Acessos a Disco (Leitura): " << static_cast<double>(PageManager->GetReadCount()) / size;
        std::cout << "\n  Média de Cálculos de Distância: " << static_cast<double>(SlimTree->GetMetricEvaluator()->GetDistanceCount()) / size;
        // A média de objetos retornados deve ser próxima de k, mas pode ser menor se houver menos de k objetos na árvore.
        // std::cout << "\n  Média de Objetos Retornados por Consulta: " << static_cast<double>(totalResultSize) / size;
    }

} //end TApp::PerformNearestQuery