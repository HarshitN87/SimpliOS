/* src/kernel.c */
#include <stdint.h>

// VGA text mode buffer is located at 0xB8000.
volatile unsigned short* vga_buffer = (unsigned short*)0xB8000;
const int VGA_COLS = 80;
const int VGA_ROWS = 25;

// We'll keep track of the cursor position.
int term_col = 0;
int term_row = 0;
unsigned char term_color = 0x0F; // White on black

// Clears the terminal screen.
void term_clear() {
    for (int r = 0; r < VGA_ROWS; r++) {
        for (int c = 0; c < VGA_COLS; c++) {
            const int index = r * VGA_COLS + c;
            vga_buffer[index] = ((unsigned short)term_color << 8) | ' ';
        }
    }
    term_col = 0;
    term_row = 0;
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

// Forward decls for descriptor tables and IRQ/Paging/PMM
void gdt_init(void);
void idt_init(void);
void irq_init(void);
void pit_init(unsigned int hz);
void keyboard_init(void);
void pmm_init(unsigned int mb_magic, unsigned int mb_info);
void paging_init(void);
void gdt_dump(void);

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
    term_print("Timer and keyboard IRQs enabled. Press keys to see scancodes.\n");

    extern void enable_interrupts();
    extern void halt_cpu();
    enable_interrupts();
    for (;;) { halt_cpu(); }
}