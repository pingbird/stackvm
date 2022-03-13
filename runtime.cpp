#include <cstdio>
#include <cstdlib>

extern "C" {
  char *code(void *context, char *mem);
  void bf_putchar(void *context, int c);
  int bf_getchar(void *context);
}

void bf_putchar(void *context, int c) {
  putchar(c);
}

int bf_getchar(void *context) {
  int c = getchar();
  if (c == -1) return 0;
  return c;
}

int main(int argc, char **argv) {
  auto mem = static_cast<char*>(calloc(4096, 1));
  code(nullptr, mem);
  free(mem);
}