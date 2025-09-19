global _start
_start:
    mov rax, 60 ;rax is A and 60 is the exit syscall(NR)
    mov rdi, 69 ;rdi is first arg and 69 is the exit code we want
    syscall
