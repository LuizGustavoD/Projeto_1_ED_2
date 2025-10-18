#include <stdio.h>
#include "bin.h"

int salvar_imagem_binario(FILE *arq, PGM *img) {
    int offset = ftell(arq);
    
    // Salvar apenas dimensões e dados (sem flag is_removed)
    if (fwrite(&img->width, sizeof(int), 1, arq) != 1 ||
        fwrite(&img->height, sizeof(int), 1, arq) != 1 ||
        fwrite(&img->grey_levels, sizeof(int), 1, arq) != 1) {
        perror("Erro ao escrever cabeçalho da imagem");
        return -1;
    }

    for (int i = 0; i < img->height; i++) {
        if (fwrite(img->pixels[i], sizeof(unsigned char), img->width, arq) != (size_t)img->width) {
            perror("Erro ao escrever pixels da imagem");
            return -1;
        }
    }
    
    // Garantir que os dados foram escritos no disco
    if (fflush(arq) != 0) {
        perror("Erro ao fazer flush do arquivo");
        return -1;
    }

    return offset; 
}

PGM *ler_imagem_binario(FILE *arq, long offset) {
    fseek(arq, offset, SEEK_SET);
    PGM *img = malloc(sizeof(PGM));
    // Ler apenas dimensões e dados (sem flag is_removed)
    fread(&img->width, sizeof(int), 1, arq);
    fread(&img->height, sizeof(int), 1, arq);
    fread(&img->grey_levels, sizeof(int), 1, arq);

    img->pixels = malloc(img->height * sizeof(unsigned char *));
    for (int i = 0; i < img->height; i++) {
        img->pixels[i] = malloc(img->width * sizeof(unsigned char));
        fread(img->pixels[i], sizeof(unsigned char), img->width, arq);
    }
    return img;
}
