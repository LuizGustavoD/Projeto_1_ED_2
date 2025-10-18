//Main, centraliza o programa em um unico centro de execução
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "app/app.h"

int main() {
    app_call();
    return 0;
}
//Para compilar: gcc main.c app/*.c -o main