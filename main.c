#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

time_t start_time;

struct PrioritizedPCB {
    int pid;
    int burst_time;
    int arrival_time;
    int priority;
    int completion_time;
    int turnaround_time;
    int waiting_time;
    char state[20];
    struct PrioritizedPCB *next;
};

struct PCB {
    int pid;
    int burst_time;
    int arrival_time;
    int completion_time;
    int turnaround_time;
    int waiting_time;
    char state[20];
    struct PCB *next;
};

struct PCB *ready_queue = NULL;
struct PCB *completed_processes = NULL;
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
    time_t current_time = time(NULL);
    double elapsed_time = difftime(current_time, start_time);

    printf("\n - Current Time: %.0f seconds since start\n\n", elapsed_time);
    printf("Executing process with PID %d, Burst Time: %d\n", process->pid, process->burst_time);

    sleep(process->burst_time);
    printf("Process with PID %d finished execution.\n", process->pid);

    strcpy(process->state, "Terminated");
    time_t end_time = time(NULL);

    process->completion_time = difftime(end_time, start_time);
    process->turnaround_time = process->completion_time - process->arrival_time;
    process->waiting_time = process->turnaround_time - process->burst_time;

    pthread_mutex_lock(&ready_queue_mutex);
    process->next = completed_processes;
    completed_processes = process;
    pthread_mutex_unlock(&ready_queue_mutex);
}

void* scheduler(void *args) {
    struct PCB *currently_running_process = NULL;
    while (1) {
        pthread_mutex_lock(&ready_queue_mutex);
        if (!ready_queue) {
            pthread_mutex_unlock(&ready_queue_mutex);
            if (currently_running_process == NULL) {
                printf("No processes in the queue and no process is running.\n");
                break;
            }
            continue;
        }
        
        struct PCB *highest_priority_process = (struct PCB *)ready_queue;
        if (currently_running_process == NULL || ((struct PrioritizedPCB *)highest_priority_process)->priority < ((struct PrioritizedPCB *)currently_running_process)->priority) {
            if (currently_running_process != NULL) {
                printf("Preempting Process %d with Process %d\n", currently_running_process->pid, highest_priority_process->pid);
                currently_running_process->next = ready_queue;
                ready_queue = currently_running_process;
            }
            ready_queue = ready_queue->next;
            highest_priority_process->next = NULL;
            currently_running_process = highest_priority_process;
        }
        pthread_mutex_unlock(&ready_queue_mutex);

        if (currently_running_process != NULL) {
            execute_process(currently_running_process);
            currently_running_process = NULL;
        }
    }
    return NULL;
}

void display_statistics() {
    struct PCB *current = completed_processes;
    double total_tat = 0, total_wt = 0;
    int count = 0;

    printf("\nProcess Execution Statistics:\n");
    printf("PID\tCompletion Time\tTurnaround Time\tWaiting Time\n");

    while (current != NULL) {
        printf("%d\t%d\t\t%d\t\t%d\n", current->pid, current->completion_time, current->turnaround_time, current->waiting_time);
        total_tat += current->turnaround_time;
        total_wt += current->waiting_time;
        count++;
        current = current->next;
    }

    if (count > 0) {
        printf("\nAverage Turnaround Time: %.2f\n", total_tat / count);
        printf("Average Waiting Time: %.2f\n", total_wt / count);
    }
}

void FCFS(int numberOfProcesses, int maxBurstTime) {
    pthread_t scheduler_thread;
    pthread_create(&scheduler_thread, NULL, scheduler, NULL);

    for (int i = 1; i <= numberOfProcesses; ++i) {
        struct PCB *new_process = (struct PCB*)malloc(sizeof(struct PCB));
        new_process->pid = i;
        new_process->burst_time = 1 + rand() % maxBurstTime;
        time_t current_time = time(NULL);
        double elapsed_time = difftime(current_time, start_time);
        new_process->arrival_time = elapsed_time;
        strcpy(new_process->state, "Ready");
        new_process->next = NULL;
        add_process(new_process);
    }

    pthread_join(scheduler_thread, NULL);
}

void SJF(int numberOfProcesses, int maxBurstTime) {
    pthread_t scheduler_thread;
    pthread_create(&scheduler_thread, NULL, scheduler, NULL);

    for (int i = 1; i <= numberOfProcesses; ++i) {
        struct PCB *new_process = (struct PCB*)malloc(sizeof(struct PCB));
        new_process->pid = i;
        new_process->burst_time = 1 + rand() % maxBurstTime;
        time_t current_time = time(NULL);
        double elapsed_time = difftime(current_time, start_time);
        new_process->arrival_time = elapsed_time;
        strcpy(new_process->state, "Ready");
        new_process->next = NULL;
        
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

void priorityPreemptive(int numberOfProcesses, int maxBurstTime) {
    pthread_t scheduler_thread;
    pthread_create(&scheduler_thread, NULL, scheduler, NULL);

    for (int i = 1; i <= numberOfProcesses; ++i) {
        struct PrioritizedPCB *new_process = (struct PrioritizedPCB*)malloc(sizeof(struct PrioritizedPCB));
        new_process->pid = i;
        new_process->burst_time = 1 + rand() % maxBurstTime;
        time_t current_time = time(NULL);
        double elapsed_time = difftime(current_time, start_time);
        new_process->arrival_time = elapsed_time;
        new_process->priority = rand() % 10;
        strcpy(new_process->state, "Ready");
        new_process->next = NULL;
        
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

int main(int argc, char *argv[]) {
    if (argc < 4) {
        printf("Usage: %s <algorithm> <number_of_processes> <max_burst_time>\n", argv[0]);
        return 1;
    }

    char *algorithm = argv[1];
    int numberOfProcesses = atoi(argv[2]);
    int maxBurstTime = atoi(argv[3]);

    start_time = time(NULL);

    if (strcmp(algorithm, "fcfs") == 0) {
        printf("Running FCFS algorithm with %d processes, max burst time: %d...\n", numberOfProcesses, maxBurstTime);
        FCFS(numberOfProcesses, maxBurstTime);
    } else if (strcmp(algorithm, "sjf") == 0) {
        printf("Running SJF algorithm with %d processes, max burst time: %d...\n", numberOfProcesses, maxBurstTime);
        SJF(numberOfProcesses, maxBurstTime);
    } else if (strcmp(algorithm, "pp") == 0) {
        printf("Running Priority Preemptive algorithm with %d processes, max burst time: %d...\n", numberOfProcesses, maxBurstTime);
        priorityPreemptive(numberOfProcesses, maxBurstTime);
    } else {
        printf("Invalid algorithm specified.\n");
        return 1;
    }

    display_statistics();
    return 0;
}
