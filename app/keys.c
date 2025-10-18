#include <stdio.h>
#include <string.h>
#include "keys.h"

void adicionar_indice(FILE *indice, IndiceRecord reg) {
    // Garantir que a flag is_removed seja inicializada como 0 (não removida)
    reg.is_removed = 0;
    fwrite(&reg, sizeof(IndiceRecord), 1, indice);
}

// Busca sequencial O(N)
long buscar_offset_por_nome(FILE *indice, const char *nome) {
    IndiceRecord reg;
    rewind(indice);
    while (fread(&reg, sizeof(IndiceRecord), 1, indice)) {
        if (strcmp(reg.name, nome) == 0)
            //Retorna o offset do arquivo
            return reg.offset;
    }
    //Não encontrado
    return -1;
}
