/* Copyright 2003-2017 GBDI-ICMC-USP <caetano@icmc.usp.br>
* (License text omitted for brevity)
*/

//---------------------------------------------------------------------------
// app.h - Implementation of the application using TComplexObject
//---------------------------------------------------------------------------
#ifndef appH
#define appH

#include <vector> // Incluir antes para std::vector
#include <string> // Incluir para std::string
#include <chrono>
#include <cstring> // Para strcmp, se necessário (ou usar std::string)


// Metric Tree includes
#include <arboretum/stMetricTree.h>
#include <arboretum/stPlainDiskPageManager.h>
#include <arboretum/stDiskPageManager.h> // Pode não ser necessário se usar PlainDisk
#include <arboretum/stMemoryPageManager.h> // Pode não ser necessário se usar PlainDisk
#include <arboretum/stSlimTree.h>

// Meu objeto complexo e leitor de arquivo
#include "complex_object.h"         // Substitui city.h
#include "distance_calculator.h"    // Para o avaliador de distância
#include "VectorFileReader.hpp"     // Para carregar dados do arquivo

// Definições de arquivos (nomes alterados para refletir o tipo de dado)
// Os caminhos dos arquivos foram mantidos como solicitado.
#define DATASET_FILE "../data/dados-hist/dataHist20k-3.txt"     // Arquivo com o dataset principal
#define QUERY_FILE "../data/dados-hist/dataHist20k-3-500.txt"    // Arquivo com os objetos de consulta

//---------------------------------------------------------------------------
// class TApp
//---------------------------------------------------------------------------
class TApp {
public:
    /**
    * This is the type used by the result, now using TComplexObject.
    */
    typedef stResult < TComplexObject > myResult;

    /**
    * This is the type of the metric tree defined by TComplexObject and
    * TComplexObjectDistanceEvaluator.
    */
    typedef stMetricTree < TComplexObject, TComplexObjectDistanceEvaluator > MetricTree;

    /**
    * This is the type of the Slim-Tree defined by TComplexObject and
    * TComplexObjectDistanceEvaluator.
    */
    typedef stSlimTree < TComplexObject, TComplexObjectDistanceEvaluator > mySlimTree;

    /**
    * Creates a new instance of this class.
    */
    TApp() : PageManager(nullptr), SlimTree(nullptr) {
        // queryObjects é inicializado vazio por padrão
    } //end TApp

    /**
    * Initializes the application resources (Page Manager, Tree).
    */
    void Init() {
        // To create it in disk
        CreateDiskPageManager();
        // Creates the tree
        CreateTree();
    } //end Init

    /**
    * Runs the application logic: loads data, builds tree, performs queries.
    */
    void Run();

    /**
    * Deinitializes the application, releasing resources.
    */
    void Done();

private:

    /**
    * The Page Manager for SlimTree.
    */
    stPlainDiskPageManager * PageManager;

    /**
    * The SlimTree instance.
    */
    MetricTree * SlimTree; // Usando o typedef genérico MetricTree

    /**
    * Vector for holding the query objects (pointers to TComplexObject).
    */
    std::vector <TComplexObject *> queryObjects; // Agora armazena ponteiros para TComplexObject

    /**
    * Creates a disk page manager. It must be called before CreateTree().
    */
    void CreateDiskPageManager();

    /**
    * Creates a tree using the current PageManager.
    */
    void CreateTree();

    /**
    * Loads data from the specified file using VectorFileReader
    * and populates the SlimTree. Assumes the tree clones objects.
    * @param fileName Path to the dataset file.
    */
    void LoadTree(const std::string& fileName); // Mudado para const std::string&

    /**
    * Loads query objects from the specified file using VectorFileReader
    * into the queryObjects vector. Objects are allocated on the heap.
    * @param fileName Path to the query file.
    */
    void LoadQueryObjects(const std::string& fileName); // Mudado para const std::string& e nome mais descritivo

    /**
    * Performs configured queries (Range, Nearest) and outputs statistics.
    */
    void PerformQueries();

    /**
    * Performs nearest neighbor queries for all objects in queryObjects.
    */
    void PerformNearestQuery();

    /**
    * Performs range queries for all objects in queryObjects.
    */
    void PerformRangeQuery();

}; //end TApp

#endif //end appH