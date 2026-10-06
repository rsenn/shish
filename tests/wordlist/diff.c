/* Differential test of src/wordlist against the node-chain expand_cat()/expand_glob() it replaces.
 * Not run by ctest; delete together with expand_cat() once the shell is ported.
 *
 *   gcc -g -w -iquote src -iquote lib -iquote . -iquote $B -iquote $B/src -DHAVE_CONFIG_H -DHAVE_GLOB \
 *     -DHAVE_GLOB_H -c tests/wordlist/diff.c -o diff.o
 *   link like CMakeFiles/shish.dir/link.txt, with diff.o and a stub defining sh_argc, sh_argv,
 *   sh_name, sh_interactive in place of sh_main.c.o
 *   mkdir gd; touch gd/fa gd/fb gd/f.c; cd gd; ../diff 300000 SEED    # prints "N iterations, 0 differ"
 *
 * Random words of 1-5 chunks (text x X_* flags x IFS), expanded both ways: field lists, string mode,
 * three words in a row, and a list that spills past WORDLIST_INLINE fields.
 * ----------------------------------------------------------------------- */
#include "expand.h"
#include "wordlist.h"
#include "tree.h"
#include "var.h"
#include "sh.h"
#include "parse.h"
#include "../lib/str.h"
#include "../lib/byte.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct chunk { const char* t; unsigned f; };

static void
old_word(struct chunk* c, int n, char** out, int* no) {
  union node *head = NULL, *tail = NULL, *x;
  int i;
  *no = 0;
  for(i = 0; i < n; i++) {
    union node* t = expand_cat(c[i].t, strlen(c[i].t), tail ? &tail : &head, c[i].f);
    if(t) tail = t;
  }
  if((x = tail)) {
    union node** np = &tail;
    if(x->narg.flag & X_GLOB) {
      union node* g = expand_glob(np, x->narg.flag & ~X_GLOB);
      if(g) x = g;
    } else if(x->narg.flag & X_GLOBRES) {
      union node* g = expand_glob(np, x->narg.flag);
      if(g) x = g;
      stralloc_nul(&x->narg.stra);
    } else if((x->narg.flag & X_LITERAL) && !(x->narg.flag & X_UNESCAPED)) {
      expand_unescape(&x->narg.stra, parse_isesc);
    } else
      stralloc_nul(&x->narg.stra);
  }
  for(x = head; x; x = x->next)
    if(x->narg.stra.s) stralloc_nul(&x->narg.stra);
  for(x = head; x; x = x->next) {
    if(!x->narg.stra.s) continue;
    if(x->narg.stra.len == 0 && !(x->narg.flag & (X_QUOTED | X_NOSPLIT | X_SPLIT))) continue;
    out[(*no)++] = strdup(x->narg.stra.s);
  }
}

static void
new_word(struct chunk* c, int n, const char* ifs, char** out, int* no) {
  arena ar; wordlist wl; int i, argc; char** v;
  arena_init(&ar, &arena_heap, 0);
  wordlist_init(&wl, &ar, ifs);
  for(i = 0; i < n; i++) wordlist_cat(&wl, c[i].t, strlen(c[i].t), c[i].f);
  wordlist_close(&wl);
  v = wordlist_argv(&wl, &argc);
  *no = argc;
  for(i = 0; i < argc; i++) out[i] = strdup(v[i]);
  wordlist_free(&wl);
  arena_free(&ar);
}

static const char* texts[] = {"a", "b", " ", "  ", "\t", ":", "::", "a b", " a  b ", ":a:", "a\\ b", "\\\\", "", "x", "f*", "\\*", "f?", ",", " : "};
static const unsigned flagsets[] = {0, X_LITERAL, X_LITERAL | X_GLOB, X_QUOTED, X_QUOTED | X_LITERAL, X_NOSPLIT, X_GLOBRES, X_SUBWORD | X_LITERAL,
  X_PATTERN | X_LITERAL, X_PATTERN | X_QUOTED, X_QUOTED | X_GLOBRES, X_NOSPLIT | X_GLOBRES, X_LITERAL | X_QUOTED | X_GLOB, X_SUBWORD};
static const char* ifss[] = {" \t\n", ":", " :", "", ":,", ",", "\n"};

static void
multi_check(long iters) {
  long it, bad = 0;
  for(it = 0; it < iters; it++) {
    struct chunk c[3][4]; int nn[3], w, i, j, tot = 0, ntot = 0, k, no;
    char* exp[256]; char* o[64]; const char* ifs = ifss[rand() % 7];
    arena ar; wordlist wl; int argc; char** v;
    var_setv("IFS", ifs, strlen(ifs), 0);
    arena_init(&ar, &arena_heap, 0);
    wordlist_init(&wl, &ar, ifs);
    for(w = 0; w < 3; w++) {
      nn[w] = 1 + rand() % 3;
      for(i = 0; i < nn[w]; i++) { c[w][i].t = texts[rand() % 19]; c[w][i].f = flagsets[rand() % 14]; }
      old_word(c[w], nn[w], o, &no);
      for(k = 0; k < no; k++) exp[tot++] = o[k];
      for(i = 0; i < nn[w]; i++) wordlist_cat(&wl, c[w][i].t, strlen(c[w][i].t), c[w][i].f);
      wordlist_close(&wl);
    }
    v = wordlist_argv(&wl, &argc);
    if(argc != tot) bad++; else for(j = 0; j < tot; j++) if(strcmp(v[j], exp[j])) bad++;
    if(v[argc] != NULL) bad++;
    (void)ntot;
    wordlist_free(&wl); arena_free(&ar);
  }
  printf("multi: %ld iterations, %ld differ\n", iters, bad);
}

static void
str_check(long iters) {
  long it, bad = 0;
  for(it = 0; it < iters; it++) {
    struct chunk c[5]; int n = 1 + rand() % 5, i;
    union node *head = NULL, *tail = NULL; wordlist wl; stralloc sa;
    for(i = 0; i < n; i++) { c[i].t = texts[rand() % 19]; c[i].f = flagsets[rand() % 14]; }
    for(i = 0; i < n; i++) { union node* t = expand_cat(c[i].t, strlen(c[i].t), tail ? &tail : &head, c[i].f | X_NOSPLIT); if(t) tail = t; }
    stralloc_init(&sa);
    wordlist_init_str(&wl, &sa);
    for(i = 0; i < n; i++) wordlist_cat(&wl, c[i].t, strlen(c[i].t), c[i].f);
    wordlist_close(&wl);
    stralloc_nul(&sa);
    if(head) { stralloc_nul(&head->narg.stra); if(strcmp(head->narg.stra.s, sa.s)) bad++; } else if(sa.len) bad++;
    stralloc_free(&sa);
  }
  printf("string mode: %ld iterations, %ld differ\n", iters, bad);
}

static void
spill_check(void) {
  arena ar; wordlist wl; int i, argc, bad = 0; char** v;
  char big[1000]; size_t o = 0;
  for(i = 0; i < 100; i++) { big[o++] = 'w'; big[o++] = '0' + i / 10; big[o++] = '0' + i % 10; big[o++] = ' '; }
  arena_init(&ar, &arena_heap, 0);
  wordlist_init(&wl, &ar, " ");
  wordlist_cat(&wl, big, o, 0);
  wordlist_close(&wl);
  v = wordlist_argv(&wl, &argc);
  if(argc != 100) bad++;
  for(i = 0; i < argc && i < 100; i++) if(v[i][0] != 'w' || v[i][1] != '0' + i / 10 || v[i][3]) bad++;
  if(v[argc]) bad++;
  wordlist_free(&wl); arena_free(&ar);
  printf("spill: %s\n", bad ? "FAIL" : "ok");
}

int
main(int argc, char** argv) {
  long iters = argc > 1 ? atol(argv[1]) : 100000, bad = 0, it;
  srand(argc > 2 ? atoi(argv[2]) : 1);
  for(it = 0; it < iters; it++) {
    struct chunk c[6]; int n = 1 + rand() % 5, i, j, no1, no2, diff = 0;
    const char* ifs = ifss[rand() % 7];
    char* o1[64]; char* o2[64];
    for(i = 0; i < n; i++) { c[i].t = texts[rand() % 19]; c[i].f = flagsets[rand() % 14]; }
    var_setv("IFS", ifs, strlen(ifs), 0);
    old_word(c, n, o1, &no1);
    new_word(c, n, ifs, o2, &no2);
    if(no1 != no2) diff = 1; else for(j = 0; j < no1; j++) if(strcmp(o1[j], o2[j])) diff = 1;
    if(diff) {
      if(bad++ < 15) {
        printf("DIFF ifs=[%s]\n", ifs);
        for(i = 0; i < n; i++) printf("  chunk [%s] f=%08x\n", c[i].t, c[i].f);
        printf("  old:"); for(j = 0; j < no1; j++) printf(" <%s>", o1[j]); printf("\n  new:"); for(j = 0; j < no2; j++) printf(" <%s>", o2[j]); printf("\n");
      }
    }
    for(j = 0; j < no1; j++) free(o1[j]);
    for(j = 0; j < no2; j++) free(o2[j]);
  }
  multi_check(iters / 4); str_check(iters / 4); spill_check();
  printf("%ld iterations, %ld differ\n", iters, bad);
  return bad != 0;
}
