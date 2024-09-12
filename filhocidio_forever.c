#include <stdio.h>
#include <stdlib.h>
#include <signal.h>     // Inclui macros de sinais (SIG)
#include <sys/wait.h>
#include <unistd.h>

#define EVER ;;
#define RIP_FILHO -2

void childHandler(int sinal);

int delay; // Tempo de tolerancia antes do filhocidio

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
        for(EVER);

    // Pai
    delay = atoi(argv[1]);
    sleep(delay);
    kill(pid, SIGKILL);
    puts("Denscansa em paz, filho");

    return RIP_FILHO; // Indica que filho foi morto
}

void childHandler(int sinal)
{
    int status;
    pid_t pid = wait(&status);

    printf("Processo filho %d sobreviveu, porque terminou antes de %d segundo(s)\n", pid, delay);
    exit(EXIT_SUCCESS);
}