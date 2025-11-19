# Build and Run Guide

This guide provides step-by-step instructions for setting up the development environment, building the SimpliOS kernel, and running it in an emulator.

## 1. Development Environment

SimpliOS is developed for a Linux environment. Windows users are strongly recommended to use **WSL (Windows Subsystem for Linux)**.

### 1.1 Setting up WSL (Windows Users)
1. Open PowerShell as Administrator.
2. Run: `wsl --install`
3. Restart your computer.
4. Follow the prompts to create a username and password for Ubuntu.

### 1.2 Install Dependencies
Open your terminal (Ubuntu/WSL) and install the required packages:

```bash
sudo apt-get update
sudo apt-get install build-essential bison flex libgmp-dev libmpc-dev libmpfr-dev texinfo grub-pc-bin grub-common xorriso qemu-system-x86 make
```

## 2. Toolchain Setup (i686-elf)

You need a cross-compiler to build 32-bit x86 code on a modern 64-bit system.

### 2.1 Environment Variables
Add these to your `~/.bashrc` to make them permanent:
```bash
export PREFIX="$HOME/opt/cross"
export TARGET=i686-elf
export PATH="$PREFIX/bin:$PATH"
```
Reload configuration: `source ~/.bashrc`

### 2.2 Build Binutils
```bash
mkdir -p $HOME/src
cd $HOME/src
wget https://ftp.gnu.org/gnu/binutils/binutils-2.42.tar.xz
tar -xf binutils-2.42.tar.xz
cd binutils-2.42
mkdir build-binutils
cd build-binutils
../configure --target=$TARGET --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror
make
sudo make install
```

### 2.3 Build GCC
```bash
cd $HOME/src
wget https://ftp.gnu.org/gnu/gcc/gcc-14.1.0/gcc-14.1.0.tar.gz
tar -xf gcc-14.1.0.tar.gz
cd gcc-14.1.0
mkdir build-gcc
cd build-gcc
../configure --target=$TARGET --prefix="$PREFIX" --disable-nls --enable-languages=c,c++ --without-headers
make all-gcc
make all-target-libgcc
sudo make install-gcc
sudo make install-target-libgcc
```

## 3. Building the Project

Navigate to the project directory:
```bash
cd /path/to/SimpliOS
```

### 3.1 Makefile Targets
- **`make`** (or `make all`): Compiles the kernel and builds the bootable ISO image (`build/simplios.iso`).
- **`make run`**: Builds the project and immediately boots it in QEMU.
- **`make clean`**: Removes the `build/` directory and all generated artifacts.

### 3.2 Build Artifacts
After a successful build, the `build/` directory will contain:
- `kernel.o`, `boot.o`, etc.: Object files.
- `kernel.elf`: The linked kernel executable.
- `simplios.iso`: The final bootable CD-ROM image.

## 4. Running in Emulator

### 4.1 QEMU
The easiest way to run SimpliOS is with QEMU, which is invoked by `make run`.
```bash
qemu-system-i386 -cdrom build/simplios.iso
```

**Tips:**
- **Keyboard Input**: Click inside the QEMU window to capture keyboard input. Press `Ctrl+Alt+G` to release the mouse cursor.
- **Monitor**: Use `Ctrl+Alt+2` to switch to the QEMU monitor console for debugging. `Ctrl+Alt+1` switches back to the display.

## 5. Troubleshooting

### "i686-elf-gcc: command not found"
- **Cause**: The cross-compiler is not in your PATH.
- **Fix**: Run `export PATH="$HOME/opt/cross/bin:$PATH"` or add it to `~/.bashrc`.

### "grub-mkrescue: command not found"
- **Cause**: Missing GRUB tools.
- **Fix**: Install `grub-common` and `xorriso` (see Dependencies).

### "Error: no such instruction" (Assembler)
- **Cause**: The assembler is trying to parse Intel syntax as AT&T or vice versa.
- **Fix**: Ensure `.intel_syntax noprefix` is at the top of assembly files.

### QEMU starts but screen is black
- **Cause**: Bootloader failed or kernel crashed early.
- **Fix**: Check if `multiboot-check` passes. Ensure the linker script places the kernel at 1 MiB.
