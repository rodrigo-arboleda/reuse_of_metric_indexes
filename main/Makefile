# Makefile para Aplicação Principal, Teste Unitário e Scan Sequencial

# Compilador C++ (Usar g++ para C++)
CXX = g++

# Flags de compilação C++
# -std=c++20 : Padrão C++20
# -Wall -Wextra : Habilita a maioria dos avisos úteis (Recomendado adicionar)
# -g : Inclui informações de debug
# -O2 : Otimização (removida para melhor debug, pode adicionar para release)
CXXFLAGS = -std=c++20 -g

# --- Configuração da Aplicação Principal (Árvore Métrica) ---
APP_TARGET = Dogs
# Adicionado VectorFileReader.cpp pois app.cpp agora o utiliza
APP_SRC = main.cpp app.cpp complex_object.cpp VectorFileReader.cpp
APP_OBJS = $(APP_SRC:.cpp=.o)
# Headers da aplicação (se necessário especificar dependências)
APP_HDRS = app.h VectorFileReader.hpp # Exemplo

# Caminhos de Include/Lib para a Aplicação Principal
INCLUDEPATH = ../src/include
LIBPATH = -L../build
INCLUDE = -I$(INCLUDEPATH) -I. # Adicionado -I. para headers no diretório atual (como complex_object.h)
# LIBS para a Aplicação Principal
APP_LIBS = $(LIBPATH) -larboretum -lm

# --- Configuração do Teste Unitário ---
TEST_TARGET = unit_test
# Fontes do teste: o teste em si, o file reader, e o objeto complexo que ele usa/testa
TEST_SRC = unit_test.cpp VectorFileReader.cpp complex_object.cpp
TEST_OBJS = $(TEST_SRC:.cpp=.o)
# Headers relevantes para o teste (necessários para compilação dos .cpp)
TEST_HDRS = VectorFileReader.hpp complex_object.h distance_calculator.h

# LIBS para o Teste Unitário
TEST_LIBS = -lm

# --- Configuração da Simulação Sequencial ---
# Assumindo que o código da simulação está em sequential_scan.cpp
SEQ_TARGET = sequential_scan
SEQ_SRC = sequential_scan.cpp VectorFileReader.cpp complex_object.cpp
SEQ_OBJS = $(SEQ_SRC:.cpp=.o)
# LIBS para a Simulação Sequencial (provavelmente só precisa de -lm)
SEQ_LIBS = -lm
# Headers relevantes para a simulação (já cobertos por TEST_HDRS/APP_HDRS)
# SEQ_HDRS = VectorFileReader.hpp complex_object.h

# --- Regras ---

# Regra padrão: construir a aplicação principal
all: $(APP_TARGET)

# Regra para CONSTRUIR E EXECUTAR a Aplicação Principal
run: $(APP_TARGET)
	@echo "--- Executando Aplicação Principal: $(APP_TARGET) ---"
	./$(APP_TARGET)
	@echo "--- Execução da Aplicação Principal Concluída ---"

# Regra para CONSTRUIR E EXECUTAR o teste unitário
test: $(TEST_TARGET)
	@echo "--- Executando Teste Unitário: $(TEST_TARGET) ---"
	./$(TEST_TARGET)
	@echo "--- Teste Unitário Concluído ---"

# Regra para CONSTRUIR E EXECUTAR a simulação sequencial (NOVO)
seq: $(SEQ_TARGET)
	@echo "--- Executando Simulação de Scan Sequencial: $(SEQ_TARGET) ---"
	./$(SEQ_TARGET)
	@echo "--- Simulação de Scan Sequencial Concluída ---"

# Regra para linkar a Aplicação Principal
$(APP_TARGET): $(APP_OBJS)
	$(CXX) $(CXXFLAGS) $(APP_OBJS) -o $(APP_TARGET) $(APP_LIBS)
	@echo ">>> Aplicação Principal '$(APP_TARGET)' criada."

# Regra para linkar o Teste Unitário
$(TEST_TARGET): $(TEST_OBJS)
	$(CXX) $(CXXFLAGS) $(TEST_OBJS) -o $(TEST_TARGET) $(TEST_LIBS)
	@echo ">>> Executável de Teste '$(TEST_TARGET)' criado."

# Regra para linkar a Simulação Sequencial (NOVO)
$(SEQ_TARGET): $(SEQ_OBJS)
	$(CXX) $(CXXFLAGS) $(SEQ_OBJS) -o $(SEQ_TARGET) $(SEQ_LIBS)
	@echo ">>> Executável de Scan Sequencial '$(SEQ_TARGET)' criado."


# Regra genérica para compilar .cpp para .o
# Usa $(CXX), $(CXXFLAGS). Adiciona $(INCLUDE) para que os .cpp encontrem
# os headers necessários (tanto os locais quanto os de INCLUDEPATH).
# Adicionada dependência de headers relevantes
# Nota: $(filter %.hpp %.h, $^) pode ser usado para headers se listados como pré-requisitos
%.o: %.cpp $(TEST_HDRS) $(APP_HDRS)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $< -o $@
	@echo "   Compilado $< -> $@"

# Regra para limpar TODOS os arquivos gerados
clean:
	@echo "--- Limpando arquivos gerados ---"
	# Adicionado $(SEQ_TARGET), $(SEQ_OBJS) e o arquivo de dados da simulação
	rm -f $(APP_TARGET) $(TEST_TARGET) $(SEQ_TARGET) \
	      $(APP_OBJS) $(TEST_OBJS) $(SEQ_OBJS) \
	      *.o SlimTreeComplex.dat complex_objects_paged.dat core.*
	@echo "   Arquivos removidos."

# Declara alvos que não são arquivos reais
.PHONY: all run test seq clean