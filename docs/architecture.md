# SimpliOS Architecture

## Boot and entry
- Multiboot header in `boot/boot.s` allows GRUB to load the kernel at 1 MiB.
- `_start` sets up a temporary stack and calls `kernel_main(magic, info)`.

## Descriptor tables
- `kernel/gdt.c` creates a minimal GDT: null, kernel code, kernel data.
- `boot/gdt_flush.s` loads GDTR and reloads segment registers via a far jump.
- `kernel/idt.c` builds an IDT with CPU exception ISRs and IRQ entries (32–47).
- `boot/idt_flush.s` loads IDTR.

## Interrupt handling
- `boot/isr_stubs.s` provides stubs for CPU exceptions (0–31) and IRQs (32–47).
- Exceptions call `isr_handler_c(int_no, err_code)` for textual reporting.
- IRQs route to `pit_handler()` and `keyboard_handler()` as needed, then EOI.

## PIC and devices
- `kernel/irq.c` remaps the 8259 PIC to 0x20/0x28 and unmasks IRQ0/IRQ1.
- PIT initialized to 100 Hz prints periodic dots; keyboard echoes ASCII.

## Memory
- `kernel/pmm.c` is a stub; intended to parse Multiboot memory map and manage frames.
- `kernel/paging.c` identity-maps first 4 MiB, loads CR3, sets PG bit in CR0.

## Console
- VGA text-mode renderer in `kernel/kernel.c` draws directly to 0xB8000.
- Minimal terminal with newline and backspace handling.

## Linker
- `linker.ld` places sections at 1 MiB and exports `kernel_start/end` symbols for future PMM use.
