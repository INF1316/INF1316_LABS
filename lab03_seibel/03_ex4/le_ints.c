#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

void fpeHandler(int sinal);

/* OBSERVACOES*/
/*
    Apesar de inteira, a divisao por zero emitiu um
    sinal de Floating Point Error.
*/

int main(void)
{
    int i1, i2;

    signal(SIGFPE, fpeHandler); // Floating Point Exception

    printf("\nInforme os dois numeros inteiros: ");
    scanf("%d %d", &i1, &i2);

    printf("\n");
    printf("Soma:    %-3d\n", i1 + i2);
    printf("Subtr.:  %-3d\n", i1 - i2);
    printf("Prod.:   %-3d\n", i1 * i2);
    printf("Div.:    %-3d\n", i1 / i2);

    return 0;
}

// Trata Floating Point Exception (FPE = 8)
void fpeHandler(int sinal)
{
    puts("\nExcecao de ponto flutuante detectada!");
    exit(EXIT_FAILURE);
}
