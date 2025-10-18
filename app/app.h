#ifndef APP_H
#define APP_H

#include "../models/structure.h"
#include "pgm.h"
#include "bin.h"
#include "keys.h"
#include "pgm_operations.h"
#include "manage_data.h"

// Função principal do sistema
void app_call();

// Funções do menu
void exibir_menu();
void limpar_tela();
void pausar();

// Opções do menu (usando funções modulares existentes)
void opcao_carregar_imagem();      // Carregar do banco de dados
void opcao_limiarizacao_e_salvar(); // Salvar no banco de dados  
void opcao_negativo();              // Deletar imagem (flag)
void opcao_compactar();             // Comprimir arquivo
void opcao_listar_imagens();        // Listar imagens no banco
void opcao_calcular_media();        // Calcular imagem média (restaurar)
void opcao_sobre();                 // Informações do programa

#endif 