#include "app.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

PGM *imagem_atual = NULL;
char data_file[] = "data/imagens.bin";
char index_file[] = "data/indices.bin";

// Limpa a tela do terminal
void limpar_tela() {
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
}

// Pausa execução aguardando Enter
void pausar() {
    printf("\nPressione Enter para continuar...");
    getchar();
}

// Exibe menu principal da aplicação
void exibir_menu() {
    limpar_tela();
    printf("===============================================\n");
    printf("   BANCO DE DADOS DE IMAGENS PGM - ED2      \n");
    printf("===============================================\n");
    printf("1. Carregar Imagem do Banco de Dados\n");
    printf("2. Salvar Imagem no Banco de Dados\n");
    printf("3. Deletar Imagem (flag de remoção)\n");
    printf("4. Comprimir Arquivo (remover deletados)\n");
    printf("5. Listar Imagens no Banco\n");
    printf("6. Calcular Imagem Média (Restaurar)\n");
    printf("7. Informações do Programa (Sobre)\n");
    printf("8. Sair\n");
    printf("===============================================\n");
    printf("Escolha uma opção: ");
}

// Carrega imagem do banco de dados para memória
void opcao_carregar_imagem() {
    char nome_busca[100];
    printf("Digite o nome da imagem para buscar: ");
    
    if (fgets(nome_busca, sizeof(nome_busca), stdin) == NULL) {
        printf("❌ Erro ao ler entrada!\n");
        pausar();
        return;
    }
    
    // Remover newline se presente
    size_t len = strlen(nome_busca);
    if (len > 0 && nome_busca[len-1] == '\n') {
        nome_busca[len-1] = '\0';
    }
    
    FILE *index = fopen(index_file, "rb");
    if (!index) {
        printf("❌ Arquivo de índices não encontrado!\n");
        pausar();
        return;
    }
    
    long offset = buscar_offset_por_nome(index, nome_busca);
    fclose(index);
    
    if (offset == -1) {
        printf("❌ Imagem '%s' não encontrada no banco!\n", nome_busca);
        pausar();
        return;
    }
    
    if (imagem_atual != NULL) {
        free_image(imagem_atual);
        imagem_atual = NULL;
    }
    
    imagem_atual = malloc(sizeof(PGM));
    if (!imagem_atual) {
        printf("❌ Erro ao alocar memória para a imagem!\n");
        pausar();
        return;
    }
    
    read_compress_image(data_file, imagem_atual, &offset);
    
    // Verificar se a imagem foi carregada com sucesso
    if (imagem_atual->width <= 0 || imagem_atual->height <= 0) {
        printf("❌ Erro ao carregar imagem do banco!\n");
        free(imagem_atual);
        imagem_atual = NULL;
        pausar();
        return;
    }
    
    printf("✅ Imagem '%s' carregada do banco!\n", nome_busca);
    printf("Dimensões: %dx%d, Níveis de cinza: %d\n", 
           imagem_atual->width, imagem_atual->height, imagem_atual->grey_levels);
    
    // Menu de operações
    int opcao_op;
    printf("\n--- OPERAÇÕES DISPONÍVEIS ---\n");
    printf("1. Aplicar Negativação\n");
    printf("2. Aplicar Limiarização\n");
    printf("3. Salvar no disco local (PGM)\n");
    printf("4. Voltar ao menu principal\n");
    printf("Escolha uma operação: ");
    scanf("%d", &opcao_op);
    
    switch(opcao_op) {
        case 1: {
            negativar(imagem_atual);
            printf("✅ Negativação aplicada!\n");
            
            // Perguntar se deseja salvar
            char save_choice;
            printf("Deseja salvar a imagem negativada no banco? (s/n): ");
            scanf(" %c", &save_choice);
            
            if (save_choice == 's' || save_choice == 'S') {
                char nome_neg[150];
                printf("Digite um nome para a imagem negativada: ");
                scanf("%149s", nome_neg);
                
                long raw_offset;
                save_raw_v2(imagem_atual, data_file, &raw_offset);
                
                FILE *idx = fopen(index_file, "ab");
                if (idx) {
                    IndiceRecord reg;
                    snprintf(reg.name, max_name, "%s_NEG", nome_neg);
                    reg.offset = raw_offset;
                    reg.is_removed = 0;
                    adicionar_indice(idx, reg);
                    fclose(idx);
                    printf(" Imagem negativada salva como: %s\n", reg.name);
                } else {
                    printf(" Erro ao abrir arquivo de índices!\n");
                }
            } else {
                printf("Negativação aplicada apenas na memória (não salva no banco)\n");
            }
            break;
        }
            
        case 2: {
            int limiar;
            printf("Digite o valor do limiar (0-%d): ", imagem_atual->grey_levels);
            scanf("%d", &limiar);
            limiarizar(imagem_atual, limiar);
            printf(" Limiarização aplicada (L=%d)!\n", limiar);
            
            // Informar sobre compactação RLE antes do input
            printf("\n>> Esta versão será automaticamente compactada pelo algoritmo RLE (Run-Length Encoding)\n");
            printf(">> RLE é otimizado para imagens binárias como as geradas pela limiarização\n");
            
            // Salvar automaticamente com compressão RLE
            char nome_rle[150];
            printf("Digite um nome para a versão limiarizada: ");
            fflush(stdout);
            scanf("%149s", nome_rle);
            
            printf(">> Processando compactação RLE...\n");
            fflush(stdout);
            
            // Salvar com RLE usando função modular
            long rle_offset;
            RLE_compress_v2(imagem_atual, data_file, &rle_offset);
            
            // Adicionar ao índice usando função modular
            FILE *idx = fopen(index_file, "ab");
            if (idx) {
                IndiceRecord reg;
                snprintf(reg.name, max_name, "%s_RLE_L%d", nome_rle, limiar);
                reg.offset = rle_offset;
                adicionar_indice(idx, reg); // Função modular
                fclose(idx);
                printf(" Imagem limiarizada salva com RLE como: %s\n", reg.name);
            }
            break;
        }
        
        case 3: {
            char caminho[256];
            printf("Digite o nome do arquivo PGM de saída: ");
            scanf("%255s", caminho);
            save_pgm(caminho, imagem_atual);
            printf(" Imagem salva em '%s'\n", caminho);
            break;
        }
        
        case 4:
            printf("Voltando ao menu principal...\n");
            break;
            
        default:
            printf(" Opção inválida!\n");
    }
    
    pausar();
}

// Carrega imagem do disco e salva no banco de dados
void opcao_limiarizacao_e_salvar() {
    char caminho[512];  // Aumentar tamanho para caminhos longos
    printf("Digite o caminho do arquivo PGM para carregar: ");
    
    // Usar fgets em vez de scanf para permitir espaços no caminho
    if (fgets(caminho, sizeof(caminho), stdin) != NULL) {
        // Remover nova linha do final se presente
        size_t len = strlen(caminho);
        if (len > 0 && caminho[len-1] == '\n') {
            caminho[len-1] = '\0';
        }
    } else {
        printf(" Erro ao ler caminho do arquivo!\n");
        pausar();
        return;
    }
    

    PGM *nova_imagem = processar_entrada(NULL, caminho);
    if (nova_imagem == NULL) {
        printf("❌ Erro ao carregar a imagem '%s'!\n", caminho);
        pausar();
        return;
    }
    
    printf(" Imagem carregada: %dx%d, Níveis: %d\n", 
           nova_imagem->width, nova_imagem->height, nova_imagem->grey_levels);
    
    // Perguntar posição no banco
    int posicao;
    printf("Posição no banco de dados:\n");
    printf("0 - Final do arquivo (append)\n");
    printf("1 - Início do arquivo (top)\n");
    printf("Escolha: ");
    scanf("%d", &posicao);
    getchar(); // Limpar o '\n' deixado pelo scanf
    
    // Perguntar nome para indexação
    char nome_indice[100];
    printf("Digite um nome para indexar esta imagem: ");
    fflush(stdout); // Garantir que o prompt seja exibido
    
    if (fgets(nome_indice, sizeof(nome_indice), stdin) != NULL) {
        // Remover nova linha do final se presente
        size_t len = strlen(nome_indice);
        if (len > 0 && nome_indice[len-1] == '\n') {
            nome_indice[len-1] = '\0';
        }
        
        // Verificar se o nome não está vazio
        if (strlen(nome_indice) == 0) {
            strcpy(nome_indice, "imagem_sem_nome");
            printf(" Nome vazio detectado, usando: '%s'\n", nome_indice);
        }
    } else {
        strcpy(nome_indice, "imagem_sem_nome");
        printf(" Erro ao ler nome, usando: '%s'\n", nome_indice);
    }
    
    append_image_to_data_file(data_file, nova_imagem, posicao, nome_indice);
    
    printf("Imagem '%s' salva no banco!\n", nome_indice);
    printf("Posição: %s\n", (posicao == 0) ? "Final" : "Início");
    
    free_image(nova_imagem);
    pausar();
}

// Marca imagem para deleção (soft delete)
void opcao_negativo() {
    char nome_busca[100];
    printf("Digite o nome da imagem para deletar: ");
    
    if (fgets(nome_busca, sizeof(nome_busca), stdin) != NULL) {
        // Remover nova linha do final se presente
        size_t len = strlen(nome_busca);
        if (len > 0 && nome_busca[len-1] == '\n') {
            nome_busca[len-1] = '\0';
        }
    } else {
        printf("Erro ao ler nome da imagem!\n");
        pausar();
        return;
    }
    
    FILE *index = fopen(index_file, "rb");
    if (!index) {
        printf("Arquivo de índices não encontrado!\n");
        pausar();
        return;
    }
    
    long offset = buscar_offset_por_nome(index, nome_busca); // Função modular
    fclose(index);
    
    if (offset == -1) {
        printf("Imagem '%s' não encontrada no banco!\n", nome_busca);
        pausar();
        return;
    }
    
    // Confirmar deleção
    char confirmacao;
    printf("Confirma a deleção da imagem '%s'? (s/n): ", nome_busca);
    scanf(" %c", &confirmacao);
    
    if (confirmacao == 's' || confirmacao == 'S') {
        FILE *data = fopen(data_file, "r+b");
        if (data) {
            remove_flag_control(data, offset); // Função modular
            fclose(data);
            printf("Imagem '%s' marcada para deleção!\n", nome_busca);
            printf("Use a opção 4 para comprimir e remover definitivamente.\n");
        } else {
            printf("Erro ao acessar banco de dados!\n");
        }
    } else {
        printf("Deleção cancelada.\n");
    }
    
    pausar();
}

void opcao_buscar_imagem() {
    char nome[100];
    int limiar;
    printf("Digite o nome da imagem: ");
    scanf("%99s", nome);
    printf("Digite o limiar usado: ");
    scanf("%d", &limiar);
    
    char identificador[150];
    snprintf(identificador, sizeof(identificador), "%s_L%d", nome, limiar);
    
    FILE *index = fopen(index_file, "rb");
    if (!index) {
        printf("Arquivo de índices não encontrado!\n");
        pausar();
        return;
    }
    
    long offset = buscar_offset_por_nome(index, identificador);
    fclose(index);
    
    if (offset == -1) {
        printf("Imagem '%s' não encontrada!\n", identificador);
        pausar();
        return;
    }
    
    // Liberar imagem atual
    if (imagem_atual != NULL) {
        free_image(imagem_atual);
    }
    
    // Carregar imagem comprimida
    imagem_atual = malloc(sizeof(PGM));
    read_compress_image(data_file, imagem_atual, &offset);
    
    printf("Imagem '%s' carregada!\n", identificador);
    printf("Dimensões: %dx%d\n", imagem_atual->width, imagem_atual->height);
    pausar();
}

void opcao_remover_imagem() {
    char nome[100];
    int limiar;
    printf("Digite o nome da imagem: ");
    scanf("%99s", nome);
    printf("Digite o limiar: ");
    scanf("%d", &limiar);
    
    char identificador[150];
    snprintf(identificador, sizeof(identificador), "%s_L%d", nome, limiar);
    
    FILE *index = fopen(index_file, "rb");
    if (!index) {
        printf("Arquivo de índices não encontrado!\n");
        pausar();
        return;
    }
    
    long offset = buscar_offset_por_nome(index, identificador);
    fclose(index);
    
    if (offset == -1) {
        printf("Imagem não encontrada!\n");
        pausar();
        return;
    }
    
    FILE *data = fopen(data_file, "r+b");
    if (data) {
        remove_flag_control(data, offset);
        fclose(data);
        printf("Imagem '%s' marcada como removida\n", identificador);
    } else {
        printf("Erro ao acessar arquivo de dados!\n");
    }
    pausar();
}

void opcao_restaurar_imagem() {
    char nome[100];
    int limiar;
    printf("Digite o nome da imagem: ");
    scanf("%99s", nome);
    printf("Digite o limiar: ");
    scanf("%d", &limiar);
    
    char identificador[150];
    snprintf(identificador, sizeof(identificador), "%s_L%d", nome, limiar);
    
    FILE *index = fopen(index_file, "rb");
    if (!index) {
        printf("Arquivo de índices não encontrado!\n");
        pausar();
        return;
    }
    
    long offset = buscar_offset_por_nome(index, identificador);
    fclose(index);
    
    if (offset == -1) {
        printf("Imagem não encontrada!\n");
        pausar();
        return;
    }
    
    FILE *data = fopen(data_file, "r+b");
    if (data) {
        restore_flag_control(data, offset);
        fclose(data);
        printf("Imagem '%s' restaurada\n", identificador);
    } else {
        printf("Erro ao acessar arquivo de dados!\n");
    }
    pausar();
}

// Compacta banco removendo imagens deletadas
void opcao_compactar() {
    printf("Iniciando compactação do banco de dados...\n");
    char temp_file[] = "data/temp.bin";
    
    // Usar função modular para compactar
    compress_data_file(data_file, temp_file);
    
    // Substituir arquivo original
    remove(data_file);
    rename(temp_file, data_file);
    
    printf("Compactação concluída!\n");
    printf("Todas as imagens com flag de deleção foram removidas.\n");
    pausar();
}

void opcao_sobre() {
    limpar_tela();
    printf("===============================================\n");
    printf("        INFORMAÇÕES DO PROGRAMA               \n");
    printf("===============================================\n");
    printf("PROJETO 1 - ESTRUTURA DE DADOS 2\n");
    printf("Aluno: Luiz Gustavo Dacome Damas\n");
    printf("Turma: Noturno\n");
    printf("Professor: Emilio Bergamim Júnior\n");
    printf("\n");
    printf("FUNCIONALIDADES:\n");
    printf("• Banco de dados de imagens PGM\n");
    printf("• Compressão RLE (Run-Length Encoding)\n");
    printf("• Sistema de indexação por nome\n");
    printf("• Operações: Limiarização e Negativação\n");
    printf("• Gestão de dados com soft delete\n");
    printf("• Compactação automática do banco\n");
    printf("\n");
    printf("ARQUITETURA MODULAR:\n");
    printf("• /app - Operações da aplicação\n");
    printf("• /models - Estruturas de dados\n");
    printf("• /data - Banco de dados binário\n");
    printf("• /docs - Documentação\n");
    printf("\n");
    printf("TECNOLOGIAS:\n");
    printf("• Linguagem C\n");
    printf("• Estruturas de dados dinâmicas\n");
    printf("• Manipulação de arquivos binários\n");
    printf("• Algoritmos de compressão\n");
    printf("===============================================\n");
    pausar();
}

void opcao_exportar_pgm() {
    if (imagem_atual == NULL) {
        printf("❌ Nenhuma imagem carregada!\n");
        pausar();
        return;
    }
    
    char caminho[256];
    printf("Digite o nome do arquivo de saída (.pgm): ");
    scanf("%255s", caminho);
    
    save_pgm(caminho, imagem_atual);
    printf("Imagem exportada para '%s'\n", caminho);
    pausar();
}

void opcao_salvar_binario() {
    if (imagem_atual == NULL) {
        printf("Nenhuma imagem carregada!\n");
        pausar();
        return;
    }
    
    char caminho[256];
    printf("Digite o nome do arquivo binário: ");
    scanf("%255s", caminho);
    
    FILE *arq = fopen(caminho, "wb");
    if (arq) {
        int offset = salvar_imagem_binario(arq, imagem_atual);
        fclose(arq);
        printf("Imagem salva em binário (offset: %d)\n", offset);
    } else {
        printf("Erro ao criar arquivo!\n");
    }
    pausar();
}

void opcao_carregar_binario() {
    char caminho[256];
    long offset;
    printf("Digite o nome do arquivo binário: ");
    scanf("%255s", caminho);
    printf("Digite o offset: ");
    scanf("%ld", &offset);
    
    FILE *arq = fopen(caminho, "rb");
    if (arq) {
        if (imagem_atual != NULL) {
            free_image(imagem_atual);
        }
        imagem_atual = ler_imagem_binario(arq, offset);
        fclose(arq);
        printf("Imagem carregada do binário!\n");
    } else {
        printf("Erro ao abrir arquivo!\n");
    }
    pausar();
}

void opcao_append_banco() {
    if (imagem_atual == NULL) {
        printf("Nenhuma imagem carregada!\n");
        pausar();
        return;
    }
    
    int posicao;
    printf("Posição (0=final, 1=início): ");
    scanf("%d", &posicao);
    
    char nome_imagem[100];
    printf("Digite um nome para a imagem: ");
    scanf("%99s", nome_imagem);
    
    append_image_to_data_file(data_file, imagem_atual, posicao, nome_imagem);
    printf("Imagem '%s' adicionada ao banco!\n", nome_imagem);
    pausar();
}

void opcao_restaurar_rle() {
    char arquivo_rle[256], arquivo_saida[256];
    printf("Digite o arquivo RLE: ");
    scanf("%255s", arquivo_rle);
    printf("Digite o arquivo de saída (.pgm): ");
    scanf("%255s", arquivo_saida);
    
    restore_image_from_bin(arquivo_rle, arquivo_saida);
    printf("Imagem restaurada para '%s'\n", arquivo_saida);
    pausar();
}

void opcao_reconstruir_bonus() {
    printf("FUNCIONALIDADE BONUS - EM DESENVOLVIMENTO\n");
    printf("Esta função criaria a média de múltiplas versões binárias\n");
    printf("de uma mesma imagem para reconstruir a original.\n");
    pausar();
}

void opcao_listar_imagens() {
    FILE *index = fopen(index_file, "rb");
    if (!index) {
        printf("Arquivo de índices não encontrado!\n");
        pausar();
        return;
    }
    
    printf("\n=== IMAGENS NO BANCO DE DADOS ===\n");
    IndiceRecord reg;
    int contador = 0;
    int ativas = 0;
    
    while (fread(&reg, sizeof(IndiceRecord), 1, index)) {
        printf("%d. %s (offset: %ld) %s\n", ++contador, reg.name, reg.offset,
               reg.is_removed ? "[DELETADA]" : "[ATIVA]");
        if (!reg.is_removed) ativas++;
    }
    
    if (contador == 0) {
        printf("Nenhuma imagem encontrada.\n");
    } else {
        printf("Total: %d imagens (%d ativas, %d deletadas)\n", contador, ativas, contador - ativas);
    }
    
    fclose(index);
    pausar();
}

void app_call() {
    int opcao;
    
    printf("=== BANCO DE DADOS DE IMAGENS PGM ===\n");
    printf("Inicializando sistema...\n");
    
    // Criar diretório data se não existir (Windows)
    system("if not exist data mkdir data");
    
    printf("Sistema inicializado com sucesso!\n");
    
    do {
        exibir_menu();
        
        if (scanf("%d", &opcao) != 1) {
            printf("Entrada inválida! Digite um número.\n");
            // Limpar buffer de entrada
            int c;
            while ((c = getchar()) != '\n' && c != EOF);
            pausar();
            continue;
        }
        
        // Limpar buffer após leitura bem-sucedida (necessário após scanf)
        getchar();
        
        switch(opcao) {
            case 1:
                opcao_carregar_imagem();  // Carregar do banco
                break;
            case 2:
                opcao_limiarizacao_e_salvar();  // Salvar no banco
                break;
            case 3:
                opcao_negativo();  // Deletar imagem
                break;
            case 4:
                opcao_compactar();  // Comprimir arquivo
                break;
            case 5:
                opcao_listar_imagens();  // Listar imagens
                break;
            case 6:
                opcao_calcular_media();  // Calcular imagem média
                break;
            case 7:
                opcao_sobre();  // Informações do programa
                break;
            case 8:
                printf("Finalizando sistema...\n");
                if (imagem_atual != NULL) {
                    free_image(imagem_atual);
                    imagem_atual = NULL;
                }
                printf("Sistema finalizado com sucesso!\n");
                break;
            default:
                printf("Opção inválida!\n");
                pausar();
        }
    } while(opcao != 8);
}

void opcao_calcular_media() {
    limpar_tela();
    printf("===============================================\n");
    printf("      CALCULAR IMAGEM MÉDIA (RESTAURAR)      \n");
    printf("===============================================\n");
    printf("\n");
    printf("  AVISO IMPORTANTE:\n");
    printf("Esta opção é destinada para um cenário específico onde o\n");
    printf("banco contém APENAS uma mesma imagem original que foi\n");
    printf("limiarizada de diferentes formas.\n");
    printf("\n");
    printf(" FINALIDADE:\n");
    printf("• Restaurar a imagem original baseada na média das versões\n");
    printf("• Útil para recuperar informações perdidas na limiarização\n");
    printf("• Funciona melhor com múltiplas versões da mesma imagem\n");
    printf("\n");
    printf(" ALGORITMO:\n");
    printf("Calcula a média aritmética pixel por pixel de todas as\n");
    printf("versões limiarizadas: I^R = Σ(I^k) / n\n");
    printf("\n");
    printf(" CRITÉRIO DE BUSCA:\n");
    printf("Busca imagens que contenham o PREFIXO informado + '_RLE_L'\n");
    printf("Exemplo: 'baboon' encontrará:\n");
    printf("  ✓ baboon_RLE_L100, baboon_RLE_L150, baboon_teste_RLE_L200\n");
    printf("  ✗ baboon (sem RLE), cat_RLE_L100 (prefixo diferente)\n");
    printf("\n");
    printf("===============================================\n");
    
    char nome_base[100];
    printf("Digite o PREFIXO do nome da imagem (ex: 'baboon'): ");
    scanf("%99s", nome_base);
    
    printf("\n Iniciando processo de restauração...\n");
    
    // Calcular imagem média usando função modular
    PGM *img_restaurada = calcular_imagem_media(nome_base, data_file, index_file);
    
    if (img_restaurada) {
        // Salvar imagem restaurada
        char nome_saida[200];
        snprintf(nome_saida, sizeof(nome_saida), "%s_RESTAURADA.ascii.pgm", nome_base);
        
        printf("\n Salvando imagem restaurada como: %s\n", nome_saida);
        save_pgm(nome_saida, img_restaurada);
        
        printf(" Imagem média salva com sucesso!\n");
        printf(" A imagem restaurada combina informações de todas as versões limiarizadas\n");
        
        // Perguntar se quer salvar no banco também
        char salvar_banco;
        printf("\nDeseja salvar a imagem restaurada no banco de dados? (s/n): ");
        scanf(" %c", &salvar_banco);
        
        if (salvar_banco == 's' || salvar_banco == 'S') {
            // Salvar no banco usando função modular
            long offset_restaurada;
            save_raw_v2(img_restaurada, data_file, &offset_restaurada);
            
            FILE *idx = fopen(index_file, "ab");
            if (idx) {
                IndiceRecord reg;
                snprintf(reg.name, max_name, "%s_MEDIA_RESTAURADA", nome_base);
                reg.offset = offset_restaurada;
                reg.is_removed = 0;
                adicionar_indice(idx, reg);
                fclose(idx);
                printf(" Imagem restaurada também salva no banco como: %s\n", reg.name);
            }
        }
        
        free_image(img_restaurada);
    } else {
        printf(" Falha na restauração da imagem.\n");
        printf(" Verifique se existem versões limiarizadas da imagem '%s' no banco.\n", nome_base);
    }
    
    printf("\n");
    pausar();
}
