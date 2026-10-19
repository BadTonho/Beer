.text

.globl tl_cxx_catch_return_trampoline
.type tl_cxx_catch_return_trampoline, @function
tl_cxx_catch_return_trampoline:
    # O funclet devolve em RAX o destino do catchret. Preserve o destino e o
    # RSP convidado em registradores callee-saved enquanto obtemos a stack TLS
    # do host. O callback usa ContextAmd64 local grande e nunca pode executá-lo
    # sobre a stack do PE.
    movq %rax, %r13
    movq %rsp, %r12
    subq $128, %rsp
    callq tl_cxx_callback_stack_top
    movq %r13, %rdi
    movq %r12, %rsi
    movq %rax, %rsp
    # System V: no call site RSP%16 == 0. `andq` ja alinha em 0; um `subq $8`
    # adicional deixava a entrada do chamado com RSP%16 == 0 (esperado: 8),
    # violando docs/arquitetura/abi-x64.md:30-32 (auditoria 6.4).
    andq $-16, %rsp
    callq tl_cxx_catch_return_from_asm
    ud2
.size tl_cxx_catch_return_trampoline, .-tl_cxx_catch_return_trampoline

.globl tl_cxx_cleanup_return_trampoline
.type tl_cxx_cleanup_return_trampoline, @function
tl_cxx_cleanup_return_trampoline:
    # Um cleanupret normalizado pelo LLVM devolve com RET. Preserve o RSP
    # convidado enquanto obtemos a stack TLS do host; o callback restaura o
    # contexto original do frame para continuar o cleanup ou o catch.
    movq %rsp, %r12
    subq $128, %rsp
    callq tl_cxx_callback_stack_top
    movq %r12, %rdi
    movq %rax, %rsp
    # System V: no call site RSP%16 == 0 (auditoria 6.4); ver catchret.
    andq $-16, %rsp
    callq tl_cxx_cleanup_return_from_asm
    ud2
.size tl_cxx_cleanup_return_trampoline, .-tl_cxx_cleanup_return_trampoline

.section .note.GNU-stack,"",@progbits
