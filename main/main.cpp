//---------------------------------------------------------------------------
// citiesMain.cpp - This is a test for DBM-Tree.
//
// Author: Marcos Rodrigues Vieira (mrvieira@icmc.usp.br)
// Copyright (c) 2003 GBDI-ICMC-USP
//---------------------------------------------------------------------------
#pragma hdrstop
#include "app.h"
#include <cstdlib>

#pragma argsused

extern std::string dataset_file_var;     // Arquivo com o dataset principal
extern std::string query_file_var;    // Arquivo com os objetos de consulta
extern double range_query_var;
extern int disk_page_size;

int main(int argc, char* argv[]){
  
   if (argc >= 2) range_query_var = std::stod(argv[1]); // Converte string para double
   if (argc >= 3) dataset_file_var = argv[2];
   if (argc >= 4) query_file_var = argv[3];
   if (argc >= 5) disk_page_size = std::stoi(argv[4]);


   TApp app;                                         

   // Init application.
   app.Init();
   // Run it.
   app.Run();
   // Release resources.
   app.Done();

   return 0;
}//end main
