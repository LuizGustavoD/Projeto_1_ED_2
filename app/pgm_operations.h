#ifndef PGM_OPERATIONS_H
#define PGM_OPERATIONS_H

#include <stdio.h>
#include "../models/structure.h"

void limiarizar(PGM *img, int L);
void negativar(PGM *img);
void RLE_compress(PGM *img, const char *bin_file_name);
void RLE_compress_v2(PGM *img, const char *bin_file_name, long *offset);
void read_compress_image(const char *bin_file_name, PGM *img, long *offset);

// Funções auxiliares para RAW e detecção de formato
int is_binary_image(const PGM *img);
void save_raw_v2(PGM *img, const char *bin_file_name, long *offset);

#endif // PGM_OPERATIONS_H
