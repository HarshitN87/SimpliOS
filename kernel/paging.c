#include <stdint.h>

extern void term_print(const char*);

static uint32_t __attribute__((aligned(4096))) page_directory[1024];
static uint32_t __attribute__((aligned(4096))) first_page_table[1024];

void paging_init(void) {
	for (int i = 0; i < 1024; i++) {
		first_page_table[i] = (i * 0x1000) | 3; // present, rw
		page_directory[i] = 0;
	}
	page_directory[0] = ((uint32_t)first_page_table) | 3; // identity map first 4MiB

	// load page directory
	__asm__ __volatile__(
		"mov %0, %%cr3\n\t"
		"mov %%cr0, %%eax\n\t"
		"or $0x80000000, %%eax\n\t"
		"mov %%eax, %%cr0\n\t"
		: : "r"(page_directory) : "eax");

	term_print("Paging: enabled identity map for first 4MiB.\n");
}


