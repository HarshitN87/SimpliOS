.section .text
.global gdt_flush
.type gdt_flush, @function
gdt_flush:
	# arg: pointer to gdt_ptr_t in eax per cdecl; but we'll accept on stack
	# We'll load from [esp+4]
	mov 4(%esp), %eax
	lgdt (%eax)

	# Update segment registers: reload CS via far jump, others via mov
	mov $0x10, %ax       # data segment selector (index 2 << 3)
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs
	mov %ax, %ss

	ljmp $0x08, $1f      # code segment selector (index 1 << 3)
1:
	ret


