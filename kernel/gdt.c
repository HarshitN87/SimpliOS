#include "gdt.h"

extern void term_print(const char*);

static gdt_entry_t gdt[3];
static gdt_ptr_t   gdt_ptr;

static void gdt_set_entry(int idx, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran)
{
	gdt[idx].base_low    = (base & 0xFFFF);
	gdt[idx].base_middle = (base >> 16) & 0xFF;
	gdt[idx].base_high   = (base >> 24) & 0xFF;

	gdt[idx].limit_low   = (limit & 0xFFFF);
	gdt[idx].granularity = ((limit >> 16) & 0x0F);
	gdt[idx].granularity |= (gran & 0xF0);

	gdt[idx].access      = access;
}

// Assembly helper to load GDTR and update segment registers
extern void gdt_flush(uint32_t gdt_ptr_addr);

void gdt_init(void)
{
	// Null descriptor
	gdt_set_entry(0, 0, 0, 0, 0);
	// Code segment: base=0, limit=4GiB, executable, readable, present, ring0, 32-bit, 4KiB granularity
	gdt_set_entry(1, 0, 0xFFFFF, 0x9A, 0xCF);
	// Data segment: base=0, limit=4GiB, writable, present, ring0, 32-bit, 4KiB granularity
	gdt_set_entry(2, 0, 0xFFFFF, 0x92, 0xCF);

	gdt_ptr.limit = sizeof(gdt) - 1;
	gdt_ptr.base  = (uint32_t)&gdt[0];

	gdt_flush((uint32_t)&gdt_ptr);
}

static const char* hexdigits = "0123456789ABCDEF";
static void print_hex8(uint8_t v) {
	char s[3];
	s[0] = hexdigits[(v >> 4) & 0xF];
	s[1] = hexdigits[v & 0xF];
	s[2] = '\0';
	term_print(s);
}

static void print_hex32(uint32_t v) {
	print_hex8((v >> 24) & 0xFF);
	print_hex8((v >> 16) & 0xFF);
	print_hex8((v >> 8) & 0xFF);
	print_hex8(v & 0xFF);
}

void gdt_dump(void) {
	term_print("GDT: base=");
	print_hex32(gdt_ptr.base);
	term_print(" limit=");
	char lim_hi = (gdt_ptr.limit >> 8) & 0xFF;
	char lim_lo = gdt_ptr.limit & 0xFF;
	print_hex8((uint8_t)lim_hi);
	print_hex8((uint8_t)lim_lo);
	term_print("\n");
	for (int i = 0; i < 3; i++) {
		term_print("GDT[");
		char idx = '0' + i;
		char s[2] = { idx, 0 };
		term_print(s);
		term_print("]: base=");
		uint32_t base = (gdt[i].base_high << 24) | (gdt[i].base_middle << 16) | gdt[i].base_low;
		uint32_t limit = ((gdt[i].granularity & 0x0F) << 16) | gdt[i].limit_low;
		print_hex32(base);
		term_print(" limit=");
		print_hex32(limit);
		term_print(" access=");
		print_hex8(gdt[i].access);
		term_print(" gran=");
		print_hex8(gdt[i].granularity);
		term_print("\n");
	}
}


