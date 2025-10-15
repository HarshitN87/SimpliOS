# boot/boot.s
.set ALIGN,    1<<0
.set MEMINFO,  1<<1
.set FLAGS,    ALIGN | MEMINFO
.set MAGIC,    0x1BADB002
.set CHECKSUM, -(MAGIC + FLAGS)

.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

.section .bss
.align 16
stack_bottom:
.skip 16384  # 16 KiB stack
stack_top:

.section .text
.global _start
.type _start, @function
_start:
    mov $stack_top, %esp
    # multiboot passes: eax=MAGIC, ebx=MB_INFO
    push %ebx
    push %eax
    call kernel_main
    add $8, %esp
    cli
_halt:
    hlt
    jmp _halt
.size _start, . - _start

.global enable_interrupts
.type enable_interrupts, @function
enable_interrupts:
	sti
	ret

.global halt_cpu
.type halt_cpu, @function
halt_cpu:
	hlt
	ret