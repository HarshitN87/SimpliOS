# SimpliOS Architecture

This document details the internal architecture of SimpliOS, covering the boot process, memory management, interrupt handling, and the kernel main loop.

## 1. Boot Process

The boot process is the sequence of events that takes the system from power-on to the kernel's `main()` function.

### 1.1 Multiboot Header (`boot/boot.s`)
SimpliOS relies on a Multiboot-compliant bootloader (like GRUB). The kernel binary includes a Multiboot header within the first 8 KiB, containing:
- **Magic Number**: `0x1BADB002` identifies the kernel as Multiboot-compliant.
- **Flags**: `0x03` (Align modules on page boundaries, provide memory map).
- **Checksum**: Ensures the header is valid.

### 1.2 Entry Point (`_start`)
The linker script (`linker.ld`) specifies `_start` as the entry point.
1. **Stack Setup**: A small stack (16 KiB) is reserved in the `.bss` section. The stack pointer (`esp`) is initialized to the top of this stack.
2. **Multiboot Info**: The bootloader passes a pointer to the Multiboot Information Structure in `ebx` and a magic number in `eax`. These are pushed onto the stack as arguments for `kernel_main`.
3. **Kernel Jump**: The assembly code calls `kernel_main`, passing control to C code.

## 2. Memory Management

SimpliOS uses a flat memory model and basic paging.

### 2.1 Global Descriptor Table (GDT)
The GDT is set up in `kernel/gdt.c` and loaded in `boot/gdt_flush.s`. It defines three segments:
- **Null Descriptor**: Required by the CPU (Index 0).
- **Kernel Code**: Base `0x0`, Limit `0xFFFFFFFF`, Type `0x9A` (Execute/Read, Ring 0).
- **Kernel Data**: Base `0x0`, Limit `0xFFFFFFFF`, Type `0x92` (Read/Write, Ring 0).

This configuration creates a "flat" 4 GiB address space where logical addresses map directly to linear addresses.

### 2.2 Paging (`kernel/paging.c`)
Paging is enabled to demonstrate virtual memory capabilities.
- **Page Directory**: A single page directory is created.
- **Identity Mapping**: The first 4 MiB of physical memory are identity-mapped to virtual addresses `0x00000000` - `0x003FFFFF`.
- **Enabling**: The address of the page directory is loaded into `CR3`, and the Paging Enable (PG) bit is set in `CR0`.

### 2.3 Physical Memory Manager (PMM) (`kernel/pmm.c`)
The PMM is a stub implementation intended to manage physical RAM frames. It parses the Multiboot memory map to identify available RAM regions.

## 3. Interrupt Handling

Interrupts are crucial for handling hardware events (timer, keyboard) and CPU exceptions.

### 3.1 Interrupt Descriptor Table (IDT) (`kernel/idt.c`)
The IDT tells the CPU where to jump when an interrupt occurs. SimpliOS sets up 256 entries:
- **0-31**: CPU Exceptions (Division by zero, Page Fault, etc.).
- **32-47**: Remapped Hardware Interrupts (IRQs).
- **48-255**: Available for software interrupts (syscalls).

### 3.2 Programmable Interrupt Controller (PIC) (`kernel/irq.c`)
The 8259 PIC is remapped because its default vector offsets (0x08-0x0F) conflict with CPU exceptions.
- **Master PIC**: Remapped to offset `0x20` (Interrupts 32-39).
- **Slave PIC**: Remapped to offset `0x28` (Interrupts 40-47).

### 3.3 Interrupt Service Routines (ISRs)
- **Stubs (`boot/isr_stubs.s`)**: Assembly wrappers that save the CPU state (registers), push the interrupt number, and call the C handler.
- **Handler (`kernel/kernel.c:isr_handler_c`)**: A generic C function that handles exceptions by printing an error message.
- **IRQ Handlers**: Specific handlers for hardware devices:
  - **IRQ0 (Timer)**: Calls `scheduler_tick()`.
  - **IRQ1 (Keyboard)**: Reads scancodes from port `0x60`.

## 4. Drivers

### 4.1 VGA Console (`kernel/kernel.c`)
- **Memory**: Writes directly to video memory at `0xB8000`.
- **Format**: Each character is 2 bytes: ASCII code + Attribute byte (Foreground/Background color).
- **Features**: Scrolling, color support, cursor positioning.

### 4.2 Keyboard (`kernel/irq.c`)
- **Controller**: PS/2 Controller.
- **Operation**: Reads scancodes from I/O port `0x60` on IRQ1.
- **Translation**: Converts Scancode Set 1 to ASCII characters, handling Shift state and special keys.

### 4.3 Programmable Interval Timer (PIT) (`kernel/irq.c`)
- **Frequency**: Configured to fire at 100 Hz (every 10ms).
- **Usage**: Drives the scheduler preemption and system uptime tracking.

## 5. Process Scheduler (`kernel/scheduler.c`)

SimpliOS implements a simple Round-Robin scheduler.

### 5.1 Process Control Block (PCB)
Each process is represented by a `pcb_t` structure containing:
- **PID**: Unique Process ID.
- **Name**: Human-readable name.
- **State**: READY, RUNNING, TERMINATED.
- **Context**: Saved CPU registers (ESP, EBP, EIP, etc.).
- **Stack**: A dedicated kernel stack for the process.

### 5.2 Scheduling Algorithm
1. **Tick**: The PIT interrupt calls `scheduler_tick()`.
2. **Quantum**: The current process's time slice is decremented.
3. **Preemption**: If the quantum reaches zero, `scheduler_yield()` is called.
4. **Switch**: The scheduler saves the current context, picks the next READY process from the circular queue, and restores its context.

## 6. Filesystem (`kernel/ramdisk.c`)

A simple in-memory filesystem (Ramdisk).
- **Storage**: A contiguous block of memory (`g_ramdisk.data`).
- **Metadata**: An array of `file_entry_t` structures tracking filenames, sizes, and offsets.
- **Persistence**: None. Files are lost on reboot.

## 7. Main Loop

After initialization, `kernel_main` enters an infinite loop:
1. **Hook Check**: Checks if `g_main_loop_hook` is set (used by games like Breakout).
2. **Halt**: Executes `hlt` instruction to wait for the next interrupt, saving power.
