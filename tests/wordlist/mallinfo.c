/* LD_PRELOAD shim: prints glibc's mallinfo2() when the process exits (see frag.sh) */
#include <malloc.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
__attribute__((destructor)) static void
report(void) {
  struct mallinfo2 m = mallinfo2();
  char b[256];
  int n = snprintf(b, sizeof b, "MALLINFO heap=%zu in_use=%zu free=%zu (%.1f%%) free_chunks=%zu fastbin_chunks=%zu fastbin_bytes=%zu\n", m.arena, m.uordblks, m.fordblks,
                   m.arena ? 100.0 * m.fordblks / m.arena : 0.0, m.ordblks, m.smblks, m.fsmblks);
  write(2, b, n);
}
