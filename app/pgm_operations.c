//Operações solicitadas pelo enunciado
#include <stdio.h>
#include <stdlib.h>
#include "pgm_operations.h"

//Atribuir a lógica de binario para a compressão do tipo RLE
void limiarizar(PGM *img, int L) {
    for (int i = 0; i < img->height; i++)
        for (int j = 0; j < img->width; j++)
            img->pixels[i][j] = (img->pixels[i][j] < L) ? 0 : img->grey_levels;
}
// Implementa o efeito negativo na imagem PGM
void negativar(PGM *img) {
    for (int i = 0; i < img->height; i++)
        for (int j = 0; j < img->width; j++)
            img->pixels[i][j] = img->grey_levels - img->pixels[i][j];
}

// Função que permite a compressão RLE após o processo de limiarização
// Formato correto conforme enunciado: [primeiro_pixel, count1, count2, count3...]
void RLE_compress(PGM *img, const char *bin_file_name){
    FILE *bin_file = fopen(bin_file_name, "wb");
    if (!bin_file) {
        perror("Erro ao abrir o arquivo binário para escrita");
        return;
    }

    // Salvar dimensões e dados
    // Usar grey_levels NEGATIVO como marcador de RLE
    fwrite(&img->width, sizeof(int), 1, bin_file);
    fwrite(&img->height, sizeof(int), 1, bin_file);
    int rle_marker = -(img->grey_levels); // Negativo indica RLE
    fwrite(&rle_marker, sizeof(int), 1, bin_file);

    // Para cada linha da imagem
    for (int i = 0; i < img->height; i++) {
        if (img->width > 0) {
            // Escreve o primeiro pixel da linha
            unsigned char primeiro_pixel = img->pixels[i][0];
            fwrite(&primeiro_pixel, sizeof(unsigned char), 1, bin_file);
            
            // Agora processa a sequência de counts
            unsigned char pixel_atual = primeiro_pixel;
            int count = 1;
            
            for (int j = 1; j < img->width; j++) {
                if (img->pixels[i][j] == pixel_atual) {
                    count++;
                } else {
                    // Escreve o count do pixel atual
                    fwrite(&count, sizeof(int), 1, bin_file);
                    // Muda para o próximo pixel (alternância automática entre 0 e grey_levels)
                    pixel_atual = img->pixels[i][j];
                    count = 1;
                }
            }
            // Escreve o último count
            fwrite(&count, sizeof(int), 1, bin_file);
        }
    }
    fclose(bin_file);
}

void RLE_compress_v2(PGM *img, const char *bin_file_name, long *offset){
    // Se a imagem não é binária (tem tons de cinza), salvar em formato RAW
    if (!is_binary_image(img)) {
        printf("  Imagem não é binária, salvando em formato RAW\n");
        save_raw_v2(img, bin_file_name, offset);
        return;
    }
    printf(" Imagem é binária, aplicando compressão RLE!\n");
    
    FILE *bin_file = fopen(bin_file_name, "ab");
    if (!bin_file) {
        perror("Erro ao abrir o arquivo binário para escrita");
        *offset = -1;
        return;
    }

    // Garantir que estamos no final do arquivo e obter posição
    fseek(bin_file, 0, SEEK_END);
    *offset = ftell(bin_file); //Guarda o offset atual do arquivo

    // Salvar dimensões com marcador RLE (grey_levels negativo)
    if (fwrite(&img->width, sizeof(int), 1, bin_file) != 1 ||
        fwrite(&img->height, sizeof(int), 1, bin_file) != 1) {
        perror("Erro ao escrever dimensões da imagem");
        fclose(bin_file);
        *offset = -1;
        return;
    }
    
    int rle_marker = -(img->grey_levels); // Negativo indica RLE
    if (fwrite(&rle_marker, sizeof(int), 1, bin_file) != 1) {
        perror("Erro ao escrever marcador RLE");
        fclose(bin_file);
        *offset = -1;
        return;
    }

    // Para cada linha da imagem
    for (int i = 0; i < img->height; i++) {
        if (img->width > 0) {
            // Escreve o primeiro pixel da linha
            unsigned char primeiro_pixel = img->pixels[i][0];
            if (fwrite(&primeiro_pixel, sizeof(unsigned char), 1, bin_file) != 1) {
                perror("Erro ao escrever primeiro pixel");
                fclose(bin_file);
                *offset = -1;
                return;
            }
            
            // Agora processa a sequência de counts
            unsigned char pixel_atual = primeiro_pixel;
            int count = 1;
            
            for (int j = 1; j < img->width; j++) {
                if (img->pixels[i][j] == pixel_atual) {
                    count++;
                } else {
                    // Escreve o count do pixel atual
                    if (fwrite(&count, sizeof(int), 1, bin_file) != 1) {
                        perror("Erro ao escrever contagem RLE");
                        fclose(bin_file);
                        *offset = -1;
                        return;
                    }
                    // Muda para o próximo pixel (alternância automática)
                    pixel_atual = img->pixels[i][j];
                    count = 1;
                }
            }
            // Escreve o último count
            if (fwrite(&count, sizeof(int), 1, bin_file) != 1) {
                perror("Erro ao escrever última contagem RLE");
                fclose(bin_file);
                *offset = -1;
                return;
            }
        }
    }
    
    // Garantir que todos os dados foram escritos no disco
    if (fflush(bin_file) != 0) {
        perror("Erro ao fazer flush do arquivo");
        fclose(bin_file);
        *offset = -1;
        return;
    }
    
    fclose(bin_file);
}

void  read_compress_image(const char *bin_file_name, PGM *img, long *offset){
    FILE *bin_file = fopen(bin_file_name, "rb");
    if (!bin_file){
        printf("Erro ao abrir arquivo binário para leitura.\n");
        return;
    }
    
    // Verificar tamanho do arquivo
    fseek(bin_file, 0, SEEK_END);
    long file_size = ftell(bin_file);
    
    if (file_size == 0) {
        printf("Arquivo de dados está vazio!\n");
        fclose(bin_file);
        return;
    }
    
    if (*offset >= file_size) {
        printf("Offset inválido! Offset: %ld, Tamanho do arquivo: %ld\n", *offset, file_size);
        fclose(bin_file);
        return;
    }
    
    fseek(bin_file, *offset, SEEK_SET);
    
    // Ler dimensões e grey_levels (pode ser negativo = RLE, ou positivo = RAW)
    if (fread(&img->width, sizeof(int), 1, bin_file) != 1 ||
        fread(&img->height, sizeof(int), 1, bin_file) != 1) {
        printf("Erro ao ler dimensões da imagem!\n");
        fclose(bin_file);
        return;
    }
    
    int stored_grey;
    if (fread(&stored_grey, sizeof(int), 1, bin_file) != 1) {
        printf("Erro ao ler níveis de cinza!\n");
        fclose(bin_file);
        return;
    }
    
    // Validar dimensões
    if (img->width <= 0 || img->height <= 0 || img->width > 10000 || img->height > 10000) {
        printf("Dimensões inválidas: %dx%d\n", img->width, img->height);
        fclose(bin_file);
        return;
    }
    
    img->pixels = malloc(img->height * sizeof(unsigned char *));
    if (!img->pixels) {
        printf("Erro ao alocar memória para imagem!\n");
        fclose(bin_file);
        return;
    }
    
    for (int i = 0; i < img->height; i++) {
        img->pixels[i] = malloc(img->width * sizeof(unsigned char));
        if (!img->pixels[i]) {
            printf("Erro ao alocar memória para linha %d!\n", i);
            // Liberar memória já alocada
            for (int j = 0; j < i; j++) {
                free(img->pixels[j]);
            }
            free(img->pixels);
            fclose(bin_file);
            return;
        }
    }
    
    // Detectar formato: negativo = RLE, positivo = RAW
    if (stored_grey < 0) {
        // Formato RLE
        img->grey_levels = -stored_grey;
        
        for (int i = 0; i < img->height; i++) {
            if (img->width > 0) {
                int j = 0;
                unsigned char primeiro_pixel;
                fread(&primeiro_pixel, sizeof(unsigned char), 1, bin_file);
                unsigned char pixel_atual = primeiro_pixel;
                
                while (j < img->width) {
                    int count;
                    fread(&count, sizeof(int), 1, bin_file);
                    
                    for (int k = 0; k < count && j < img->width; k++) {
                        img->pixels[i][j++] = pixel_atual;
                    }
                    
                    pixel_atual = (pixel_atual == 0) ? img->grey_levels : 0;
                }
            }
        }
    } else {
        // Formato RAW - ler pixels diretamente
        img->grey_levels = stored_grey;
        
        for (int i = 0; i < img->height; i++) {
            fread(img->pixels[i], sizeof(unsigned char), img->width, bin_file);
        }
    }
    fclose(bin_file);
}

// Verifica se a imagem é binária (apenas 0 e grey_levels)
int is_binary_image(const PGM *img) {
    for (int i = 0; i < img->height; i++) {
        for (int j = 0; j < img->width; j++) {
            unsigned char pixel = img->pixels[i][j];
            if (pixel != 0 && pixel != img->grey_levels) {
                return 0; // Não é binária
            }
        }
    }
    return 1; // É binária
}

// Salva imagem em formato RAW (sem compressão)
void save_raw_v2(PGM *img, const char *bin_file_name, long *offset) {
    FILE *bin_file = fopen(bin_file_name, "ab");
    if (!bin_file) {
        perror("Erro ao abrir o arquivo binário para escrita");
        *offset = -1;
        return;
    }
    
    fseek(bin_file, 0, SEEK_END);
    *offset = ftell(bin_file);
    
    // Salvar com grey_levels POSITIVO (indica RAW)
    if (fwrite(&img->width, sizeof(int), 1, bin_file) != 1 ||
        fwrite(&img->height, sizeof(int), 1, bin_file) != 1 ||
        fwrite(&img->grey_levels, sizeof(int), 1, bin_file) != 1) {
        perror("Erro ao escrever cabeçalho da imagem");
        fclose(bin_file);
        *offset = -1;
        return;
    }
    
    // Salvar pixels diretamente (RAW)
    for (int i = 0; i < img->height; i++) {
        if (fwrite(img->pixels[i], sizeof(unsigned char), img->width, bin_file) != (size_t)img->width) {
            perror("Erro ao escrever pixels da imagem");
            fclose(bin_file);
            *offset = -1;
            return;
        }
    }
    
    // Garantir que todos os dados foram escritos no disco
    if (fflush(bin_file) != 0) {
        perror("Erro ao fazer flush do arquivo");
        fclose(bin_file);
        *offset = -1;
        return;
    }
    
    fclose(bin_file);
    printf("Imagem salva em formato RAW (escala de cinza preservada)\n");
}
             