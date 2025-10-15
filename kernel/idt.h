#ifndef IDT_H
#define IDT_H

#include <stdint.h>

typedef struct __attribute__((packed)) {
	uint16_t offset_low;   // lower 16 bits of handler address
	uint16_t selector;     // code segment selector
	uint8_t  zero;         // must be zero
	uint8_t  type_attr;    // type and attributes
	uint16_t offset_high;  // higher 16 bits of handler address
} idt_entry_t;

typedef struct __attribute__((packed)) {
	uint16_t limit;        // size of IDT - 1
	uint32_t base;         // base address of first idt_entry_t
} idt_ptr_t;

void idt_init(void);

#endif // IDT_H


