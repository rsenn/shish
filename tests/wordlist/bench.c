/* Per-command cost of expansion output: node chain (expand_cat) against wordlist.
 * Not run by ctest. Build and link like tests/wordlist/diff.c, adding
 * -Wl,--wrap=malloc,--wrap=calloc,--wrap=realloc,--wrap=free
 *
 *   ./bench old|new A|B|C N     A: `: a b c d e f g h`   B: one unquoted "a b ... h"   C: mixed words
 *
 * Allocations and live bytes come from the wrappers; instructions and data references from
 * valgrind --tool=cachegrind (N=40000 minus N=20000, divided by 20000), perf is not installed here.
 * ----------------------------------------------------------------------- */
#include "expand.h"
#include "wordlist.h"
#include "tree.h"
#include "var.h"
#include "parse.h"
#include "../lib/byte.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <time.h>
#include <sys/resource.h>

/* allocation accounting through --wrap */
static unsigned long n_alloc, b_alloc, live, peak, track = 0;
void* __real_malloc(size_t); void* __real_realloc(void*, size_t); void* __real_calloc(size_t, size_t); void __real_free(void*);
static void note(void* p) { if(track && p) { n_alloc++; live += malloc_usable_size(p); b_alloc += malloc_usable_size(p); if(live > peak) peak = live; } }
void* __wrap_malloc(size_t n) { void* p = __real_malloc(n); note(p); return p; }
void* __wrap_calloc(size_t a, size_t b) { void* p = __real_calloc(a, b); note(p); return p; }
void* __wrap_realloc(void* o, size_t n) { size_t os = o && track ? malloc_usable_size(o) : 0; void* p = __real_realloc(o, n); if(track && p) { n_alloc++; live += malloc_usable_size(p); live -= os; b_alloc += malloc_usable_size(p); if(live > peak) peak = live; } return p; }
void __wrap_free(void* p) { if(track && p) live -= malloc_usable_size(p); __real_free(p); }

struct chunk { const char* t; unsigned f; };
static struct chunk A[8] = {{"a",X_LITERAL},{"b",X_LITERAL},{"c",X_LITERAL},{"d",X_LITERAL},{"e",X_LITERAL},{"f",X_LITERAL},{"g",X_LITERAL},{"h",X_LITERAL}};
static struct chunk B[1] = {{"a b c d e f g h", 0}};
static struct chunk C[6] = {{"echo",X_LITERAL},{"hello world",X_QUOTED},{"/usr/local/bin/something",X_LITERAL},{"x y  z",0},{"--option=value",X_LITERAL},{"-",X_LITERAL}};
/* each chunk of C is its own word except the 4th which splits */

static void
old_cmd(struct chunk* c, int n) {
  union node *head = NULL, *tail = NULL;
  int i;
  for(i = 0; i < n; i++) {
    union node* t = expand_cat(c[i].t, strlen(c[i].t), tail ? &tail : &head, c[i].f);
    if(t) tail = t;
    if(tail) {
      if((tail->narg.flag & X_LITERAL) && !(tail->narg.flag & X_UNESCAPED)) expand_unescape(&tail->narg.stra, parse_isesc);
      stralloc_nul(&tail->narg.stra);
    }
    if(i + 1 < n) {
      if(!tail) continue;
      tail->next = tree_newnode(N_ARG);
      tail = tail->next;
      stralloc_init(&tail->narg.stra);
      stralloc_nul(&tail->narg.stra);
    }
  }
  {
    union node* x; char* argv[64]; int k = 0;
    for(x = head; x; x = x->next) if(x->narg.stra.len) argv[k++] = x->narg.stra.s;
    (void)argv;
  }
  tree_free(head);
}

static arena ar;

static void
new_cmd(struct chunk* c, int n) {
  wordlist wl; arena_pos pos = arena_tell(&ar); int i, argc;
  wordlist_init(&wl, &ar, " \t\n");
  for(i = 0; i < n; i++) { wordlist_cat(&wl, c[i].t, strlen(c[i].t), c[i].f); wordlist_close(&wl); }
  (void)wordlist_argv(&wl, &argc);
  wordlist_free(&wl);
  arena_rewind(&ar, pos);
}

int
main(int argc, char** argv) {
  int newm = !strcmp(argv[1], "new"); char w = argv[2][0]; long N = atol(argv[3]), i;
  struct chunk* c = w == 'A' ? A : w == 'B' ? B : C; int n = w == 'A' ? 8 : w == 'B' ? 1 : 6;
  struct timespec t0, t1; struct rusage ru;
  arena_init(&ar, &arena_heap, 8192);
  var_setv("IFS", " \t\n", 3, 0);
  /* warm up: the pool buffer and first arena chunk are allocated here, as in a running shell */
  for(i = 0; i < 100; i++) newm ? new_cmd(c, n) : old_cmd(c, n);
  track = 1;
  newm ? new_cmd(c, n) : old_cmd(c, n);
  track = 0;
  printf("%s %c: per command: %lu allocs, %lu bytes in blocks, peak live %lu bytes\n", argv[1], w, n_alloc, b_alloc, peak);
  clock_gettime(CLOCK_MONOTONIC, &t0);
  for(i = 0; i < N; i++) newm ? new_cmd(c, n) : old_cmd(c, n);
  clock_gettime(CLOCK_MONOTONIC, &t1);
  getrusage(RUSAGE_SELF, &ru);
  printf("   %.0f ns/command, maxrss %ld KiB\n", ((t1.tv_sec - t0.tv_sec) * 1e9 + (t1.tv_nsec - t0.tv_nsec)) / N, ru.ru_maxrss);
  return 0;
}
