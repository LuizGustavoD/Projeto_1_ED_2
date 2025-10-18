#ifndef PGM_H
#define PGM_H

#include <stdio.h>
#include "../models/structure.h"

PGM *processar_entrada(FILE *arq, char *nome_arquivo);
void free_image(PGM *img);
void save_pgm(const char *caminho, PGM *img);
void remove_flag_control(FILE *data_file, long offset);
void restore_flag_control(FILE *data_file, long offset);
void restore_image_from_bin(const char *bin_file_name, const char *output_pgm_name);
PGM *calcular_imagem_media(const char *nome_base, const char *data_file, const char *index_file);

#endif // PGM_H
