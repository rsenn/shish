/* lib/arena unit test; prints "<what>: OK|FAIL", exits non-zero on any FAIL */
#include "../lib/arena.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int failed;

static void
check(int ok, const char* what) {
  printf("%s: %s\n", what, ok ? "OK" : "FAIL");
  failed |= !ok;
}

static int
all_zero(const char* p, size_t n) {
  while(n--)
    if(*p++)
      return 0;
  return 1;
}

static void
test_fixed(void) {
  char buf[256];
  arena a;
  char *p, *q;

  arena_init_fixed(&a, buf + 1, sizeof(buf) - 1);
  memset(buf, 0xff, sizeof(buf));
  arena_init_fixed(&a, buf + 1, sizeof(buf) - 1);

  p = arena_alloc(&a, 16, 8);
  check(p && ((uintptr_t)p & 7) == 0 && all_zero(p, 16), "fixed: aligned, zeroed alloc");
  check(arena_alloc(&a, 1000, 1) == NULL, "fixed: NULL once full, no growth");
  check(arena_alloc(&a, 0, 1) != NULL, "fixed: zero-size alloc still succeeds");

  arena_reset(&a);
  q = arena_alloc(&a, 16, 8);
  check(q == p, "fixed: reset reuses the buffer from the start");

  arena_init_fixed(&a, buf, 4);
  check(arena_alloc(&a, 1, 1) == NULL, "fixed: buffer smaller than header is an empty arena");
  memset(&a, 0, sizeof(a));
  check(arena_alloc(&a, 1, 1) == NULL, "zeroed arena: alloc fails");
}

static void
test_alloc(const struct arena_src* src, const char* name) {
  char msg[96];
  arena a;
  char *p, *q, *s;
  size_t n;
  arena_pos pos;

  arena_init(&a, src, 256);

  p = arena_alloc(&a, 100, 16);
  snprintf(msg, sizeof(msg), "%s: alloc 16-aligned, zeroed", name);
  check(p && ((uintptr_t)p & 15) == 0 && all_zero(p, 100), msg);

  snprintf(msg, sizeof(msg), "%s: allocn overflow gives NULL", name);
  check(arena_allocn(&a, SIZE_MAX / 2 + 1, 2, 1) == NULL, msg);

  s = arena_strndup(&a, "hello world", 5);
  snprintf(msg, sizeof(msg), "%s: strndup copies and terminates", name);
  check(s && !strcmp(s, "hello"), msg);

  q = arena_dup(&a, "abc", 3);
  snprintf(msg, sizeof(msg), "%s: dup copies", name);
  check(q && !memcmp(q, "abc", 3), msg);

  /* newest: grows in place */
  p = arena_dup(&a, "xy", 2);
  q = arena_grow(&a, p, 2, 20);
  snprintf(msg, sizeof(msg), "%s: grow of newest is in place, zeroed", name);
  check(q == p && !memcmp(q, "xy", 2) && all_zero(q + 2, 18), msg);

  /* not newest any more: refused, nothing changes */
  s = arena_dup(&a, "z", 1);
  n = arena_used(&a);
  snprintf(msg, sizeof(msg), "%s: grow of older object is refused, no hole", name);
  check(arena_grow(&a, p, 20, 40) == NULL && arena_used(&a) == n, msg);
  snprintf(msg, sizeof(msg), "%s: grow of NULL or wrong oldsize is refused", name);
  check(arena_grow(&a, NULL, 0, 8) == NULL && arena_grow(&a, s, 5, 8) == NULL, msg);

  /* trim gives slack back, next alloc lands right behind */
  q = arena_grow(&a, s, 1, 64);
  arena_trim(&a, q, 64, 8);
  p = arena_dup(&a, "w", 1);
  snprintf(msg, sizeof(msg), "%s: trim returns slack of newest", name);
  check(q == s && p == q + 8, msg);

  /* no room left in the chunk: refused, content kept */
  n = arena_used(&a);
  snprintf(msg, sizeof(msg), "%s: grow past the chunk is refused", name);
  check(arena_grow(&a, p, 1, 1 << 20) == NULL && arena_used(&a) == n && p[0] == 'w', msg);
  q = arena_alloc(&a, 1000, 1);
  snprintf(msg, sizeof(msg), "%s: allocation larger than a chunk gets its own", name);
  check(q && all_zero(q, 1000), msg);

  /* tell/rewind across chunks */
  pos = arena_tell(&a);
  n = arena_used(&a);
  for(int i = 0; i < 50; i++)
    if(!arena_alloc(&a, 100, 8))
      break;
  check(arena_used(&a) > n, "used grows with allocations");
  arena_rewind(&a, pos);
  snprintf(msg, sizeof(msg), "%s: rewind restores usage", name);
  check(arena_used(&a) == n, msg);
  snprintf(msg, sizeof(msg), "%s: data before the mark survives rewind", name);
  check(p[0] == 'w', msg);

  arena_reset(&a);
  snprintf(msg, sizeof(msg), "%s: reset empties the arena", name);
  check(arena_used(&a) == 0 && arena_alloc(&a, 8, 8), msg);

  /* rewind to a position taken while empty keeps the first chunk usable */
  arena_free(&a);
  pos = arena_tell(&a);
  arena_alloc(&a, 10, 1);
  arena_rewind(&a, pos);
  snprintf(msg, sizeof(msg), "%s: rewind to empty mark reuses first chunk", name);
  check(arena_used(&a) == 0 && arena_alloc(&a, 10, 1), msg);

  arena_free(&a);
  snprintf(msg, sizeof(msg), "%s: free leaves an empty arena", name);
  check(arena_used(&a) == 0, msg);
}

int
main(void) {
  test_fixed();
  test_alloc(&arena_heap, "heap");
  test_alloc(&arena_mmap, "mmap");
  test_alloc(&arena_brk, "brk");

  puts(failed ? "FAILED" : "all passed");
  return failed;
}
