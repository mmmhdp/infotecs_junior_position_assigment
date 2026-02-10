#include <stdio.h>
#include "mymem.h"

int main(void)
{
  printf("Testing custom malloc/free\n");

  void *a = my_malloc(15);
  void *b = my_malloc(15);
  void *c = my_malloc(180);
  void *d = my_malloc(180);

  if (!a || !b || !c || !d) {
    printf("Allocation failed\n");
    return 1;
  }

  printf("Allocated:\n");
  printf("  a (15)  = %p\n", a);
  printf("  b (15)  = %p\n", b);
  printf("  c (180) = %p\n", c);
  printf("  d (180) = %p\n", d);

  my_free(b);
  my_free(d);

  printf("Freed b and d\n");

  void *e = my_malloc(15);
  void *f = my_malloc(180);

  printf("Reallocated:\n");
  printf("  e (15)  = %p (should reuse b)\n", e);
  printf("  f (180) = %p (should reuse d)\n", f);

  my_free(a);
  my_free(c);
  my_free(e);
  my_free(f);

  printf("All memory freed successfully\n");

  // Uncomment to test invalid free detection
  my_free(a);

  printf("Test completed OK\n");
  return 0;
}
