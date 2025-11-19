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
KERNEL_SRC = kernel/kernel.c kernel/gdt.c kernel/idt.c kernel/irq.c kernel/pmm.c kernel/paging.c kernel/pcb.c kernel/scheduler.c kernel/ramdisk.c kernel/shell.c kernel/breakout.c

BOOT_OBJ = $(BUILD_DIR)/boot.o $(BUILD_DIR)/gdt_flush.o $(BUILD_DIR)/idt_flush.o $(BUILD_DIR)/isr_stubs.o
KERNEL_OBJ = $(BUILD_DIR)/kernel.o $(BUILD_DIR)/gdt.o $(BUILD_DIR)/idt.o $(BUILD_DIR)/irq.o $(BUILD_DIR)/pmm.o $(BUILD_DIR)/paging.o $(BUILD_DIR)/pcb.o $(BUILD_DIR)/scheduler.o $(BUILD_DIR)/ramdisk.o $(BUILD_DIR)/shell.o $(BUILD_DIR)/breakout.o
KERNEL_ELF = $(BUILD_DIR)/kernel.elf
ISO_DIR = $(BUILD_DIR)/isodir
ISO_FILE = $(BUILD_DIR)/simplios.iso

all: $(ISO_FILE)

$(BUILD_DIR):
	mkdir -p $@

$(BOOT_OBJ): $(BOOT_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/gdt_flush.o: boot/gdt_flush.s | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/idt_flush.o: boot/idt_flush.s | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/isr_stubs.o: boot/isr_stubs.s | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/kernel.o: kernel/kernel.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/gdt.o: kernel/gdt.c kernel/gdt.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c kernel/gdt.c -o $@

$(BUILD_DIR)/idt.o: kernel/idt.c kernel/idt.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c kernel/idt.c -o $@

$(BUILD_DIR)/irq.o: kernel/irq.c kernel/irq.h kernel/io.h kernel/idt.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c kernel/irq.c -o $@

$(BUILD_DIR)/pmm.o: kernel/pmm.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c kernel/pmm.c -o $@

$(BUILD_DIR)/paging.o: kernel/paging.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c kernel/paging.c -o $@

$(BUILD_DIR)/pcb.o: kernel/pcb.c kernel/pcb.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c kernel/pcb.c -o $@

$(BUILD_DIR)/scheduler.o: kernel/scheduler.c kernel/scheduler.h kernel/pcb.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c kernel/scheduler.c -o $@

$(BUILD_DIR)/ramdisk.o: kernel/ramdisk.c kernel/ramdisk.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c kernel/ramdisk.c -o $@

$(BUILD_DIR)/shell.o: kernel/shell.c kernel/shell.h kernel/ramdisk.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c kernel/shell.c -o $@

$(BUILD_DIR)/breakout.o: kernel/breakout.c kernel/breakout.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c kernel/breakout.c -o $@

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