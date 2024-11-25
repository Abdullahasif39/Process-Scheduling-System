#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

struct PrioritizedPCB {
    int pid;
    int burst_time;
    int arrival_time;
    int priority;
    int remaining_cpu_time;
    char state[20];
    struct PrioritizedPCB *next;
};

struct PCB {
    int pid;
    int burst_time;
    int arrival_time;
    int remaining_cpu_time;
    char state[20];
    struct PCB *next;
};

struct PCB *ready_queue = NULL;
pthread_mutex_t ready_queue_mutex = PTHREAD_MUTEX_INITIALIZER;

void add_process(struct PCB *new_process) {
    pthread_mutex_lock(&ready_queue_mutex);
    if (ready_queue == NULL) {
        ready_queue = new_process;
    } else {
        struct PCB *current = ready_queue;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_process;
    }
    pthread_mutex_unlock(&ready_queue_mutex);
}

struct PCB *select_next_process() {
    pthread_mutex_lock(&ready_queue_mutex);
    if (ready_queue == NULL) {
        pthread_mutex_unlock(&ready_queue_mutex);
        return NULL;
    }
    struct PCB *next_process = ready_queue;
    ready_queue = ready_queue->next;
    next_process->next = NULL;
    pthread_mutex_unlock(&ready_queue_mutex);
    return next_process;
}

void execute_process(struct PCB *process) {
    printf("Executing process with PID %d, Burst Time: %d\n", process->pid, process->burst_time);
    sleep(process->burst_time);
    printf("Process with PID %d finished execution.\n", process->pid);

    strcpy(process->state, "Terminated");
    free(process); // Free memory allocated to the PCB
}


void* scheduler(void *args) {
    while (1) {
        if (!ready_queue) {
            printf("No processes in the ready queue.\n");
            sleep(1);
            continue;
        }
        struct PCB *next_process = select_next_process();
        if (next_process != NULL) {
            execute_process(next_process);
        }
    }
    return NULL;
}

void FCFS() {
    pthread_t scheduler_thread;
    pthread_create(&scheduler_thread, NULL, scheduler, NULL);

    // Simulate processes (creating PCBs and adding them to the ready queue)
    for (int i = 1; i <= 5; ++i) {
        struct PCB *new_process = (struct PCB*)malloc(sizeof(struct PCB));
        new_process->pid = i;
        new_process->burst_time = 1 + rand() % 5; // Random burst time between 1 and 5
        new_process->arrival_time = i; // Random arrival time between 0 and 9
        new_process->remaining_cpu_time = new_process->burst_time;
        strcpy(new_process->state, "Ready");
        new_process->next = NULL;
        add_process(new_process);
    }

    pthread_join(scheduler_thread, NULL);
}

void SJF() {
    pthread_t scheduler_thread;
    pthread_create(&scheduler_thread, NULL, scheduler, NULL);

    // Simulate processes (creating PCBs and adding them to the ready queue)
    for (int i = 1; i <= 5; ++i) {
        struct PCB *new_process = (struct PCB*)malloc(sizeof(struct PCB));
        new_process->pid = i;
        new_process->burst_time = 1 + rand() % 5; // Random burst time between 1 and 5
        new_process->arrival_time = i; // Random arrival time between 0 and 9
        new_process->remaining_cpu_time = new_process->burst_time;
        strcpy(new_process->state, "Ready");
        new_process->next = NULL;
        
        // Insert process in the ready queue based on burst time (SJF)
        pthread_mutex_lock(&ready_queue_mutex);
        if (ready_queue == NULL || new_process->burst_time < ready_queue->burst_time) {
            new_process->next = ready_queue;
            ready_queue = new_process;
        } else {
            struct PCB *current = ready_queue;
            while (current->next != NULL && current->next->burst_time < new_process->burst_time) {
                current = current->next;
            }
            new_process->next = current->next;
            current->next = new_process;
        }
        pthread_mutex_unlock(&ready_queue_mutex);
    }

    pthread_join(scheduler_thread, NULL);
}

void priorityPreemptive() {
    pthread_t scheduler_thread;
    pthread_create(&scheduler_thread, NULL, scheduler, NULL);

    // Simulate processes (creating PCBs and adding them to the ready queue)
    for (int i = 1; i <= 5; ++i) {
        struct PrioritizedPCB *new_process = (struct PrioritizedPCB*)malloc(sizeof(struct PrioritizedPCB));
        new_process->pid = i;
        new_process->burst_time = 1 + rand() % 5; // Random burst time between 1 and 5
        new_process->arrival_time = i; // Random arrival time between 0 and 9
        new_process->priority = rand() % 10; // Random priority between 0 and 9
        new_process->remaining_cpu_time = new_process->burst_time;
        strcpy(new_process->state, "Ready");
        new_process->next = NULL;
        
        // Insert process in the ready queue based on priority (preemptive)
        pthread_mutex_lock(&ready_queue_mutex);
        if (ready_queue == NULL || new_process->priority < ((struct PrioritizedPCB *)ready_queue)->priority) {
            new_process->next = (struct PrioritizedPCB *)ready_queue;
            ready_queue = (struct PCB *)new_process;
        } else {
            struct PCB *current = (struct PCB *)ready_queue;
            while (current->next != NULL && ((struct PrioritizedPCB *)current->next)->priority <= new_process->priority) {
                current = current->next;
            }
            new_process->next = ((struct PrioritizedPCB *)current->next);
            current->next = (struct PCB *)new_process;
        }
        pthread_mutex_unlock(&ready_queue_mutex);
    }

    pthread_join(scheduler_thread, NULL);
}


int main() {
    // FCFS();
    // SJF();
    priorityPreemptive();
    return 0;
}