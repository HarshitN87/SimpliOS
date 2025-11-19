#include "pcb.h"

// Initialize a PCB with default values
void pcb_init(pcb_t* pcb, uint32_t pid, const char* name, uint32_t priority) {
    pcb->pid = pid;
    pcb->state = PROCESS_READY;
    pcb->priority = priority;
    pcb->quantum_remaining = 10; // Default quantum of 10 ticks
    pcb->total_runtime = 0;
    
    // Initialize CPU context to 0 (will be set when process runs)
    pcb->eax = 0;
    pcb->ebx = 0;
    pcb->ecx = 0;
    pcb->edx = 0;
    pcb->esi = 0;
    pcb->edi = 0;
    pcb->esp = 0;
    pcb->ebp = 0;
    pcb->eip = 0;
    pcb->eflags = 0;
    
    // Stack information (will be set when allocating stack)
    pcb->stack_base = 0;
    pcb->stack_size = 4096; // 4KB stack
    pcb->stack_pointer = 0;
    
    // Copy process name
    int i = 0;
    while (name[i] != '\0' && i < 31) {
        pcb->name[i] = name[i];
        i++;
    }
    pcb->name[i] = '\0';
    
    pcb->creation_time = 0; // Will be set by scheduler
    
    // Initialize queue pointers
    pcb->next = 0;
    pcb->prev = 0;
}

// Save current CPU context to PCB
void pcb_save_context(pcb_t* pcb) {
    (void)pcb; // Suppress unused parameter warning
    // Note: In a real implementation, this would use assembly to save registers
    // For this simplified version, we'll just mark that context was saved
    // The actual register saving would be done by the context switch routine
}

// Restore CPU context from PCB
void pcb_restore_context(pcb_t* pcb) {
    (void)pcb; // Suppress unused parameter warning
    // Note: In a real implementation, this would use assembly to restore registers
    // For this simplified version, we'll just mark that context was restored
    // The actual register restoring would be done by the context switch routine
}
