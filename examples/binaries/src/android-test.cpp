#include <android/log.h>
#include <cstdio>
#include <unistd.h>

int main(int argc, char **argv) {
  __android_log_print(ANDROID_LOG_INFO, "hello", "Hello World");
  printf("Hello from: %d\n", getpid());
  return 0;
}
