.intel_syntax noprefix

.text

.globl _dl_resolve_internal

_dl_resolve_internal:
    push rbp;
    mov rbp, rsp;
    sub rsp, 1024;
    and rsp, -1024;
    fxsave [rsp];
    vextractf128 [rsp+512], ymm0, 1;
    vextractf128 [rsp+528], ymm1, 1;
    vextractf128 [rsp+544], ymm2, 1;
    vextractf128 [rsp+560], ymm3, 1;
    vextractf128 [rsp+576], ymm4, 1;
    vextractf128 [rsp+592], ymm5, 1;
    vextractf128 [rsp+608], ymm6, 1;
    vextractf128 [rsp+624], ymm7, 1;
    vextractf128 [rsp+640], ymm8, 1;
    vextractf128 [rsp+656], ymm9, 1;
    vextractf128 [rsp+672], ymm10, 1;
    vextractf128 [rsp+688], ymm11, 1;
    vextractf128 [rsp+704], ymm12, 1;
    vextractf128 [rsp+720], ymm13, 1;
    vextractf128 [rsp+736], ymm14, 1;
    vextractf128 [rsp+752], ymm15, 1;
    push r15;
    push r14;
    push r13;
    push r12;
    push r11;
    push r10;
    push r9;
    push r8;
    push rdi;
    push rsi;
    push rdx;
    push rcx;
    push rbx;
    push rax;
    mov rdi, [rbp + 8];
    mov rsi, [rbp + 16];
    call _ZN4QBDL7Loaders3ELF10dl_resolveEPvm
    mov [rbp + 16], rax;
    pop rax;
    pop rbx;
    pop rcx;
    pop rdx;
    pop rsi;
    pop rdi;
    pop r8;
    pop r9;
    pop r10;
    pop r11;
    pop r12;
    pop r13;
    pop r14;
    pop r15;
    fxrstor [rsp];
    vinsertf128 ymm0, ymm0, [rsp+512], 1;
    vinsertf128 ymm1, ymm1, [rsp+528], 1;
    vinsertf128 ymm2, ymm2, [rsp+544], 1;
    vinsertf128 ymm3, ymm3, [rsp+560], 1;
    vinsertf128 ymm4, ymm4, [rsp+576], 1;
    vinsertf128 ymm5, ymm5, [rsp+592], 1;
    vinsertf128 ymm6, ymm6, [rsp+608], 1;
    vinsertf128 ymm7, ymm7, [rsp+624], 1;
    vinsertf128 ymm8, ymm8, [rsp+640], 1;
    vinsertf128 ymm9, ymm9, [rsp+656], 1;
    vinsertf128 ymm10, ymm10, [rsp+672], 1;
    vinsertf128 ymm11, ymm11, [rsp+688], 1;
    vinsertf128 ymm12, ymm12, [rsp+704], 1;
    vinsertf128 ymm13, ymm13, [rsp+720], 1;
    vinsertf128 ymm14, ymm14, [rsp+736], 1;
    vinsertf128 ymm15, ymm15, [rsp+752], 1;
    mov rsp, rbp;
    mov r11, [rbp + 16];
    pop rbp;
    add rsp, 16;
    jmp r11;
