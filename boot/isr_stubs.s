.section .text

.macro ISR_NOERR n
.global isr\n
isr\n:
	cli
	pushl $0           # fake error code for uniformity
	pushl $\n          # push interrupt number
	jmp isr_common_stub
.endm

.macro ISR_ERR n
.global isr\n
isr\n:
	cli
	pushl $\n          # interrupt number
	jmp isr_common_stub
.endm

.extern isr_handler_c
.extern pit_handler
.extern keyboard_handler

.global isr_common_stub
isr_common_stub:
	# On entry stack has: [int_no] or [errcode,int_no]
	push %ds
	push %es
	push %fs
	push %gs

	push %eax
	push %ebx
	push %ecx
	push %edx
	push %esi
	push %edi
	push %ebp

	mov $0x10, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

	# Load (int_no, err_code) from saved stack and pass as args
	mov 44(%esp), %eax    # int_no
	mov 48(%esp), %edx    # err_code (0 for no-err ISRs)
	push %edx
	push %eax
	call isr_handler_c
	add $8, %esp

	pop %ebp
	pop %edi
	pop %esi
	pop %edx
	pop %ecx
	pop %ebx
	pop %eax

	pop %gs
	pop %fs
	pop %es
	pop %ds

	add $8, %esp   # pop errcode and int_no
	sti
	iret

# ISRs 0-31. Those with error codes: 8,10-14,17
ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7
ISR_ERR 8
ISR_NOERR 9
ISR_ERR 10
ISR_ERR 11
ISR_ERR 12
ISR_ERR 13
ISR_ERR 14
ISR_NOERR 15
ISR_NOERR 16
ISR_ERR 17
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_NOERR 21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_NOERR 30
ISR_NOERR 31

# IRQ common stub
.global irq_common_stub
irq_common_stub:
    push %ds
    push %es
    push %fs
    push %gs

    push %eax
    push %ebx
    push %ecx
    push %edx
    push %esi
    push %edi
    push %ebp

    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    # irq_no is at 44(%esp) with this stack layout
    mov 44(%esp), %eax

    # Call high-level handlers for specific IRQs (0: PIT, 1: keyboard)
    cmp $0, %eax
    jne 1f
    call pit_handler
    jmp 2f
1:
    cmp $1, %eax
    jne 2f
    call keyboard_handler
2:
    # Now acknowledge PIC in C (EOI) after device handlers
    push %eax            # arg: irq_no
    call irq_handler_c
    add $4, %esp

    pop %ebp
    pop %edi
    pop %esi
    pop %edx
    pop %ecx
    pop %ebx
    pop %eax

    pop %gs
    pop %fs
    pop %es
    pop %ds

    add $8, %esp   # pop fake errcode and irq_no
    iret

.macro IRQ n
.global irq\n
irq\n:
	cli
	pushl $0           # fake error code for uniform interface
	pushl $\n          # irq number
	jmp irq_common_stub
.endm

IRQ 0
IRQ 1
IRQ 2
IRQ 3
IRQ 4
IRQ 5
IRQ 6
IRQ 7
IRQ 8
IRQ 9
IRQ 10
IRQ 11
IRQ 12
IRQ 13
IRQ 14
IRQ 15


