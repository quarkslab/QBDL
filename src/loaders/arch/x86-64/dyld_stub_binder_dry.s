.intel_syntax noprefix

.text

.globl __dyld_stub_binder_dry_call

__dyld_stub_binder_dry_call:
    pop rax; // cache addresss (used as a scratch register)
    ret;
