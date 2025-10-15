#ifndef IRQ_H
#define IRQ_H

#include <stdint.h>

void irq_init(void);
void pit_init(uint32_t hz);
void keyboard_init(void);

void pit_handler(void);
void keyboard_handler(void);

#endif // IRQ_H


