#ifndef MANAGE_DATA_H
#define MANAGE_DATA_H

#include <stdio.h>
#include "../models/structure.h"

void compress_data_file(const char *data_file_name, const char *compress_file_name);
void append_image_to_data_file(const char *data_base_file_name, PGM *img, int top, const char *nome_imagem);
void insert_image_at_top(const char *data_base_file_name, PGM *img, long *new_offset);
void recreate_index_file(const char *data_file_name, const char *index_file_name);
void recreate_index_with_names(const char *data_file_name, const char *index_file_name, 
                               const char *new_image_name, char existing_names[][max_name], 
                               int existing_count);
void recreate_index_after_compression(const char *data_file_name, const char *index_file_name);

#endif // MANAGE_DATA_H
