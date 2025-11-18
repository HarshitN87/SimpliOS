#include "scheduler.h"
#include "pcb.h"
#include <stddef.h>

// External functions from kernel.c
extern void term_print(const char* str);
extern void term_putc(char c);

// Global scheduler instance
scheduler_t g_scheduler;

// Simple helper function to print numbers
static void print_number(uint32_t num) {
    char buffer[12];
    int i = 0;
    
    if (num == 0) {
        term_putc('0');
        return;
    }
    
    // Convert number to string
    while (num > 0) {
        buffer[i++] = '0' + (num % 10);
        num /= 10;
    }
    
    // Print in reverse order
    while (i > 0) {
        term_putc(buffer[--i]);
    }
}

// Initialize the scheduler
void scheduler_init(void) {
    g_scheduler.ready_queue = NULL;
    g_scheduler.current_process = NULL;
    g_scheduler.next_pid = 1;
    g_scheduler.process_count = 0;
    g_scheduler.tick_count = 0;
    
    term_print("Scheduler initialized.\n");
}

// Add a new process to the ready queue
void scheduler_add_process(const char* name, uint32_t priority) {
    if (g_scheduler.process_count >= MAX_PROCESSES) {
        term_print("Error: Maximum processes reached.\n");
        return;
    }
    
    // Find an empty PCB slot
    pcb_t* pcb = NULL;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (g_scheduler.processes[i].pid == 0) {
            pcb = &g_scheduler.processes[i];
            break;
        }
    }
    
    if (!pcb) {
        term_print("Error: No available PCB slots.\n");
        return;
    }
    
    // Initialize the PCB
    pcb_init(pcb, g_scheduler.next_pid++, name, priority);
    pcb->creation_time = g_scheduler.tick_count;
    pcb->state = PROCESS_READY;
    
    // Add to ready queue (simple FIFO insertion)
    if (g_scheduler.ready_queue == NULL) {
        g_scheduler.ready_queue = pcb;
        pcb->next = pcb;
        pcb->prev = pcb;
    } else {
        pcb->next = g_scheduler.ready_queue;
        pcb->prev = g_scheduler.ready_queue->prev;
        g_scheduler.ready_queue->prev->next = pcb;
        g_scheduler.ready_queue->prev = pcb;
    }
    
    g_scheduler.process_count++;
    
    term_print("Added process: ");
    term_print(name);
    term_print(" (PID: ");
    print_number(pcb->pid);
    term_print(")\n");
}

// Remove a process from the system
void scheduler_remove_process(uint32_t pid) {
    // Find the PCB
    pcb_t* pcb = NULL;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (g_scheduler.processes[i].pid == pid) {
            pcb = &g_scheduler.processes[i];
            break;
        }
    }
    
    if (!pcb || pcb->state == PROCESS_TERMINATED) {
        return;
    }
    
    // Remove from ready queue if present
    if (pcb->state == PROCESS_READY) {
        if (pcb->next == pcb) {
            // Only process in queue
            g_scheduler.ready_queue = NULL;
        } else {
            if (g_scheduler.ready_queue == pcb) {
                g_scheduler.ready_queue = pcb->next;
            }
            pcb->prev->next = pcb->next;
            pcb->next->prev = pcb->prev;
        }
    }
    
    // Mark as terminated
    pcb->state = PROCESS_TERMINATED;
    pcb->pid = 0; // Free the PID
    g_scheduler.process_count--;
    
    // If this was the current process, schedule next
    if (g_scheduler.current_process == pcb) {
        g_scheduler.current_process = NULL;
        scheduler_yield();
    }
}

// Handle a timer tick
void scheduler_tick(void) {
    g_scheduler.tick_count++;
    
    if (g_scheduler.current_process != NULL) {
        g_scheduler.current_process->quantum_remaining--;
        g_scheduler.current_process->total_runtime++;
        
        // Check if quantum expired
        if (g_scheduler.current_process->quantum_remaining <= 0) {
            scheduler_yield();
        }
    }
}

// Yield the CPU to the next process
void scheduler_yield(void) {
    // Save context of current process
    if (g_scheduler.current_process != NULL) {
        pcb_save_context(g_scheduler.current_process);
        g_scheduler.current_process->state = PROCESS_READY;
        
        // Add back to ready queue
        if (g_scheduler.ready_queue == NULL) {
            g_scheduler.ready_queue = g_scheduler.current_process;
            g_scheduler.current_process->next = g_scheduler.current_process;
            g_scheduler.current_process->prev = g_scheduler.current_process;
        } else {
            g_scheduler.current_process->next = g_scheduler.ready_queue;
            g_scheduler.current_process->prev = g_scheduler.ready_queue->prev;
            g_scheduler.ready_queue->prev->next = g_scheduler.current_process;
            g_scheduler.ready_queue->prev = g_scheduler.current_process;
        }
    }
    
    // Select next process from ready queue
    if (g_scheduler.ready_queue != NULL) {
        g_scheduler.current_process = g_scheduler.ready_queue;
        g_scheduler.current_process->state = PROCESS_RUNNING;
        g_scheduler.current_process->quantum_remaining = TIME_QUANTUM;
        
        // Remove from ready queue
        if (g_scheduler.current_process->next == g_scheduler.current_process) {
            // Only process in queue
            g_scheduler.ready_queue = NULL;
        } else {
            g_scheduler.ready_queue = g_scheduler.current_process->next;
            g_scheduler.current_process->prev->next = g_scheduler.current_process->next;
            g_scheduler.current_process->next->prev = g_scheduler.current_process->prev;
        }
        
        // Restore context
        pcb_restore_context(g_scheduler.current_process);
    } else {
        g_scheduler.current_process = NULL;
    }
}

// Get the currently running process
pcb_t* scheduler_get_current_process(void) {
    return g_scheduler.current_process;
}

// Print scheduler status
void scheduler_print_status(void) {
    term_print("\n=== Scheduler Status ===\n");
    term_print("Tick: ");
    print_number(g_scheduler.tick_count);
    term_print("\nActive processes: ");
    print_number(g_scheduler.process_count);
    term_print("\n");
    
    if (g_scheduler.current_process != NULL) {
        term_print("Current: ");
        term_print(g_scheduler.current_process->name);
        term_print(" (PID: ");
        print_number(g_scheduler.current_process->pid);
        term_print(", Quantum: ");
        print_number(g_scheduler.current_process->quantum_remaining);
        term_print(")\n");
    } else {
        term_print("Current: None\n");
    }
    
    // Print ready queue
    if (g_scheduler.ready_queue != NULL) {
        term_print("Ready queue: ");
        pcb_t* pcb = g_scheduler.ready_queue;
        do {
            term_print(pcb->name);
            term_print(" (");
            print_number(pcb->pid);
            term_print(")");
            if (pcb->next != g_scheduler.ready_queue) {
                term_print(", ");
            }
            pcb = pcb->next;
        } while (pcb != g_scheduler.ready_queue);
        term_print("\n");
    } else {
        term_print("Ready queue: Empty\n");
    }
    term_print("========================\n");
}

// Simple task implementations - OS-style processes
void task_kernel_init(void) {
    static uint32_t init_cycle = 0;
    init_cycle++;
    
    // Simulate kernel initialization/maintenance tasks
    if (init_cycle % 200 == 0) {
        // Periodic kernel maintenance
        // In a real OS, this would handle cleanup, memory defrag, etc.
    }
}

void task_vfs(void) {
    static uint32_t vfs_ops = 0;
    vfs_ops++;
    
    // Simulate virtual filesystem daemon
    // In a real OS, this would handle file system operations, caching, etc.
    if (vfs_ops % 150 == 0) {
        // Periodic filesystem maintenance
        // Could flush buffers, check disk health, etc.
    }
}

void task_shell(void) {
    static uint32_t shell_ticks = 0;
    shell_ticks++;
    
    // Simulate shell process
    // In a real OS, the shell would be a separate process
    // This represents the shell's background maintenance
    if (shell_ticks % 300 == 0) {
        // Periodic shell maintenance (history cleanup, etc.)
    }
}

void task_syslog(void) {
    static uint32_t log_entries = 0;
    log_entries++;
    
    // Simulate system logging daemon
    // In a real OS, this would handle log rotation, buffering, etc.
    if (log_entries % 250 == 0) {
        // Periodic log maintenance
    }
}
