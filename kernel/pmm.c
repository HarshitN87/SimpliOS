#include <stdint.h>
#include "io.h"

extern void term_print(const char*);

// Very small stub PMM: just acknowledge multiboot and print a message.
// Proper implementation would parse multiboot memory map and build a bitmap.
void pmm_init(uint32_t mb_magic, uint32_t mb_info) {
	(void)mb_magic; (void)mb_info;
	term_print("PMM: stub initialized.\n");
}


