#include "mysignal.h"
#include "pcbqueue.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <unistd.h>

#define TRUE 1
#define INT_CTL_PATH    "./inter_control"
#define PATH            "./a"
#define NUM_PRCS        6       // Number of processes
#define ARGC            5       // Number of elements in argv
#define SYSC_ARGC       2       // Number of system call arguments
#define OFFSET_C        4       // Number of elements in offset vector   
#define PAGE            4096    // byte(s)
#define BUF_SIZE        64      // byte(s)

/* PROTOTYPES */
static void error(const char* msg);
void        timeslice_handler(int signal);
void        iointerrupt_handler(int signal);
void        syscall_handler(int signal);
PCB*        new_process(const char* path, char** argv);
PCB*        init(int p_count, const char* path, char** argv);

/* GLOBAL VARIABLES */
int process_count;
int shm_offset_v[ARGC] = { 0, 100, 200 }; // Shared memory offsets in bytes with respect to shmptrbase (0 -> pc, 1 -> syscall_arg0, 2 -> syscall_arg1)
int* pc; // Program Counter

char shmidbuf[BUF_SIZE]; // Buffer that contains shmid in string form.
char* argv[ARGC] = { shmidbuf, "0", "100", "200", NULL }; // Vector of arguments sent to child processes.
char* syscallarg[SYSC_ARGC]; // System call arguments.

void *shmptrbase; // Base of shared memory

PCB* current_pcb;       // Process Control Board (PCB) of the current process.
PCB* inter_control_pcb; // PCB of Interrupt Controller.

Queue* ready_q; // Queue of processes in ready state.
Queue* wait_q;  // Queue of processes in wait state.

/* FUNCTIONS */
int main(void)
{
    int shmid;

    signal(SIGIRQ1, timeslice_handler);
    signal(SIGIRQ2, iointerrupt_handler);
    signal(SIGSYS, syscall_handler);

    if ((shmid = shmget(IPC_PRIVATE, PAGE, IPC_CREAT | S_IRWXU)) == -1)
        error("shmid");
    
    if ((shmptrbase = shmat(shmid, NULL, 0)) == (void*)-1)
        error("shmat");

    sprintf(shmidbuf, "%d", shmid); // Saves shmid in string form to send it to child processes via argv

    // Mapping shared memory for Inter-process communication (IPC)
    pc = (int*)(shmptrbase + shm_offset_v[0]);
    
    for (int i = 0; i < SYSC_ARGC; i++)
        syscallarg[i] = (char*)(shmptrbase + shm_offset_v[i + 1]);

    // Allocate queues
    ready_q = new_queue();
    wait_q = new_queue();

    // Interrupt Controller
    inter_control_pcb = new_process(INT_CTL_PATH, NULL);

    // Processes to be scheduled
    current_pcb = init(NUM_PRCS, PATH, argv);

    puts("\nBEGINNING OF SCHEDULING");

    kill(current_pcb->pid, SIGCONT); // Continues first process

    // Scheduling
    while (process_count > 0);

    puts("END OF SCHEDULING");

    shmctl(shmid, IPC_RMID, NULL);
    shmdt(shmptrbase);

    return 0;
}

// Prints error message and exits with EXIT_FAILURE.
static void error(const char* msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

// Creates new process that will execute the program in path with arguments argv.
PCB* new_process(const char* path, char** argv)
{
    pid_t pid;

    if ((pid = fork()) < 0) // Error
        error("fork");

    else if (pid == 0) // Child
        execv(path, argv);

    return new_pcb(pid, 0, NULL, NULL);
}

// Creates child processes to be scheduled and enqueues them
// in ready_q except for the first one.
// Pauses all created processes.
PCB* init(int p_count, const char* path, char** argv)
{
    PCB* first, *current;

    first = new_process(path, argv);
    kill(first->pid, SIGSTOP);
    process_count = 1;
    
    // Comment this line, if you will
    printf("PID %d created\n", first->pid);

    for (int i = 1; i < p_count; i++)
    {
        enqueue((current = new_process(path, argv)), ready_q); // Creates and enqueues new process
        kill(current->pid, SIGSTOP); // Stops process
        process_count++;

        // Comment this line, if you will
        printf("PID %d created\n", current->pid);
    }

    return first;
}

// Saves context of current process and enqueues it in enq.
void context_save(Queue* enq)
{
    int status;

    kill(inter_control_pcb->pid, SIGSTOP); // Deactivate interruptions momentarilly

    if (!current_pcb)
        return;
    
    kill(current_pcb->pid, SIGSTOP);

    // Checks whether process has finished
    if (current_pcb->pid == waitpid(current_pcb->pid, &status, WNOHANG))
    {
        // Comment this line, if you will
        printf("PID %d finished\n", current_pcb->pid);

        free_pcb(current_pcb); // Frees memory used by PCB
        current_pcb = NULL;
        process_count--;
        return;
    }
    // Process has not finished

    // Comment this line, if you will
    printf("%-2d PID %d stopped\n", *pc, current_pcb->pid);
    
    current_pcb->pc = *pc; // Saves Program Counter
    enqueue(current_pcb, enq); // Enqueues current PCB, but does not update it

    kill(inter_control_pcb->pid, SIGCONT); // Reactivate interruptions
}

// Changes process context.
// Current PCB becomes dequeued PCB from deq.
void context_swap(Queue* deq)
{
    kill(inter_control_pcb->pid, SIGSTOP); // Deactivate interruptions momentarilly

    current_pcb = dequeue(deq);

    if (current_pcb)
    {
        *pc = current_pcb->pc;
        kill(current_pcb->pid, SIGCONT);

        // Comment this line, if you will
        printf("%-2d PID %d continued\n", *pc, current_pcb->pid);
    }
    else
        puts("Idle"); // Comment this line, if you will

    kill(inter_control_pcb->pid, SIGCONT); // Reactivate interruptions
}

// Handles end of time_slice.
// Swaps the context to next ready process.
void timeslice_handler(int signal)
{
    kill(inter_control_pcb->pid, SIGSTOP);

    context_save(ready_q);
    context_swap(ready_q);

    kill(inter_control_pcb->pid, SIGCONT);
}

// Handles a syscall.
// Swaps the context to next ready process.
void syscall_handler(int signal)
{
    kill(inter_control_pcb->pid, SIGSTOP); // Deactivate interruptions momentarilly

    // Saves syscallarguments in PCB
    strcpy(current_pcb->syscallarg[0], syscallarg[0]);
    strcpy(current_pcb->syscallarg[1], syscallarg[1]);
    
    context_save(wait_q);

    // Comment this line, if you will
    printf("PID %d requests systemcall(%s, %s)\n", current_pcb->pid, current_pcb->syscallarg[0], current_pcb->syscallarg[1]);
    
    context_swap(ready_q);

    kill(inter_control_pcb->pid, SIGCONT); // Reactivate interruptions
    kill(inter_control_pcb->pid, SIGIRQ2); // Signals Interrupt Controller of IO operation
}

// Dequeues from wait and enqueues in ready.
// Handles finishing of an IO operation.
void iointerrupt_handler(int signal)
{
    PCB* pcb;

    kill(inter_control_pcb->pid, SIGSTOP); // Deactivate interruptions momentarilly

    pcb = dequeue(wait_q);

    // Comment this line, if you will
    printf("PID %d's systemcall(%s, %s) finished\n", pcb->pid, pcb->syscallarg[0], pcb->syscallarg[1]);
    
    enqueue(pcb, ready_q);

    kill(inter_control_pcb->pid, SIGCONT); // Reactivate interruptions
}
