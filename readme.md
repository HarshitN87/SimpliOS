# SimpliOS - A Simple, Educational Operating System

Welcome to **SimpliOS**! This project is an educational 32-bit monolithic kernel built from the ground up to demonstrate core operating system concepts. It is designed as a learning tool for those interested in low-level systems programming.

This guide provides all the necessary steps to set up the development environment, build the cross-compiler, and run the operating system in the QEMU emulator.

---

## 🚀 Getting Started

These instructions guide you through setting up the project on a **Windows machine** using the **Windows Subsystem for Linux (WSL)**.

---

### 1. Prerequisites: Setting up the Environment

The build tools for this project are designed for a Linux environment. The easiest way to get this on Windows is by installing WSL.

#### A. Install Windows Subsystem for Linux (WSL)

1. Open **PowerShell** as an Administrator.
2. Run the following command to install WSL and the default Ubuntu distribution:

```bash
wsl --install
```

3. Restart your computer. After restart, an Ubuntu terminal will open to complete the installation. You will need to create a username and password.

#### B. Install Build Dependencies

Once WSL/Ubuntu is running, open the Ubuntu terminal and run:

```bash
sudo apt-get update
sudo apt-get install build-essential bison flex libgmp-dev libmpc-dev libmpfr-dev texinfo grub-pc-bin grub-common xorriso qemu-system-x86
```

---

### 2. Building the i686-elf Cross-Compiler

To compile code for our 32-bit target OS, we need a **cross-compiler**.

#### A. Set Up Environment Variables

```bash
export PREFIX="$HOME/opt/cross"
export TARGET=i686-elf
export PATH="$PREFIX/bin:$PATH"
```

#### B. Download and Build Binutils

Binutils contains the assembler and linker.

```bash
# Create a directory for source code
mkdir -p $HOME/src
cd $HOME/src

# Download and extract Binutils 2.42
wget https://ftp.gnu.org/gnu/binutils/binutils-2.42.tar.xz
tar -xf binutils-2.42.tar.xz

# Configure and build
cd binutils-2.42
mkdir build-binutils
cd build-binutils
../configure --target=$TARGET --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror
make
sudo make install
```

#### C. Download and Build GCC

This step builds the C compiler. **Note:** This process can take 15–45 minutes.

```bash
# Navigate back to the source directory
cd $HOME/src

# Download and extract GCC 14.1.0
wget https://ftp.gnu.org/gnu/gcc/gcc-14.1.0/gcc-14.1.0.tar.gz
tar -xf gcc-14.1.0.tar.gz

# Configure and build
cd gcc-14.1.0
mkdir build-gcc
cd build-gcc
../configure --target=$TARGET --prefix="$PREFIX" --disable-nls --enable-languages=c,c++ --without-headers
make all-gcc
make all-target-libgcc
sudo make install-gcc
sudo make install-target-libgcc
```

#### D. Make the Compiler Path Permanent

```bash
echo 'export PATH="$HOME/opt/cross/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc
```

---

### 3. Building and Running SimpliOS

With the environment fully configured, you can now build and run the OS.

1. Navigate to the project directory (WSL path format: `/mnt/c/...`):

```bash
cd /mnt/c/Users/YourUser/simplios
```

2. Clean previous builds (optional):

```bash
make clean
```

3. Build and run the operating system:

```bash
make run
```

A **QEMU window** should appear, booting your OS and displaying the welcome message.

---

## 📂 Project Structure

```text
simplios/
├── src/
│   ├── boot.s        # Assembly entry point and multiboot header
│   └── kernel.c      # Main C kernel code
├── linker.ld         # Linker script to structure the kernel binary
├── Makefile          # Automates the build and run process
└── README.md         # This file
```

---

## ❔ Troubleshooting

- **make: i686-elf-gcc: No such file or directory**  
  The cross-compiler is not in your `PATH`. Run `source ~/.bashrc` to fix it for the current session.

- **Assembly Errors in `boot.s`**  
  If you see errors like `Error: no such instruction:`, it means the assembler is misinterpreting the syntax. Ensure the first line of `src/boot.s` is:

```asm
.intel_syntax noprefix
```


