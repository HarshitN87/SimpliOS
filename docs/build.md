# Build and Run

## Toolchain
You need an i686-elf cross toolchain and GRUB utilities.

- i686-elf-gcc, i686-elf-ld, i686-elf-objcopy
- grub-mkrescue, xorriso
- qemu-system-i386

On Windows, use WSL for easiest setup or install MSYS2/Chocolatey equivalents.

## Commands
```bash
make clean
make
make run
```

Artifacts:
- `build/kernel.elf` – Multiboot kernel
- `build/simplios.iso` – Bootable ISO

## QEMU tips
- Ensure the VM window has focus for keyboard input.
- If needed, try `qemu-system-i386 -cdrom build/simplios.iso -k en-us`.
