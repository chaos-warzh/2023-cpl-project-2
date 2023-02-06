#include "ramfs.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define Log() printf("in function %s, at line %d\n", __FUNCTION__, __LINE__)
#ifdef LOCAL
#include <assert.h>
#else
#define assert(cond)                                                           \
  do {                                                                         \
    if (cond)                                                                  \
      ;                                                                        \
    else {                                                                     \
      puts("false");                                                           \
      Log();                                                                   \
      exit(EXIT_SUCCESS);                                                      \
    }                                                                          \
  } while (0)
#endif
#define KB * 1024
#define MB KB * 1024
#define PGSIZE 1 KB
#define SCALE 1024

#define test(func, expect, ...) assert(func(__VA_ARGS__) == expect)
#define succopen(var, ...) assert((var = ropen(__VA_ARGS__)) >= 0)
#define failopen(var, ...) assert((var = ropen(__VA_ARGS__)) == -1)

void gen_random(char *pg) {
  int *p = (int *)pg;
  for (int i = 0; i < PGSIZE / 4; i++) {
    p[i] = rand();
  }
}

int notin(int fd, int *fds, int n) {
  for (int i = 0; i < n; i++) {
    if (fds[i] == fd)
      return 0;
  }
  return 1;
}

int genfd(int *fds, int n) {
  for (int i = 0; i < 4096; i++) {
    if (notin(i, fds, n))
      return i;
  }
  return -1;
}

int fd[SCALE];
uint8_t buf[1 MB];
uint8_t ref[1 MB];
int f;
int main() {
  srand(time(NULL));
  init_ramfs();

  test(rmkdir, 0, "/never");

  test(rrmdir, 0, "/never");
  test(rrmdir, -1, "/never/gonna");

  /* sixth round */
  test(rrmdir, -1, "/never");

  /* can't have subdir in file */
//  do {
//    if ((f = ropen("/never", 0100)) >= 0);
//    else {
//      puts("false");
//      printf("\t\tin function %s, at line %d\n", "_function_name_", 318);
//      printf("\n%d\n", f);
//      exit(0);
//    }
//  }
//  while (0);

  succopen(f, "/never", O_CREAT); // maybe o_create understand wrong?
  test(rclose, 0, f);
  test(rmkdir, -1, "/never/gonna");
  test(rmkdir, -1, "/never/gonna/give");
  test(rmkdir, -1, "/never/gonna/give/you");
  test(rrmdir, -1, "/never/gonna");
  test(rrmdir, -1, "/never/gonna/give");
  test(rrmdir, -1, "/never/gonna/give/you");

  puts("true");
}