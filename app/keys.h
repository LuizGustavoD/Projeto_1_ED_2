#ifndef KEYS_H
#define KEYS_H

#include <stdio.h>
#include "../models/structure.h"

void adicionar_indice(FILE *indice, IndiceRecord reg);
long buscar_offset_por_nome(FILE *indice, const char *nome);

#endif // KEYS_H
