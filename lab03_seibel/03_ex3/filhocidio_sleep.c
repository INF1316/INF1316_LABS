#include <stdio.h>
#include <stdlib.h>
#include <signal.h>     // Flags SIG e funcao signal
#include <sys/wait.h>
#include <unistd.h>

#define TEMPO 2         // Em segundos
#define RIP_FILHO -2    // Indica que filho foi morto

void childHandler(int sinal);

int delay; // Tempo de tolerancia antes do filhocidio

// Recebe como argumentos o nome do executavel e o tempo de espera ate a morte (em segundos)
int main(int arg, char** argv)
{
    pid_t pid;

    signal(SIGCHLD, childHandler); // Handler que detecta termino do filho

    if ((pid = fork()) < 0) // Erro
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0) // Filho
    {
        sleep(TEMPO);
        exit(EXIT_SUCCESS); // Filho avisa ao pai que terminou
    }

    // Pai
    delay = atoi(argv[1]);
    sleep(delay);
    kill(pid, SIGKILL);
    puts("Denscansa em paz, filho");

    return RIP_FILHO;
}

// Trata SIGCHLD (17), que indica que um processo filho terminou
void childHandler(int sinal)
{
    int status;
    pid_t pid = wait(&status);

    printf("Processo filho %d sobreviveu, porque terminou antes de %d segundo(s)\n", pid, delay);
    exit(EXIT_SUCCESS); // Encerra antes que o pai mate o filho
}
