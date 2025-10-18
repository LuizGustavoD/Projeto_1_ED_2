#ifndef BIN_H
#define BIN_H

#include <stdio.h>
#include <stdlib.h>
#include "../models/structure.h"

int salvar_imagem_binario(FILE *arq, PGM *img);
PGM *ler_imagem_binario(FILE *arq, long offset);

#endif // BIN_H
