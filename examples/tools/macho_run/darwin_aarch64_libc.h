#ifndef DARWIN_AARCH64_LIBC
#define DARWIN_AARCH64_LIBC

extern "C" {
__attribute__((darwin_abi)) int darwin_aarch64_printf(const char *format, ...);
__attribute__((darwin_abi)) int darwin_aarch64_puts(const char *s);
}

#endif
