# SimpliOS - A Simple, Educational Operating System

SimpliOS is a tiny, yet feature-rich 32-bit x86 educational kernel designed to demonstrate the core concepts of operating system development. It boots via Multiboot, sets up a protected mode environment, handles hardware interrupts, manages memory, and provides a functional userspace environment with a shell and interactive applications.

This project is built from the ground up to be readable, extendable, and educational. It avoids complex abstractions in favor of clear, direct hardware manipulation, making it an ideal starting point for learning about OS internals.

## 🌟 Key Features

### Core Kernel
- **Multiboot Compliant**: Boots using GRUB or any Multiboot-compliant bootloader.
- **Protected Mode**: Runs in 32-bit protected mode with a flat Global Descriptor Table (GDT).
- **Interrupt Handling**: Full Interrupt Descriptor Table (IDT) for CPU exceptions and hardware interrupts.
- **Programmable Interrupt Controller (PIC)**: Remapped IRQs to avoid conflicts with CPU exceptions.
- **System Timer**: Programmable Interval Timer (PIT) configured for 100 Hz system tick.
- **Memory Management**:
  - Physical Memory Manager (PMM) stub for frame allocation.
  - Paging enabled with identity mapping for the first 4 MiB.

### Drivers & Hardware
- **VGA Text Mode**: Direct memory access to 0xB8000 for fast text rendering (80x25).
- **Keyboard Driver**: PS/2 keyboard driver with support for:
  - Printable ASCII characters.
  - Shift state tracking.
  - Special keys (Enter, Backspace, Arrows).
  - Scancode set 1 decoding.

### Filesystem & Storage
- **Ramdisk Filesystem**: A custom in-memory filesystem.
  - Flat file structure.
  - Support for file attributes (size, type, permissions).
  - Operations: Create, Read, Write, Delete, List.
  - Pre-loaded sample files (`welcome.txt`, `readme.txt`, `version.txt`).

### Process Management
- **Round-Robin Scheduler**: Preemptive multitasking.
- **Process Control Blocks (PCB)**: Tracks PID, name, state, priority, and CPU context.
- **Context Switching**: Saves and restores register state (EAX, EBX, ECX, EDX, ESI, EDI, EBP, ESP, EIP, EFLAGS).
- **System Tasks**: Background tasks simulating OS services (`kernel_init`, `vfs`, `shell`, `syslog`).

### User Experience
- **Interactive Shell**: A robust command-line interface.
  - Command history (Up/Down arrows).
  - Line editing (Left/Right arrows, Backspace).
  - Tab completion (basic).
  - Colorized output (Prompt, Info, Warnings, Errors).
- **Built-in Commands**:
  - `help`: List available commands.
  - `clear`: Clear the screen.
  - `ls`: List files.
  - `cat`/`read`: View file contents.
  - `create`/`delete`: Manage files.
  - `write`: Edit files.
  - `status`: View system resource usage.
  - `ps`: List active processes.
  - `uptime`: Show system uptime.
  - `setcolor`: Customize terminal colors.
  - `calc`: Basic arithmetic calculator.
  - `breakout`: Play the Breakout game.

### Applications
- **Breakout Game**: A fully functional arcade game running directly on the kernel.
  - 60 FPS smooth rendering using a main loop hook.
  - Collision detection (Walls, Paddle, Bricks).
  - Scoring and Lives system.
  - Interactive controls.

## 📂 Repository Structure

```text
SimpliOS/
├── boot/               # Bootloader assembly and configuration
│   ├── boot.s          # Multiboot header and entry point
│   ├── gdt_flush.s     # GDT loading assembly
│   ├── idt_flush.s     # IDT loading assembly
│   └── isr_stubs.s     # Interrupt Service Routines stubs
├── kernel/             # Core kernel source code
│   ├── kernel.c        # Kernel entry and initialization
│   ├── gdt.c/h         # Global Descriptor Table management
│   ├── idt.c/h         # Interrupt Descriptor Table management
│   ├── irq.c/h         # Interrupt Request handling
│   ├── paging.c/h      # Memory paging
│   ├── pmm.c/h         # Physical Memory Manager
│   ├── scheduler.c/h   # Process scheduler
│   ├── ramdisk.c/h     # In-memory filesystem
│   ├── shell.c/h       # Command-line interface
│   ├── breakout.c/h    # Breakout game application
│   └── ...
├── docs/               # Detailed documentation
├── build/              # Build artifacts (ISO, ELF)
├── linker.ld           # Linker script
└── Makefile            # Build automation
```

## 🚀 Quick Start

### Prerequisites
You need a cross-compiler toolchain targeting `i686-elf`. The recommended environment is **WSL (Windows Subsystem for Linux)** or a native Linux distribution.

**Install Dependencies (Ubuntu/Debian/WSL):**
```bash
sudo apt-get update
sudo apt-get install build-essential bison flex libgmp-dev libmpc-dev libmpfr-dev texinfo grub-pc-bin grub-common xorriso qemu-system-x86
```

### Build & Run
1. **Clean** the build directory:
   ```bash
   make clean
   ```
2. **Compile** the kernel and build the ISO:
   ```bash
   make
   ```
3. **Run** in QEMU:
   ```bash
   make run
   ```

## 📚 Documentation
For deep dives into specific subsystems, check the `docs/` directory:

- [**Architecture**](docs/architecture.md): Boot flow, memory map, and kernel design.
- [**Build Guide**](docs/build.md): Detailed toolchain setup and build instructions.
- [**Features**](docs/features.md): User manual for the Shell, Game, and Tools.
- [**API Reference**](docs/api.md): Internal kernel APIs for developers.

## 🤝 Contributing
This is an educational project, and contributions are welcome! Whether it's fixing a bug, adding a new shell command, or implementing a new driver, feel free to open a pull request.

## 📄 License
This project is open-source and available for educational use.



