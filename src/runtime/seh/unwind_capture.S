.text
.globl tl_RtlCaptureContext
.type tl_RtlCaptureContext, @function
tl_RtlCaptureContext:
    # Microsoft x64: RCX is the ContextAmd64* supplied by the guest.  This
    # function has no prologue, so [RSP] is the guest return address and the
    # saved RSP is exactly RSP+8.  Offsets are static-asserted in unwind.hpp.
    movq %r11, 208(%rcx)
    movq %rcx, %r11
    movq %rax, 120(%r11)
    movq %rcx, 128(%r11)
    movq %rdx, 136(%r11)
    movq %rbx, 144(%r11)
    leaq 8(%rsp), %rax
    movq %rax, 152(%r11)
    movq %rbp, 160(%r11)
    movq %rsi, 168(%r11)
    movq %rdi, 176(%r11)
    movq %r8, 184(%r11)
    movq %r9, 192(%r11)
    movq %r10, 200(%r11)
    movq %r12, 216(%r11)
    movq %r13, 224(%r11)
    movq %r14, 232(%r11)
    movq %r15, 240(%r11)
    movq (%rsp), %rax
    movq %rax, 248(%r11)
    movl $0x0010000b, 48(%r11)
    stmxcsr 52(%r11)
    pushfq
    popq %rax
    movl %eax, 68(%r11)
    movdqu %xmm0, 416(%r11)
    movdqu %xmm1, 432(%r11)
    movdqu %xmm2, 448(%r11)
    movdqu %xmm3, 464(%r11)
    movdqu %xmm4, 480(%r11)
    movdqu %xmm5, 496(%r11)
    movdqu %xmm6, 512(%r11)
    movdqu %xmm7, 528(%r11)
    movdqu %xmm8, 544(%r11)
    movdqu %xmm9, 560(%r11)
    movdqu %xmm10, 576(%r11)
    movdqu %xmm11, 592(%r11)
    movdqu %xmm12, 608(%r11)
    movdqu %xmm13, 624(%r11)
    movdqu %xmm14, 640(%r11)
    movdqu %xmm15, 656(%r11)
    # RDI and RSI are non-volatile in Microsoft x64 but volatile in System
    # V (the trace hook may clobber them), so park both in the stack around
    # the host call — no spare register survives a System V call here
    # (docs/arquitetura/abi-x64.md:33-40).  Alignment: entry RSP%16 == 8;
    # two pushes + subq $8 give the 16-aligned call site System V expects.
    pushq %rdi
    pushq %rsi
    subq $8, %rsp
    leaq .Ltl_RtlCaptureContext_name(%rip), %rdi
    callq trace_assembly_function_entry
    addq $8, %rsp
    popq %rsi
    popq %rdi
    retq
.size tl_RtlCaptureContext, .-tl_RtlCaptureContext

.section .rodata
.Ltl_RtlCaptureContext_name:
    .asciz "tl_RtlCaptureContext"

.section .note.GNU-stack,"",@progbits
