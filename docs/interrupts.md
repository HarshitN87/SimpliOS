# Interrupts

## Overview
- GDT sets flat, ring-0 code/data segments (selectors 0x08/0x10).
- IDT contains 256 entries; CPU exceptions (0–31), hardware IRQs (32–47).

## CPU exceptions
- Stubs in `boot/isr_stubs.s` push an error code (0 if none) and int number.
- `isr_handler_c(int_no, err_code)` prints a brief message.

## PIC remap
- The 8259 PICs are remapped from 0x08/0x70 to 0x20/0x28 to avoid overlap with exceptions.
- Only IRQ0 (PIT) and IRQ1 (keyboard) are unmasked.

## IRQ flow
1. Device asserts IRQ line.
2. PIC delivers vector (32–47) to CPU.
3. ASM stub calls `irq_common_stub` then device-specific handlers.
4. EOI is sent to PIC (slave then master when needed).

## PIT (timer)
- `pit_init(100)` programs channel 0 to 100 Hz.
- `pit_handler()` increments a tick and prints a dot every ~1s.

## Keyboard
- Reads scancode from port 0x60.
- Translates set-1 scancodes to ASCII (with Shift), ignores release codes.
- Echoes characters via VGA text mode.
