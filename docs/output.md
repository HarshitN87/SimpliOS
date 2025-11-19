# Runtime Output Guide

This document explains what you should see on screen when SimpliOS boots in QEMU and what each line or symbol means.

## Boot banner and initialization
- "Welcome to SimpliOS!"
  - The kernel has taken control and initialized VGA text mode output.
- "Initializing subsystems..."
  - Beginning setup of GDT/IDT, memory stubs, and devices.

## GDT dump
You will see something like:
```
GDT: base=xxxxxxxx limit=xxxx
GDT[0]: base=00000000 limit=00000000 access=00 gran=00
GDT[1]: base=00000000 limit=000FFFFF access=9A gran=CF
GDT[2]: base=00000000 limit=000FFFFF access=92 gran=CF
```
- `base`/`limit` are the GDTR values and per-entry fields.
- Entry 0 is the mandatory null descriptor.
- Entry 1 (access=9A, gran=CF) is the kernel code segment (selector 0x08), flat 0..4GiB.
- Entry 2 (access=92, gran=CF) is the kernel data segment (selector 0x10), flat 0..4GiB.

## Memory messages
- "PMM: stub initialized."
  - Physical memory manager placeholder ran; it does not yet manage frames.
- "Paging: enabled identity map for first 4MiB."
  - Paging is on; addresses 0..0x3FFFFF map to themselves. This validates paging setup without relocating the kernel.

## IRQ enablement
- "GDT/IDT, IRQs, PMM (stub) and paging initialized."
  - Descriptor tables are live, interrupts are configured, paging is enabled.
- "Timer and keyboard IRQs enabled. Press keys to see scancodes."
  - The PIT (IRQ0) runs at 100 Hz; the keyboard (IRQ1) is listening.

## Timer output
- A single dot `.` roughly every second.
  - The PIT handler increments a tick and prints a dot every 100 ticks (~1s at 100 Hz).
  - If you see no dots, confirm interrupts are enabled and the QEMU window is focused.

## Keyboard output
- Each key press prints the corresponding ASCII character.
  - Shift modifies letters and common symbols (e.g., `! @ # $ % ^ & * ( )`).
  - Enter prints a newline; Backspace erases the previous character.
  - Key releases (break codes) are ignored except to update Shift state.
- Unsupported keys (function keys, arrows) are currently ignored.

## Example session
```
Welcome to SimpliOS!
Initializing subsystems...
GDT: base=0010ABCD limit=0017
GDT[0]: base=00000000 limit=00000000 access=00 gran=00
GDT[1]: base=00000000 limit=000FFFFF access=9A gran=CF
GDT[2]: base=00000000 limit=000FFFFF access=92 gran=CF
PMM: stub initialized.
Paging: enabled identity map for first 4MiB.
GDT/IDT, IRQs, PMM (stub) and paging initialized.
Timer and keyboard IRQs enabled. Press keys to see scancodes.
.
.
hello, world!
.
```

## Troubleshooting
- No keyboard characters:
  - Ensure the QEMU window has focus.
  - Try `-k en-us` for QEMU: `qemu-system-i386 -cdrom build/simplios.iso -k en-us`.
  - Verify interrupts are enabled (the periodic dots should appear).
- No dots from the timer:
  - Check that your build includes `kernel/irq.c` and that `irq_init()` and `pit_init(100)` are called before `enable_interrupts()`.
- GDT/IDT lines missing:
  - Confirm `gdt_dump()` is called early in `kernel_main`.

## Notes
- All output is drawn directly to VGA text memory at 0xB8000.
- The console does not scroll yet; once the screen fills, it clears and restarts from the top.
