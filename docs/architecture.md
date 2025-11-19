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

## Ramdisk filesystem
- `kernel/ramdisk.c` implements an in-memory block with fixed-size metadata entries and contiguous data storage.
- Each `file_entry_t` tracks filename, size, timestamps, file type, permissions, and data offset; free slots are zeroed at boot.
- Helper routines provide `ls`, `cat/read`, `write`, `create`, and `delete` semantics used by the shell.
- `ramdisk_create_samples()` seeds a few demo files so commands immediately have something to inspect.

## Shell
- `kernel/shell.c` exposes a `simplios>` prompt with simple line editing (printable ASCII filter, backspace, newline).
- Commands are defined in a table and dispatched via string compares; current set: `help`, `clear`, `ls`, `cat`, `echo`, `create`, `delete`, `write`, `read`, `status`.
- Command handlers call into ramdisk helpers, the terminal, or the scheduler to display information.
- The shell state machine keeps track of the current line buffer, cursor, and echo settings so future extensions can hook into it.

## Game Loop Hook
- `kernel/kernel.c` exposes a `g_main_loop_hook` function pointer.
- This allows applications like `breakout` to hook into the main kernel loop (running in the idle task context) instead of blocking inside an interrupt handler.
- `kernel/breakout.c` implements the game logic, rendering, and input handling, using this hook for smooth animation.

## Scheduler
- `kernel/scheduler.c` provides a round-robin scheduler with a circular ready queue backed by `pcb_t` entries from `kernel/pcb.c`.
- Each PCB tracks PID, name, priority, runtime stats, and saved register context to allow future task switching.
- `scheduler_tick()` consumes PIT interrupts, decrements a quantum counter, and calls `scheduler_yield()` when a task's slice expires.
- Demo tasks (`task_idle`, `task_counter`, `task_printer`) show periodic log messages so you can see scheduling activity before user programs exist.

## Linker
- `linker.ld` places sections at 1 MiB and exports `kernel_start/end` symbols for future PMM use.
