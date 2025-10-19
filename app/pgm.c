#include "pgm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Carrega arquivo PGM do disco para estrutura PGM
PGM *processar_entrada(FILE *arq, char *nome_arquivo) {
    PGM *img = malloc(sizeof(PGM));
    if (img == NULL){
      printf("Erro ao carregar a imagem.\n");
      return NULL;
    }
    arq = fopen(nome_arquivo, "r");
    if (arq == NULL) {
        printf("Erro ao abrir o arquivo %s.\n", nome_arquivo);
        free(img);
        return NULL;
    }
    
    // Ler magic number P2
    char magic[3];
    if (fscanf(arq, "%2s", magic) != 1 || magic[0] != 'P' || magic[1] != '2') {
        printf("Erro: Formato PGM inválido. Esperado P2.\n");
        free(img);
        fclose(arq);
        return NULL;
    }
    
    // Função para pular comentários
    char linha[256];
    int width_lido = 0, height_lido = 0, grey_lido = 0;
    
    while (!width_lido || !height_lido || !grey_lido) {
        if (fgets(linha, sizeof(linha), arq) == NULL) {
            printf("Erro: Fim de arquivo inesperado\n");
            free(img);
            fclose(arq);
            return NULL;
        }
        
        // Pular comentários
        if (linha[0] == '#') continue;
        
        // Tentar ler as dimensões
        if (!width_lido && !height_lido) {
            if (sscanf(linha, "%d %d", &img->width, &img->height) == 2) {
                width_lido = height_lido = 1;
                continue;
            }
        }
        
        // Tentar ler os níveis de cinza
        if (width_lido && height_lido && !grey_lido) {
            if (sscanf(linha, "%d", &img->grey_levels) == 1) {
                grey_lido = 1;
                break;
            }
        }
    }
    
    printf("📄 Carregando PGM: %dx%d, max=%d\n", img->width, img->height, img->grey_levels);
    
    img->pixels = malloc(img->height * sizeof(unsigned char *));
    for (int i = 0; i < img->height; i++) {
        img->pixels[i] = malloc(img->width * sizeof(unsigned char));
        for (int j = 0; j < img->width; j++) {
            int valor;
            fscanf(arq, "%d", &valor);
            img->pixels[i][j] = (unsigned char)valor;
        }
    }

    fclose(arq);
    return img;
}

// Libera memória alocada para estrutura PGM
void free_image(PGM *img) {
    for (int i = 0; i < img->height; i++)
        free(img->pixels[i]);
    free(img->pixels);
    free(img);
}

// Salva estrutura PGM em arquivo disco formato P2
void save_pgm(const char *caminho, PGM *img) {
    // Abrir em modo binário para evitar conversão automática de \n para \r\n no Windows
    FILE *arq = fopen(caminho, "wb");
    if (!arq) {
        printf("Erro ao abrir arquivo %s para escrita.\n", caminho);
        return;
    }
    
    // Cabeçalho PGM correto (usar \n explicitamente para manter compatibilidade Unix)
    if (fprintf(arq, "P2\n%d %d\n%d\n", img->width, img->height, img->grey_levels) < 0) {
        printf("Erro ao escrever cabeçalho do arquivo PGM.\n");
        fclose(arq);
        return;
    }
    
    // Escrever pixels (20 valores por linha para compatibilidade)
    int valores_por_linha = 20;
    int valores_na_linha = 0;
    
    for (int i = 0; i < img->height; i++) {
        for (int j = 0; j < img->width; j++) {
            if (fprintf(arq, "%d ", img->pixels[i][j]) < 0) {
                printf("Erro ao escrever pixels no arquivo PGM.\n");
                fclose(arq);
                return;
            }
            valores_na_linha++;
            
            // Quebrar linha a cada 20 valores
            if (valores_na_linha >= valores_por_linha) {
                if (fprintf(arq, "\n") < 0) {
                    printf("Erro ao escrever nova linha no arquivo PGM.\n");
                    fclose(arq);
                    return;
                }
                valores_na_linha = 0;
            }
        }
    }
    
    // Garantir que termina com nova linha se necessário
    if (valores_na_linha > 0) {
        if (fprintf(arq, "\n") < 0) {
            printf("Erro ao escrever nova linha final no arquivo PGM.\n");
            fclose(arq);
            return;
        }
    }
    
    // Garantir que os dados foram escritos no disco
    if (fflush(arq) != 0) {
        perror("Erro ao fazer flush do arquivo PGM");
        fclose(arq);
        return;
    }
    
    fclose(arq);
}

//Função para marcar a flag de remoção no índice correspondente
// Marca imagem como removida (soft delete)
void remove_flag_control(FILE *data_file, long offset) {
    // Agora precisamos atualizar o índice, não o arquivo de dados
    FILE *index = fopen("data/indices.bin", "r+b");
    if (!index) {
        printf("Erro ao abrir arquivo de índices!\n");
        return;
    }
    
    IndiceRecord reg;
    long index_pos = 0;
    
    // Procurar o registro com o offset correspondente
    while (fread(&reg, sizeof(IndiceRecord), 1, index)) {
        if (reg.offset == offset) {
            // Encontrou o registro, marcar como removido
            reg.is_removed = 1;
            fseek(index, index_pos, SEEK_SET);
            fwrite(&reg, sizeof(IndiceRecord), 1, index);
            fclose(index);
            return;
        }
        index_pos = ftell(index);
    }
    
    fclose(index);
    printf("Registro não encontrado no índice!\n");
}

//Função para restaurar a flag de remoção no índice correspondente
void restore_flag_control(FILE *data_file, long offset) {
    // Agora precisamos atualizar o índice, não o arquivo de dados
    FILE *index = fopen("data/indices.bin", "r+b");
    if (!index) {
        printf("Erro ao abrir arquivo de índices!\n");
        return;
    }
    
    IndiceRecord reg;
    long index_pos = 0;
    
    // Procurar o registro com o offset correspondente
    while (fread(&reg, sizeof(IndiceRecord), 1, index)) {
        if (reg.offset == offset) {
            // Encontrou o registro, marcar como ativo
            reg.is_removed = 0;
            fseek(index, index_pos, SEEK_SET);
            fwrite(&reg, sizeof(IndiceRecord), 1, index);
            fclose(index);
            return;
        }
        index_pos = ftell(index);
    }
    
    fclose(index);
    printf("Registro não encontrado no índice!\n");
}

// Função que lê a imagem do binario RLE e restaura a imagem PGM original
void restore_image_from_bin(const char *bin_file_name, const char *output_pgm_name) {
    FILE *bin_file = fopen(bin_file_name, "rb");
    if (!bin_file) {
        perror("Erro ao abrir o arquivo binário para leitura");
        return;
    }

    PGM img;
    // Ler dimensões e grey_levels (pode ser negativo = RLE, positivo = RAW)
    fread(&img.width, sizeof(int), 1, bin_file);
    fread(&img.height, sizeof(int), 1, bin_file);
    int stored_grey;
    fread(&stored_grey, sizeof(int), 1, bin_file);

    img.pixels = malloc(img.height * sizeof(unsigned char *));
    for (int i = 0; i < img.height; i++) {
        img.pixels[i] = malloc(img.width * sizeof(unsigned char));
    }

    // Detectar formato: negativo = RLE, positivo = RAW
    if (stored_grey < 0) {
        // Formato RLE
        img.grey_levels = -stored_grey;
        
        for (int i = 0; i < img.height; i++) {
            if (img.width > 0) {
                int j = 0;
                unsigned char primeiro_pixel;
                fread(&primeiro_pixel, sizeof(unsigned char), 1, bin_file);
                unsigned char pixel_atual = primeiro_pixel;
                
                while (j < img.width) {
                    int count;
                    fread(&count, sizeof(int), 1, bin_file);
                    
                    for (int k = 0; k < count && j < img.width; k++, j++) {
                        img.pixels[i][j] = pixel_atual;
                    }
                    
                    pixel_atual = (pixel_atual == 0) ? img.grey_levels : 0;
                }
            }
        }
    } else {
        // Formato RAW - ler pixels diretamente
        img.grey_levels = stored_grey;
        
        for (int i = 0; i < img.height; i++) {
            fread(img.pixels[i], sizeof(unsigned char), img.width, bin_file);
        }
    }
    
    fclose(bin_file);
    save_pgm(output_pgm_name, &img);

    for (int i = 0; i < img.height; i++) {
        free(img.pixels[i]);
    }
    free(img.pixels);
}

// Calcula imagem média de versões limiarizadas para restauração
PGM *calcular_imagem_media(const char *nome_base, const char *data_file, const char *index_file) {
    FILE *index = fopen(index_file, "rb");
    if (!index) {
        printf("Erro ao abrir arquivo de índices!\n");
        return NULL;
    }
    
    FILE *data = fopen(data_file, "rb");
    if (!data) {
        printf("Erro ao abrir arquivo de dados!\n");
        fclose(index);
        return NULL;
    }
    
    printf("Buscando versões limiarizadas de '%s'...\n", nome_base);
    
    // Arrays para armazenar informações das imagens encontradas
    long offsets[50];
    int limiares[50];
    int versoes_encontradas = 0;
    int width = 0, height = 0, grey_levels = 0;
    
    // Buscar todas as versões da imagem base
    IndiceRecord reg;
    while (fread(&reg, sizeof(IndiceRecord), 1, index)) {
        if (!reg.is_removed) {
            // Verificar se é uma versão limiarizada da imagem base
            if (strstr(reg.name, nome_base) != NULL && strstr(reg.name, "_RLE_L") != NULL) {
                // Extrair limiar do nome (formato: nome_RLE_L{limiar})
                char *pos_l = strstr(reg.name, "_RLE_L");
                if (pos_l) {
                    int limiar = atoi(pos_l + 6);
                    offsets[versoes_encontradas] = reg.offset;
                    limiares[versoes_encontradas] = limiar;
                    versoes_encontradas++;
                    
                    printf("    Encontrada: %s (Limiar: %d)\n", reg.name, limiar);
                    
                    // Ler dimensões da primeira imagem para validação
                    if (versoes_encontradas == 1) {
                        fseek(data, reg.offset, SEEK_SET);
                        fread(&width, sizeof(int), 1, data);
                        fread(&height, sizeof(int), 1, data);
                        fread(&grey_levels, sizeof(int), 1, data);
                    }
                }
            }
        }
    }
    
    if (versoes_encontradas == 0) {
        printf(" Nenhuma versão limiarizada encontrada para '%s'\n", nome_base);
        fclose(index);
        fclose(data);
        return NULL;
    }
    
    printf(" Total de versões encontradas: %d\n", versoes_encontradas);
    printf(" Dimensões: %dx%d\n", width, height);
    
    // Alocar matriz para soma dos pixels
    double **soma_pixels = malloc(height * sizeof(double*));
    for (int i = 0; i < height; i++) {
        soma_pixels[i] = calloc(width, sizeof(double));
    }
    
    // Processar cada versão encontrada
    for (int v = 0; v < versoes_encontradas; v++) {
        printf(" Processando versão %d/%d (Limiar: %d)...\n", 
               v + 1, versoes_encontradas, limiares[v]);
        
        fseek(data, offsets[v], SEEK_SET);
        
        // Pular cabeçalho
        int w, h, gl;
        fread(&w, sizeof(int), 1, data);
        fread(&h, sizeof(int), 1, data);
        fread(&gl, sizeof(int), 1, data);
        
        // Descomprimir RLE e somar pixels
        if (gl < 0) { // Formato RLE
            int grey_original = -gl;
            
            for (int i = 0; i < height; i++) {
                unsigned char primeiro_pixel;
                fread(&primeiro_pixel, sizeof(unsigned char), 1, data);
                
                int coluna = 0;
                unsigned char pixel_atual = primeiro_pixel;
                
                while (coluna < width) {
                    int count;
                    fread(&count, sizeof(int), 1, data);
                    
                    // Somar pixels da sequência atual
                    for (int c = 0; c < count && coluna < width; c++) {
                        soma_pixels[i][coluna] += (double)pixel_atual;
                        coluna++;
                    }
                    
                    // Alternar pixel (0 ↔ grey_original)
                    pixel_atual = (pixel_atual == 0) ? grey_original : 0;
                }
            }
        }
    }
    
    // Calcular média e criar imagem resultado
    PGM *img_media = malloc(sizeof(PGM));
    img_media->width = width;
    img_media->height = height;
    img_media->grey_levels = 255; // Resultado sempre em escala 0-255
    
    img_media->pixels = malloc(height * sizeof(unsigned char*));
    for (int i = 0; i < height; i++) {
        img_media->pixels[i] = malloc(width * sizeof(unsigned char));
    }
    
    printf(" Calculando média pixel a pixel...\n");
    
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            double media = soma_pixels[i][j] / versoes_encontradas;
            img_media->pixels[i][j] = (unsigned char)media;
        }
    }
    
    // Liberar memória da soma
    for (int i = 0; i < height; i++) {
        free(soma_pixels[i]);
    }
    free(soma_pixels);
    
    fclose(index);
    fclose(data);
    
    printf(" Imagem média calculada com sucesso!\n");
    printf(" Fórmula aplicada: I^R = Σ(I^k) / n, onde n = %d\n", versoes_encontradas);
    
    return img_media;
}

