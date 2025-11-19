#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "pcb.h"

// Maximum number of processes
#define MAX_PROCESSES 16

// Time quantum for round-robin scheduling (in timer ticks)
#define TIME_QUANTUM 5

// Scheduler structure
typedef struct {
    pcb_t* ready_queue;      // Queue of ready processes
    pcb_t* current_process;  // Currently running process
    pcb_t processes[MAX_PROCESSES]; // Array of all PCBs
    uint32_t next_pid;       // Next available process ID
    uint32_t process_count;  // Number of active processes
    uint32_t tick_count;     // Global tick counter
} scheduler_t;

// Global scheduler instance
extern scheduler_t g_scheduler;

// Function declarations
void scheduler_init(void);
void scheduler_add_process(const char* name, uint32_t priority);
void scheduler_remove_process(uint32_t pid);
void scheduler_tick(void);
void scheduler_yield(void);
pcb_t* scheduler_get_current_process(void);
void scheduler_print_status(void);

// Process entry points (OS-style processes)
void task_kernel_init(void);
void task_vfs(void);
void task_shell(void);
void task_syslog(void);

#endif // SCHEDULER_H
