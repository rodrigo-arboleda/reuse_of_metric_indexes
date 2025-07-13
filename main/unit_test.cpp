// unit_test.cpp

#include <iostream>
#include <string>
#include <vector>
#include <cmath>     // Para std::abs, std::sqrt
#include <stdexcept> // Para std::runtime_error (em try-catch)
#include <limits>    // Para std::numeric_limits (para epsilon)

// Includes das classes a serem testadas
#include "VectorFileReader.hpp" // Presumindo que este arquivo existe
#include "complex_object.h"
#include "distance_calculator.h"

#define VERDE "\033[32m"
#define VERMELHO "\033[31m"
#define RESET "\033[0m"

// --- Função de Teste para VectorFileReader ---
bool testVectorFileReader() {
    std::cout << "\n--- Iniciando Teste: VectorFileReader ---" << std::endl;
    bool success = true; // Assume sucesso inicialmente

    // Cria uma instância do leitor
    VectorFileReader reader;

    // Define o nome do arquivo de teste
    // IMPORTANTE: Certifique-se que este caminho está correto em relação
    // ao local onde o executável de teste será rodado.
    const std::string filename = "../data/TestLoad.txt";
    std::cout << "[INFO] Tentando carregar o arquivo: '" << filename << "'" << std::endl;

    // Chama o método para carregar os dados
    bool load_success = false;
    try {
         load_success = reader.loadFromFile(filename);
    } catch (const std::exception& e) {
        std::cerr << VERMELHO << "[ERRO] Exceção durante loadFromFile: " << e.what() << RESET << std::endl;
        load_success = false;
    }


    // Verifica se o carregamento foi bem-sucedido
    if (load_success) {
        std::cout << VERDE << "[SUCESSO] Arquivo carregado e dados validados com sucesso." << RESET << std::endl;

        // Verifica se os métodos de acesso retornam valores esperados
        // (Adapte estes valores ao conteúdo esperado do seu TestLoad.txt)
        const size_t expected_lines = 4; 
        const size_t expected_elements = 5;

        if (reader.getNumLines() == expected_lines) {
             std::cout << "[INFO] Número de linhas carregadas: " << reader.getNumLines() << " (Esperado: " << expected_lines << ")" << std::endl;
        } else {
             std::cerr << VERMELHO << "[FALHA] Número de linhas incorreto. Obtido: " << reader.getNumLines() << ", Esperado: " << expected_lines << RESET << std::endl;
             success = false;
        }

        if (reader.getNumElements() == expected_elements) {
            std::cout << "[INFO] Número de elementos por vetor: " << reader.getNumElements() << " (Esperado: " << expected_elements << ")" << std::endl;
        } else {
             std::cerr << VERMELHO << "[FALHA] Número de elementos incorreto. Obtido: " << reader.getNumElements() << ", Esperado: " << expected_elements << RESET << std::endl;
             success = false;
        }

        // Exibe os dados carregados (útil para depuração visual)
        std::cout << "[INFO] Exibindo dados carregados (via displayVectors):" << std::endl;
        reader.displayVectors();

    } else {
        std::cerr << VERMELHO << "[FALHA] Falha ao carregar ou validar o arquivo '" << filename << "'." << RESET << std::endl;
        std::cerr << "  Verifique as mensagens de erro ou o conteúdo do arquivo." << std::endl;
        success = false;
    }

    std::cout << "--- Teste VectorFileReader Concluído: " << (success ? VERDE "SUCESSO" : VERMELHO "FALHA") << RESET << " ---" << std::endl;
    return success;
}

// --- Função de Teste para TComplexObject ---
bool testComplexObject() {
    std::cout << "\n--- Iniciando Teste: TComplexObject ---" << std::endl;
    bool success = true; // Assume sucesso inicialmente
    const double epsilon = std::numeric_limits<double>::epsilon() * 100; // Tolerância para floats

    // 1. Teste de Construtores e Getters
    std::cout << "[TESTE] Construtores e Getters..." << std::endl;
    try {
        TComplexObject obj1; // Default constructor
        if (obj1.GetResolution() != 0 || !obj1.GetLabel().empty() || !obj1.GetData().empty()) {
            std::cerr << VERMELHO << "[FALHA] Construtor padrão não inicializou corretamente." << RESET << std::endl;
            success = false;
        }

        std::vector<double> data_vec = {1.1, 2.2, 3.3};
        TComplexObject obj2("LabelA", 10, data_vec);
        if (obj2.GetResolution() != 10 || obj2.GetLabel() != "LabelA" || obj2.GetData() != data_vec) {
             std::cerr << VERMELHO << "[FALHA] Construtor parametrizado ou getters incorretos." << RESET << std::endl;
             success = false;
        }
         std::cout << "[INFO] Construtores OK." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << VERMELHO << "[ERRO] Exceção nos construtores/getters: " << e.what() << RESET << std::endl;
        success = false;
    }

    // 2. Teste de Clone
    std::cout << "[TESTE] Clone..." << std::endl;
    try {
        std::vector<double> data_vec_orig = {5.5, 6.6};
        TComplexObject original("Original", 5, data_vec_orig);
        TComplexObject* clone = original.Clone();

        if (!clone) {
             std::cerr << VERMELHO << "[FALHA] Clone retornou nullptr." << RESET << std::endl;
             success = false;
        } else {
            if (clone == &original) { // Deve ser diferente objeto/endereço
                std::cerr << VERMELHO << "[FALHA] Clone retornou o mesmo ponteiro do original." << RESET << std::endl;
                success = false;
            }
            // Verifica se o conteúdo é igual usando IsEqual
            if (!original.IsEqual(clone)) {
                 std::cerr << VERMELHO << "[FALHA] Clone não é igual ao original (IsEqual falhou)." << RESET << std::endl;
                 success = false;
            }
            // Verifica se Getters retornam o mesmo
             if (clone->GetLabel() != original.GetLabel() || clone->GetResolution() != original.GetResolution() || clone->GetData() != original.GetData()) {
                 std::cerr << VERMELHO << "[FALHA] Conteúdo do Clone difere do original (Getters)." << RESET << std::endl;
                 success = false;
             }
            delete clone; // Lembrar de deletar o clone
             std::cout << "[INFO] Clone OK." << std::endl;
        }
    } catch (const std::exception& e) {
         std::cerr << VERMELHO << "[ERRO] Exceção durante o teste de Clone: " << e.what() << RESET << std::endl;
         success = false;
    }


    // 3. Teste de IsEqual
    std::cout << "[TESTE] IsEqual..." << std::endl;
    try {
        std::vector<double> dv1 = {1.0, 2.0};
        std::vector<double> dv2 = {1.0, 2.0};
        std::vector<double> dv3 = {1.0, 3.0}; // Diferente valor
        std::vector<double> dv4 = {1.0};      // Diferente tamanho

        TComplexObject eq_obj1("LabelX", 20, dv1);
        TComplexObject eq_obj2("LabelY", 20, dv2); // Mesmo Res/Data, Label diferente
        TComplexObject eq_obj3("LabelX", 21, dv1); // Mesmo Data, Res diferente
        TComplexObject eq_obj4("LabelX", 20, dv3); // Mesmo Res, Data diferente (valor)
        TComplexObject eq_obj5("LabelX", 20, dv4); // Mesmo Res, Data diferente (tamanho)

        if (!eq_obj1.IsEqual(&eq_obj2)) { // Deve ser igual (Label não importa)
            std::cerr << VERMELHO << "[FALHA] IsEqual falhou para objetos com mesmo Res/Data e Labels diferentes." << RESET << std::endl;
            success = false;
        }
         if (eq_obj1.IsEqual(&eq_obj3)) { // Deve ser diferente (Res diferente)
            std::cerr << VERMELHO << "[FALHA] IsEqual retornou true para objetos com Res diferentes." << RESET << std::endl;
            success = false;
        }
        if (eq_obj1.IsEqual(&eq_obj4)) { // Deve ser diferente (Data valor diferente)
            std::cerr << VERMELHO << "[FALHA] IsEqual retornou true para objetos com Data diferente (valor)." << RESET << std::endl;
            success = false;
        }
        if (eq_obj1.IsEqual(&eq_obj5)) { // Deve ser diferente (Data tamanho diferente)
            std::cerr << VERMELHO << "[FALHA] IsEqual retornou true para objetos com Data diferente (tamanho)." << RESET << std::endl;
            success = false;
        }
         if (eq_obj1.IsEqual(nullptr)) { // Deve retornar false para nullptr
            std::cerr << VERMELHO << "[FALHA] IsEqual não retornou false para nullptr." << RESET << std::endl;
            success = false;
        }
         std::cout << "[INFO] IsEqual OK." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << VERMELHO << "[ERRO] Exceção durante o teste de IsEqual: " << e.what() << RESET << std::endl;
        success = false;
    }

    // 4. Teste de Serialização/Deserialização
    std::cout << "[TESTE] Serialize/Unserialize..." << std::endl;
    try {
        std::vector<double> data_ser = {7.7, 8.8, 9.9, 10.1};
        TComplexObject obj_orig("SerializeMe", 55, data_ser);
        TComplexObject obj_dest; // Objeto destino vazio

        const uint8_t* serialized_data = obj_orig.Serialize();
        size_t serialized_size = obj_orig.GetSerializedSize();

        if (!serialized_data || serialized_size == 0) {
             std::cerr << VERMELHO << "[FALHA] Serialize retornou dados nulos ou tamanho zero." << RESET << std::endl;
             success = false;
        } else {
            obj_dest.Unserialize(serialized_data, serialized_size);

            // Verifica igualdade usando IsEqual
            if (!obj_orig.IsEqual(&obj_dest)) {
                 std::cerr << VERMELHO << "[FALHA] Objeto deserializado não é igual ao original (IsEqual)." << RESET << std::endl;
                 success = false;
            }
            // Verifica getters individuais
            if (obj_dest.GetLabel() != obj_orig.GetLabel() ||
                obj_dest.GetResolution() != obj_orig.GetResolution() ||
                obj_dest.GetData() != obj_orig.GetData())
            {
                std::cerr << VERMELHO << "[FALHA] Getters do objeto deserializado não batem com o original." << RESET << std::endl;
                success = false;
            }
             std::cout << "[INFO] Serialize/Unserialize OK." << std::endl;
        }
         // Teste com objeto vazio
         TComplexObject obj_empty;
         TComplexObject obj_empty_dest;
         const uint8_t* empty_ser_data = obj_empty.Serialize();
         size_t empty_ser_size = obj_empty.GetSerializedSize();
         obj_empty_dest.Unserialize(empty_ser_data, empty_ser_size);
          if (!obj_empty.IsEqual(&obj_empty_dest)) {
                std::cerr << VERMELHO << "[FALHA] Serialização/Deserialização de objeto vazio falhou." << RESET << std::endl;
                success = false;
          } else {
               std::cout << "[INFO] Serialize/Unserialize (objeto vazio) OK." << std::endl;
          }


    } catch (const std::exception& e) {
        std::cerr << VERMELHO << "[ERRO] Exceção durante o teste de Serialize/Unserialize: " << e.what() << RESET << std::endl;
        success = false;
    }


    std::cout << "--- Teste TComplexObject Concluído: " << (success ? VERDE "SUCESSO" : VERMELHO "FALHA") << RESET << " ---" << std::endl;
    return success;
}

// --- Função de Teste para TComplexObjectDistanceEvaluator ---
bool testDistanceCalculator() {
    std::cout << "\n--- Iniciando Teste: TComplexObjectDistanceEvaluator ---" << std::endl;
    bool success = true; // Assume sucesso inicialmente
    const double epsilon = 1e-9; // Tolerância pequena para comparação de doubles

    try {
        TComplexObjectDistanceEvaluator evaluator;

        // 1. Teste com objetos idênticos
        std::cout << "[TESTE] Distância entre objetos idênticos..." << std::endl;
        std::vector<double> data1 = {1.5, 2.5, 3.5};
        TComplexObject objA("A", 1, data1);
        TComplexObject objB("B", 1, data1); // Mesmo Data e Res

        double dist2_ident = evaluator.GetDistance2(objA, objB);
        double dist_ident = evaluator.GetDistance(objA, objB);

        if (std::abs(dist2_ident - 0.0) > epsilon || std::abs(dist_ident - 0.0) > epsilon) {
            std::cerr << VERMELHO << "[FALHA] Distância entre objetos idênticos não é zero. Dist2=" << dist2_ident << ", Dist=" << dist_ident << RESET << std::endl;
            success = false;
        } else {
             std::cout << "[INFO] Distância objetos idênticos OK (0.0)." << std::endl;
        }

        // 2. Teste com objetos diferentes (distância conhecida)
         std::cout << "[TESTE] Distância entre objetos diferentes..." << std::endl;
        std::vector<double> dataC = {1.5, 3.5, 4.5}; // Diferença em [1] e [2]
        TComplexObject objC("C", 1, dataC);

        double expected_dist2 = 2.0;
        double expected_dist = 2.0;

        double dist2_diff = evaluator.GetDistance2(objA, objC);
        double dist_diff = evaluator.GetDistance(objA, objC);

        if (std::abs(dist2_diff - expected_dist2) > epsilon || std::abs(dist_diff - expected_dist) > epsilon) {
             std::cerr << VERMELHO << "[FALHA] Distância calculada incorreta. Obtida Dist2=" << dist2_diff << ", Dist=" << dist_diff
                       << ". Esperada Dist2=" << expected_dist2 << ", Dist=" << expected_dist << RESET << std::endl;
            success = false;
        } else {
             std::cout << "[INFO] Distância objetos diferentes OK." << std::endl;
        }

        // 3. Teste com vetores vazios
         std::cout << "[TESTE] Distância com vetores vazios..." << std::endl;
         std::vector<double> empty_data;
         TComplexObject obj_empty1("E1", 2, empty_data);
         TComplexObject obj_empty2("E2", 2, empty_data);
         double dist2_empty = evaluator.GetDistance2(obj_empty1, obj_empty2);
         double dist_empty = evaluator.GetDistance(obj_empty1, obj_empty2);
          if (std::abs(dist2_empty - 0.0) > epsilon || std::abs(dist_empty - 0.0) > epsilon) {
            std::cerr << VERMELHO << "[FALHA] Distância entre objetos com vetores vazios não é zero. Dist2=" << dist2_empty << ", Dist=" << dist_empty << RESET << std::endl;
            success = false;
        } else {
             std::cout << "[INFO] Distância com vetores vazios OK." << std::endl;
        }


        // 4. Teste com vetores de tamanhos diferentes (deve lançar exceção)
        std::cout << "[TESTE] Distância com vetores de tamanhos diferentes..." << std::endl;
        std::vector<double> data_short = {1.0};
        TComplexObject obj_short("S", 1, data_short);
        bool exception_caught = false;
        try {
            evaluator.GetDistance(objA, obj_short); // objA tem 3 elementos, obj_short tem 1
        } catch (const std::runtime_error& e) {
            std::cout << "[INFO] Exceção esperada capturada: " << e.what() << std::endl;
            exception_caught = true;
        } catch (...) {
             std::cerr << VERMELHO << "[FALHA] Tipo de exceção inesperado capturado." << RESET << std::endl;
             success = false;
        }
         if (!exception_caught) {
            std::cerr << VERMELHO << "[FALHA] Exceção não foi lançada para vetores de tamanhos diferentes." << RESET << std::endl;
            success = false;
        }

    } catch (const std::exception& e) {
        std::cerr << VERMELHO << "[ERRO] Exceção inesperada durante o teste de DistanceCalculator: " << e.what() << RESET << std::endl;
        success = false;
    }

    std::cout << "--- Teste TComplexObjectDistanceEvaluator Concluído: " << (success ? VERDE "SUCESSO" : VERMELHO "FALHA") << RESET << " ---" << std::endl;
    return success;
}


// --- Função Principal ---
int main() {
    std::cout << "========= INICIANDO SUÍTE DE TESTES UNITÁRIOS =========" << std::endl;

    bool all_tests_passed = true;

    // Executa cada teste e atualiza o status geral
    if (!testVectorFileReader()) {
        all_tests_passed = false;
    }
    if (!testComplexObject()) {
        all_tests_passed = false;
    }
    if (!testDistanceCalculator()) {
        all_tests_passed = false;
    }

    std::cout << "\n========= RESULTADO FINAL DA SUÍTE DE TESTES =========" << std::endl;
    if (all_tests_passed) {
        std::cout << VERDE << ">>> TODOS OS TESTES PASSARAM <<<" << RESET << std::endl;
        return 0; // Sucesso geral
    } else {
        std::cout << VERMELHO << ">>> ALGUNS TESTES FALHARAM <<<" << RESET << std::endl;
        return 1; // Falha geral
    }
}