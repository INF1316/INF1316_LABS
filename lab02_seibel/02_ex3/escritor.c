#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>

#define KEY_ID 7000     // Identificador da memoria a ser criada
#define SHM_SIZE 4096   // Tamanho minimo da memoria a ser criada

int main(void)
{
    // Cria memoria compartilhada
    int shmid = shmget(KEY_ID, SHM_SIZE, IPC_CREAT | S_IRWXU);
    char* shmptr;

    if (shmid == -1)
    {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    shmptr = (char*)shmat(shmid, NULL, 0); // Anexa-se a memoria compartilhada

    if (shmptr == -1)
    {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    printf("Escreva a mensagem do dia: ");
    fgets(shmptr, SHM_SIZE, stdin); // Obtem mensagem vinda do teclado
    puts("\nMensagem armazenada com sucesso!");

    shmdt(shmptr); // Desanexa memoria, mas nao a remove

    return 0;
}
