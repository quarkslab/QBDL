.text
.globl _dl_resolve_internal

.type _dl_resolve_internal, #function
.align 2

// See binutils/gold/aarch64.cc for the details
//
// ADRP  X16, #printf_ptr@PAGE
// LDR   X17, [X16,#printf_ptr@PAGEOFF]
// ADD   X16, X16, #printf_ptr@PAGEOFF
// BR    X17
//
// STP   X16, LR, [SP,#-0x10]!                    // Save the relocation and the return address on
//                                                // the stack
// ADRP  X16, #_dl_resolve_internal@PAGE
// LDR   X17, [X16,#_dl_resolve_internal@PAGEOFF] // Load the address of our resolver stub
// ADD   X16, X16, #_dl_resolve_internal@PAGEOFF
// BR    X17
// ===========================
_dl_resolve_internal:
  // .inst 0xd4200000 // breakpoint
  stp x0,  x1,  [sp, #-16]!;
  stp x2,  x3,  [sp, #-16]!;
  stp x4,  x5,  [sp, #-16]!;
  stp x6,  x7,  [sp, #-16]!;
  stp x8,  x9,  [sp, #-16]!;
  stp x10, x11, [sp, #-16]!;
  stp x12, x13, [sp, #-16]!;
  stp x14, x15, [sp, #-16]!;
  stp x16, x17, [sp, #-16]!;
  stp x18, x19, [sp, #-16]!;
  stp x20, x21, [sp, #-16]!;
  stp x22, x23, [sp, #-16]!;
  stp x24, x25, [sp, #-16]!;
  stp x26, x27, [sp, #-16]!;
  stp x28, x29, [sp, #-16]!;
  stp x30, x0,  [sp, #-16]!;

  // Loader parameters
  ldr x0, [ip0, #-8];         // Scratch variable
  ldr x1, [sp, 16 * (8 + 8)]; // Relocation address

  bl _ZN4QBDL7Loaders3ELF10dl_resolveEPvm // QBDL::Loaders::ELF::dl_resolve(void*, unsigned long)
  str x0, [sp, 16 * (8 + 8)];

  ldp x30, x0,  [sp], 16;
  ldp x28, x29, [sp], 16;
  ldp x26, x27, [sp], 16;
  ldp x24, x25, [sp], 16;
  ldp x22, x23, [sp], 16;
  ldp x20, x21, [sp], 16;
  ldp x18, x19, [sp], 16;
  ldp x16, x17, [sp], 16;
  ldp x14, x15, [sp], 16;
  ldp x12, x13, [sp], 16;
  ldp x10, x11, [sp], 16;
  ldp x8,  x9,  [sp], 16;
  ldp x6,  x7,  [sp], 16;
  ldp x4,  x5,  [sp], 16;
  ldp x2,  x3,  [sp], 16;
  ldp x0,  x1,  [sp], 16;

  ldp x16, lr, [sp], #16;
  br x16;

