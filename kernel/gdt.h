#ifndef GDT_H
#define GDT_H

#include <stdint.h>

// GDT entry structure (32-bit protected mode)
typedef struct __attribute__((packed)) {
	uint16_t limit_low;      // Lower 16 bits of the limit
	uint16_t base_low;       // Lower 16 bits of the base
	uint8_t  base_middle;    // Next 8 bits of the base
	uint8_t  access;         // Access flags
	uint8_t  granularity;    // Granularity and high 4 bits of limit
	uint8_t  base_high;      // Highest 8 bits of the base
} gdt_entry_t;

// GDTR structure
typedef struct __attribute__((packed)) {
	uint16_t limit;          // Size of GDT - 1
	uint32_t base;           // Address of first gdt_entry_t
} gdt_ptr_t;

void gdt_init(void);
void gdt_dump(void);

#endif // GDT_H


