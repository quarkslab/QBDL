#ifdef __aarch64__
#include "darwin_aarch64_libc.h"
#include <cstdio>

int darwin_aarch64_printf(const char *format, ...) {
  va_list args;
  va_start(args, format);
  const int ret = vprintf(format, args);
  va_end(args);
  return ret;
}

int darwin_aarch64_puts(const char *format) { return puts(format); }
#endif
