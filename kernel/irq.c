#include <stdint.h>
#include "io.h"
#include "idt.h"

#define PIC1        0x20
#define PIC2        0xA0
#define PIC1_COMMAND PIC1
#define PIC1_DATA   (PIC1+1)
#define PIC2_COMMAND PIC2
#define PIC2_DATA   (PIC2+1)

#define PIC_EOI     0x20

static void pic_remap(int offset1, int offset2) {
	uint8_t a1 = inb(PIC1_DATA);
	uint8_t a2 = inb(PIC2_DATA);

	outb(PIC1_COMMAND, 0x11);
	io_wait();
	outb(PIC2_COMMAND, 0x11);
	io_wait();

	outb(PIC1_DATA, offset1);
	io_wait();
	outb(PIC2_DATA, offset2);
	io_wait();

	outb(PIC1_DATA, 4);
	io_wait();
	outb(PIC2_DATA, 2);
	io_wait();

	outb(PIC1_DATA, 0x01);
	io_wait();
	outb(PIC2_DATA, 0x01);
	io_wait();

	outb(PIC1_DATA, a1);
	outb(PIC2_DATA, a2);
}

extern void irq0();  extern void irq1();  extern void irq2();  extern void irq3();
extern void irq4();  extern void irq5();  extern void irq6();  extern void irq7();
extern void irq8();  extern void irq9();  extern void irq10(); extern void irq11();
extern void irq12(); extern void irq13(); extern void irq14(); extern void irq15();

void irq_init(void) {
	// Remap PIC to 0x20-0x2F
	pic_remap(0x20, 0x28);

	// Unmask PIT (IRQ0) and keyboard (IRQ1); keep others masked for now
	uint8_t mask1 = inb(PIC1_DATA);
	mask1 &= ~0x01; // enable IRQ0
	mask1 &= ~0x02; // enable IRQ1
	outb(PIC1_DATA, mask1);
}

// Called from ASM stub
void irq_handler_c(unsigned int irq_no) {
	if (irq_no >= 8) {
		outb(PIC2_COMMAND, PIC_EOI);
	}
	outb(PIC1_COMMAND, PIC_EOI);
}

// PIT
static volatile uint32_t tick = 0;
void pit_init(unsigned int hz) {
	uint32_t divisor = 1193180 / hz;
	outb(0x43, 0x36);
	outb(0x40, (uint8_t)(divisor & 0xFF));
	outb(0x40, (uint8_t)((divisor >> 8) & 0xFF));
}

extern void term_print(const char*);
extern void term_putc(char c);

void pit_handler(void) {
	++tick;
	if ((tick % 100) == 0) {
		term_print(".");
	}
}

// Keyboard
// Simple US QWERTY set-1 scancode to ASCII mapping (make codes only)
static const char keymap_unshift[128] = {
	[0x01] = 0,      [0x02] = '1', [0x03] = '2', [0x04] = '3', [0x05] = '4', [0x06] = '5', [0x07] = '6', [0x08] = '7',
	[0x09] = '8',    [0x0A] = '9', [0x0B] = '0', [0x0C] = '-', [0x0D] = '=', [0x0E] = '\b', [0x0F] = '\t',
	[0x10] = 'q',    [0x11] = 'w', [0x12] = 'e', [0x13] = 'r', [0x14] = 't', [0x15] = 'y', [0x16] = 'u', [0x17] = 'i',
	[0x18] = 'o',    [0x19] = 'p', [0x1A] = '[', [0x1B] = ']', [0x1C] = '\n',
	[0x1E] = 'a',    [0x1F] = 's', [0x20] = 'd', [0x21] = 'f', [0x22] = 'g', [0x23] = 'h', [0x24] = 'j', [0x25] = 'k',
	[0x26] = 'l',    [0x27] = ';', [0x28] = '\'', [0x29] = '`',
	[0x2C] = 'z',    [0x2D] = 'x', [0x2E] = 'c', [0x2F] = 'v', [0x30] = 'b', [0x31] = 'n', [0x32] = 'm', [0x33] = ',',
	[0x34] = '.',    [0x35] = '/', [0x39] = ' '
};

static const char keymap_shift[128] = {
	[0x01] = 0,      [0x02] = '!', [0x03] = '@', [0x04] = '#', [0x05] = '$', [0x06] = '%', [0x07] = '^', [0x08] = '&',
	[0x09] = '*',    [0x0A] = '(', [0x0B] = ')', [0x0C] = '_', [0x0D] = '+', [0x0E] = '\b', [0x0F] = '\t',
	[0x10] = 'Q',    [0x11] = 'W', [0x12] = 'E', [0x13] = 'R', [0x14] = 'T', [0x15] = 'Y', [0x16] = 'U', [0x17] = 'I',
	[0x18] = 'O',    [0x19] = 'P', [0x1A] = '{', [0x1B] = '}', [0x1C] = '\n',
	[0x1E] = 'A',    [0x1F] = 'S', [0x20] = 'D', [0x21] = 'F', [0x22] = 'G', [0x23] = 'H', [0x24] = 'J', [0x25] = 'K',
	[0x26] = 'L',    [0x27] = ':', [0x28] = '"', [0x29] = '~',
	[0x2C] = 'Z',    [0x2D] = 'X', [0x2E] = 'C', [0x2F] = 'V', [0x30] = 'B', [0x31] = 'N', [0x32] = 'M', [0x33] = '<',
	[0x34] = '>',    [0x35] = '?', [0x39] = ' '
};

static int shift_down = 0;

void keyboard_init(void) {
	shift_down = 0;
}

void keyboard_handler(void) {
	uint8_t sc = inb(0x60);
	if (sc & 0x80) {
		// key release
		uint8_t make = sc & 0x7F;
		if (make == 0x2A || make == 0x36) shift_down = 0; // left/right shift up
		return;
	}
	// key press (make code)
	if (sc == 0x2A || sc == 0x36) { shift_down = 1; return; } // shift down
	char ch = shift_down ? keymap_shift[sc] : keymap_unshift[sc];
	if (ch) {
		term_putc(ch);
	}
}


