#ifndef STRUCTURE_H
#define STRUCTURE_H

//Declarações de estruturas utilizadas no projeto
#define max 256
#define max_name 100

typedef struct {  
    // Removendo is_removed da estrutura PGM
    int width;
    int height;
    int grey_levels;
    unsigned char **pixels;  
} PGM;

typedef struct {
    long offset;                   
    char name[max_name];
    int is_removed;  // ← FLAG DE DELEÇÃO MOVIDA PARA O ÍNDICE
} IndiceRecord;

#endif // STRUCTURE_H