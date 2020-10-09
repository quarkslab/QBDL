// gcc -O0 -g hello-world.c -o hello-world
#include <stdio.h>
#include <stdlib.h>

__attribute__((constructor)) void ctor() { printf("In ctor\n"); }

int main(int argc, char **argv) {

  for (int i = 0; i < argc; ++i) {
    printf("argv[%d] = %s", i, argv[i]);
  }
  return 0;
}

__attribute__((destructor)) void dtor() { printf("In dtor!\n"); }
