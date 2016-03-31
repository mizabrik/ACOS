.set SYS_write, 1
.set SYS_open, 2
.set SYS_close, 3
.set SYS_lseek, 8
.set SYS_exit, 60
.set SYS_chdir, 80
.set SYS_getdents64, 217

.set EXIT_SUCCESS, 0

.set d_off, 8
.set d_size, 16
.set d_type, 18
.set d_name, 19
.set DT_DIR, 4

.set SEEK_SET, 0
.set STDOUT, 1

.section .data
	.string "tree.S"
current_dir:
	.string "."
parent_dir:
	.string ".."
newline:
	.ascii "\n"
spaces:
	.space 256 , ' ' // ' ' ascii code
.set spaces_size, . - spaces

.section .bss
buffer:
	.space 1024
.set buffer_size, . - buffer

filename:
	.space 256

.section .text
.global _start
_start:
	xor %rbx, %rbx
	call tree
	
	movq $SYS_exit, %rax
	movq $EXIT_SUCCESS, %rdi
	syscall

tree:
	movq $SYS_open, %rax
	movq $current_dir, %rdi
	movq $0, %rsi
	syscall
	push %rax

read_dents:
	// Initialize dirents buffer, r14 --- size of loaded dents,
	// r15 --- size of processed dents
	xor %r15, %r15
	movq $SYS_getdents64, %rax
	movq (%rsp), %rdi
	movq $buffer, %rsi
	movq $buffer_size, %rdx
	syscall
	cmpq $0, %rax
	jle tree_finish
	movq %rax, %r14

	movq $buffer, %r12
process_dent:
	movw d_size(%r12), %r13w
	cmpb $'.', d_name(%r12)
	je process_dent_finish
	leaq d_name(%r12), %rsi
	call puts

	// Recursive call if we meet a directory
	cmpb $DT_DIR, d_type(%r12)
	jne process_dent_finish
	movq $SYS_chdir, %rax
	leaq d_name(%r12), %rdi
	syscall
	cmpq $0, %rax
	jne process_dent_finish
	push d_off(%r12)

	inc %rbx
	call tree
	dec %rbx

	movq $SYS_chdir, %rax
	movq $parent_dir, %rdi
	syscall
	movq $SYS_lseek, %rax
	pop %rsi
	movq (%rsp), %rdi
	movq $SEEK_SET, %rdx
	syscall
	jmp read_dents

process_dent_finish:
	addq %r13, %r15
	cmpq %r14, %r15
	je read_dents
	addq %r13, %r12
	jmp process_dent
	
tree_finish:
	movq $SYS_close, %rax
	pop %rdi
	syscall
	ret

puts:
	push %r12
	xor %r12, %r12
puts_copy:
	movb (%rsi,%r12,1), %al
	movb %al, filename(,%r12,1)
	inc %r12
	cmpb $0, %al
	jne puts_copy
	movb $'\n', filename-1(,%r12,1)
	
	call indent
	movq $SYS_write, %rax
	mov $STDOUT, %rdi
	movq $filename, %rsi
	movq %r12, %rdx
	syscall
	pop %r12
	ret

indent:
	push %rbx
indent_big:
	cmpq $spaces_size, %rbx
	jl indent_rest
	movq $SYS_write, %rax
	movq $STDOUT, %rdi
	movq $spaces, %rsi
	movq $spaces_size, %rdx
	syscall
	subq $spaces_size, %rbx
	jmp indent_big

indent_rest:
	cmpq $0, %rbx
	je indent_end
	movq $SYS_write, %rax
	movq $STDOUT, %rdi
	movq $spaces, %rsi
	movq %rbx, %rdx
	syscall
indent_end:
	pop %rbx
	ret
