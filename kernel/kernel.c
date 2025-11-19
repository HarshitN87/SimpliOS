/* src/kernel.c */
#include <stdint.h>

// VGA text mode buffer is located at 0xB8000.
volatile unsigned short* vga_buffer = (unsigned short*)0xB8000;
const int VGA_COLS = 80;
const int VGA_ROWS = 25;

// We'll keep track of the cursor position.
int term_col = 0;
int term_row = 0;
uint8_t term_color = 0x0F; // White on black
uint8_t term_default_color = 0x0F;

// Clears the terminal screen.
void term_clear() {
    for (int r = 0; r < VGA_ROWS; r++) {
        for (int c = 0; c < VGA_COLS; c++) {
            const int index = r * VGA_COLS + c;
            vga_buffer[index] = ((unsigned short)term_default_color << 8) | ' ';
        }
    }
    term_col = 0;
    term_row = 0;
    term_color = term_default_color;
}

// Prints a single character to the screen.
void term_putc(char c) {
	if (c == '\n') {
		term_col = 0;
		term_row++;
	} else if (c == '\b') {
		if (term_col > 0) {
			term_col--;
			const int index = term_row * VGA_COLS + term_col;
			vga_buffer[index] = ((unsigned short)term_color << 8) | ' ';
		}
	} else {
		const int index = term_row * VGA_COLS + term_col;
		vga_buffer[index] = ((unsigned short)term_color << 8) | c;
		term_col++;
	}

	if (term_col >= VGA_COLS) {
		term_col = 0;
		term_row++;
	}

	if (term_row >= VGA_ROWS) {
		// For simplicity, we just reset to the top.
		// A real terminal would scroll.
		term_clear();
	}
}

// Prints a string to the screen.
void term_print(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        term_putc(str[i]);
    }
}

// Terminal color helpers
uint8_t term_get_color(void) {
    return term_color;
}

void term_set_color(uint8_t color) {
    term_color = color;
}

uint8_t term_get_default_color(void) {
    return term_default_color;
}

void term_set_default_color(uint8_t color) {
    term_default_color = color;
    term_color = color;
}

void term_reset_color(void) {
    term_color = term_default_color;
}

// Move cursor left without erasing
void term_move_left(void) {
    if (term_col > 0) {
        term_col--;
    }
}

// Move cursor right without printing
void term_move_right(void) {
    if (term_col < VGA_COLS - 1) {
        term_col++;
    }
}

// Clear from cursor to end of line
void term_clear_to_eol(void) {
    int start_col = term_col;
    for (int c = start_col; c < VGA_COLS; c++) {
        const int index = term_row * VGA_COLS + c;
        vga_buffer[index] = ((unsigned short)term_color << 8) | ' ';
    }
}

// Get current cursor column (for shell use)
int term_get_col(void) {
    return term_col;
}

// Set cursor column (for shell use)
void term_set_col(int col) {
    if (col >= 0 && col < VGA_COLS) {
        term_col = col;
    }
}

// Get current cursor row (for shell use)
int term_get_row(void) {
    return term_row;
}

// Set cursor row (for shell use)
void term_set_row(int row) {
    if (row >= 0 && row < VGA_ROWS) {
        term_row = row;
    }
}

// Forward decls for descriptor tables and IRQ/Paging/PMM
void gdt_init(void);
void idt_init(void);
void irq_init(void);
void pit_init(unsigned int hz);
void keyboard_init(void);
void pmm_init(unsigned int mb_magic, unsigned int mb_info);
void paging_init(void);
void gdt_dump(void);

// Forward decls for scheduler
void scheduler_init(void);
void scheduler_add_process(const char* name, uint32_t priority);
void scheduler_tick(void);
void scheduler_print_status(void);

// Forward decls for ramdisk and shell
void ramdisk_init(void);
void ramdisk_create_samples(void);
void shell_init(void);

// C-level ISR handler to print an error message
static const char* exception_messages[32] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Into Detected Overflow",
    "Out of Bounds",
    "Invalid Opcode",
    "No Coprocessor",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Bad TSS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Unknown Interrupt",
    "Coprocessor Fault",
    "Alignment Check",
    "Machine Check",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved"
};

void isr_handler_c(unsigned int int_no, unsigned int err_code) {
    (void)err_code;
    term_print("\nException: ");
    if (int_no < 32) {
        term_print(exception_messages[int_no]);
    } else {
        term_print("Unknown");
    }
    term_print(" (int ");
    // Print small int value
    char buf[4];
    buf[0] = '0' + (char)((int_no / 10) % 10);
    buf[1] = '0' + (char)(int_no % 10);
    buf[2] = ')';
    buf[3] = '\0';
    term_print(buf);
}

// The main entry point for our C kernel.
void kernel_main(unsigned int mb_magic, unsigned int mb_info) {
    gdt_init();
    idt_init();

    term_clear();
    term_print("Welcome to SimpliOS!\n");
    term_print("Initializing subsystems...\n");
    gdt_dump();

    pmm_init(mb_magic, mb_info);
    paging_init();

    irq_init();
    pit_init(100); // 100 Hz
    keyboard_init();

    term_print("GDT/IDT, IRQs, PMM (stub) and paging initialized.\n");
    
    // Initialize ramdisk filesystem
    ramdisk_init();
    ramdisk_create_samples();
    
    // Initialize scheduler and add OS-style processes
    scheduler_init();
    scheduler_add_process("kernel_init", 0);  // System initialization (highest priority)
    scheduler_add_process("vfs", 1);          // Virtual filesystem daemon
    scheduler_add_process("shell", 2);        // Shell process
    scheduler_add_process("syslog", 3);       // System logging daemon
    
    // Initialize shell
    shell_init();
    
    term_print("Scheduler initialized with 4 processes.\n");
    term_print("Ramdisk filesystem initialized.\n");
    term_print("Shell ready. Type 'help' for available commands.\n");

    extern void enable_interrupts();
    extern void halt_cpu();
    enable_interrupts();
    
    extern volatile void (*g_main_loop_hook)(void);
    
    for (;;) {
        if (g_main_loop_hook) {
            g_main_loop_hook();
        }
        halt_cpu();
    }
}

// Global hook for main loop
volatile void (*g_main_loop_hook)(void) = 0;