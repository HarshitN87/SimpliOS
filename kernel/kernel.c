/* src/kernel.c */

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

// The main entry point for our C kernel.
void kernel_main(void) {
    term_clear();
    term_print("Welcome to SimpliOS!\n");
    term_print("Milestone 1 Complete: Bootloader and Basic Kernel Initialized.");
}