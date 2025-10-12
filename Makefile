# Makefile
CC = i686-elf-gcc
LD = i686-elf-ld
OBJCOPY = i686-elf-objcopy
QEMU = qemu-system-i386

CFLAGS = -std=gnu99 -ffreestanding -fno-stack-protector -m32 -Wall -Wextra
LDFLAGS = -T linker.ld -nostdlib

BUILD_DIR = build
SRC_DIR = .
BOOT_SRC = boot/boot.s
KERNEL_SRC = kernel/kernel.c

BOOT_OBJ = $(BUILD_DIR)/boot.o
KERNEL_OBJ = $(BUILD_DIR)/kernel.o
KERNEL_ELF = $(BUILD_DIR)/kernel.elf
ISO_DIR = $(BUILD_DIR)/isodir
ISO_FILE = $(BUILD_DIR)/simplios.iso

all: $(ISO_FILE)

$(BUILD_DIR):
	mkdir -p $@

$(BOOT_OBJ): $(BOOT_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL_OBJ): $(KERNEL_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL_ELF): $(BOOT_OBJ) $(KERNEL_OBJ)
	$(LD) $(LDFLAGS) -o $@ $^

$(ISO_FILE): $(KERNEL_ELF)
	mkdir -p $(ISO_DIR)/boot/grub
	cp $< $(ISO_DIR)/boot/kernel.elf
	echo 'set timeout=0' > $(ISO_DIR)/boot/grub/grub.cfg
	echo 'set default=0' >> $(ISO_DIR)/boot/grub/grub.cfg
	echo '' >> $(ISO_DIR)/boot/grub/grub.cfg
	echo 'menuentry "SimpliOS" {' >> $(ISO_DIR)/boot/grub/grub.cfg
	echo '    multiboot /boot/kernel.elf' >> $(ISO_DIR)/boot/grub/grub.cfg
	echo '    boot' >> $(ISO_DIR)/boot/grub/grub.cfg
	echo '}' >> $(ISO_DIR)/boot/grub/grub.cfg
	grub-mkrescue -o $@ $(ISO_DIR) 2>/dev/null

run: $(ISO_FILE)
	$(QEMU) -cdrom $<

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all run clean