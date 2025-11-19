#ifndef PCB_H
#define PCB_H

#include <stdint.h>

// Process states
typedef enum {
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_BLOCKED,
    PROCESS_TERMINATED
} process_state_t;

// Process Control Block structure
typedef struct pcb {
    uint32_t pid;                    // Process ID
    process_state_t state;           // Current process state
    uint32_t priority;               // Process priority (0 = highest)
    uint32_t quantum_remaining;      // Time quantum remaining
    uint32_t total_runtime;          // Total CPU time used
    
    // CPU context (registers)
    uint32_t eax, ebx, ecx, edx;     // General purpose registers
    uint32_t esi, edi, esp, ebp;     // Stack and index registers
    uint32_t eip;                    // Instruction pointer
    uint32_t eflags;                 // CPU flags
    
    // Stack information
    uint32_t stack_base;             // Base address of stack
    uint32_t stack_size;             // Size of stack
    uint32_t stack_pointer;          // Current stack pointer
    
    // Process information
    char name[32];                   // Process name
    uint32_t creation_time;          // Time when process was created
    
    // Linked list pointers for scheduling queues
    struct pcb* next;                // Next PCB in queue
    struct pcb* prev;                // Previous PCB in queue
} pcb_t;

// Function declarations
void pcb_init(pcb_t* pcb, uint32_t pid, const char* name, uint32_t priority);
void pcb_save_context(pcb_t* pcb);
void pcb_restore_context(pcb_t* pcb);

#endif // PCB_H
