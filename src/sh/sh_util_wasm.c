#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <emscripten.h>
#include <fcntl.h>
#include <setjmp.h>
#include <stdlib.h>
#include <unistd.h>

#include "../fd.h"
#include "../fdtable.h"
#include "../sh.h"
#include "../source.h"
#include "../parse.h"
#include "../debug.h"
#include "../tree.h"
#include "../../lib/buffer.h"
#include "../../lib/byte.h"
#include "../../lib/open.h"
#include "../../lib/str.h"

extern const char* tree_separator;

int sh_argc;
char** sh_argv;
const char* sh_name = "shutil";
int sh_no_position = 1;
int sh_interactive = 0;

/* parse_exit_hook target: a syntax error must unwind back to the
 * current shutil_format()/shutil_parse_ast() call, not exit(1) the
 * whole WASM runtime (the plain-exit(1) fallback parse_error() would
 * otherwise take) or pull in sh_exit()'s interpreter/trap machinery
 * (see fixes/219). Matches sh_exit()'s own "never returns" contract. */
static jmp_buf wasm_syntax_error;

static void
wasm_exit_hook(int code) {
  longjmp(wasm_syntax_error, code ? code : -1);
}

/* one-time fd-table setup shared by both entry points below. Uses
 * fd_malloc() (heap), not the stack-allocated fd_allocb(), because
 * these fds must outlive this function across many exported calls. */
static void
wasm_init(void) {
  static int ready = 0;
  int e, v;
  struct fd* fd;

  if(ready)
    return;
  ready = 1;

  parse_exit_hook = wasm_exit_hook;

  fd_expected = STDERR_FILENO + 1;

  for(e = STDIN_FILENO; e <= STDERR_FILENO; e++) {
    int flags;

    if((flags = fdtable_check(e))) {
      fd = fd_mallocb();
      fd_push(fd, e, flags | FD_FREE);
      fd_setfd(fd, e);
    } else if(e < fd_expected)
      fd_expected = e;
  }

  fdtable_foreach(v) {
    fd_stat(fdtable[v]);
    fd_setbuf(fdtable[v], &fdtable[v][1], FD_BUFSIZE);
  }

  sh_argv0 = "shutil";
}

/* reads the whole of an already-open fd into a malloc'd, NUL-terminated
 * buffer -- the standard hand-back-to-JS convention (paired with
 * shutil_free()). */
static char*
read_all(int fd) {
  stralloc sa;
  char buf[4096];
  ssize_t n;
  char* result;

  stralloc_init(&sa);

  while((n = read(fd, buf, sizeof(buf))) > 0)
    stralloc_catb(&sa, buf, n);

  result = malloc(sa.len + 1);

  if(sa.len)
    byte_copy(result, sa.len, sa.s);

  result[sa.len] = 0;
  stralloc_free(&sa);
  return result;
}

EMSCRIPTEN_KEEPALIVE
void
shutil_free(char* p) {
  free(p);
}

/* formats shell source text, returning the reformatted script, or 0 on
 * a genuine syntax error (the diagnostic goes to stderr, which the JS
 * wrapper captures via Module.printErr). */
EMSCRIPTEN_KEEPALIVE
char*
shutil_format(const char* src) {
  struct fd fd;
  struct source srcbuf;
  struct parser p;
  stralloc cmd, out;
  enum tok_flag tok;
  size_t len = str_len(src);
  char* result;

  wasm_init();

  tree_separator = "  ";

  fd_push(&fd, STDSRC_FILENO, FD_READ);
  source_push(&srcbuf);
  fd_string(&fd, src, len);

  /* a syntax error longjmps here via wasm_exit_hook (parse_error()
   * never returns in that case) instead of exiting the WASM runtime
   * or pulling in sh_exit()'s interpreter machinery. */
  if(setjmp(wasm_syntax_error)) {
    source_popfd(&fd);
    return 0;
  }

  stralloc_init(&cmd);
  stralloc_init(&out);
  parse_init(&p, P_COMMENT);

  while(!(((tok = parse_gettok(&p, P_DEFAULT)) & T_EOF))) {
    union node* list;
    stralloc* sa = tok == -2 ? &p.sa : &cmd;

    if(tok >= 0) {
      p.pushback++;
      parse_lineno = source->position.line;

      list = parse_list(&p);
      stralloc_zero(&cmd);

      if(list)
        tree_cat(list, &cmd);

      if(!(p.tok & (T_NL | T_SEMI | T_BGND)) && p.tok != T_EOF)
        parse_error(&p, 0);

      if(p.tok & (T_NL | T_SEMI | T_BGND))
        p.pushback = 0;
    }

    stralloc_cat(&out, sa);
    stralloc_catc(&out, '\n');
  }

  source_popfd(&fd);
  stralloc_free(&cmd);

  result = malloc(out.len + 1);

  if(out.len)
    byte_copy(result, out.len, out.s);

  result[out.len] = 0;
  stralloc_free(&out);
  return result;
}

/* parses shell source text into the debug/JSON AST (see debug_node.c),
 * returning the JSON text, or 0 on a genuine syntax error. debug_list()
 * writes through the fd-bound debug_buffer, so it's pointed at a MEMFS
 * temp file for the duration of the call and read back afterwards. */
EMSCRIPTEN_KEEPALIVE
char*
shutil_parse_ast(const char* src) {
  struct fd fd;
  struct source srcbuf;
  struct parser p;
  enum tok_flag tok;
  size_t len = str_len(src);
  union node *script = 0, **nptr = &script;
  int out_fd;
  char* result;

  wasm_init();

  fd_push(&fd, STDSRC_FILENO, FD_READ);
  source_push(&srcbuf);
  fd_string(&fd, src, len);

  /* see the matching comment in shutil_format() above. */
  if(setjmp(wasm_syntax_error)) {
    source_popfd(&fd);
    return 0;
  }

  parse_init(&p, P_DEFAULT);

  for(;;) {
    union node* list;

    p.pushback = 0;
    tok = parse_gettok(&p, P_DEFAULT);

    if(tok & T_EOF)
      break;

    p.pushback++;
    parse_lineno = source->position.line;

    list = parse_list(&p);
    nptr = tree_append(nptr, list);

    if(!(p.tok & (T_NL | T_SEMI | T_BGND)) && p.tok != T_EOF)
      parse_error(&p, 0);
  }

  unlink("/tmp/.shutil-ast.json");
  out_fd = open_trunc("/tmp/.shutil-ast.json");
  debug_buffer.fd = out_fd;

  if(script) {
    debug_list(script, 1);
    debug_nl_fl();
  }

  buffer_flush(&debug_buffer);
  close(out_fd);

  out_fd = open_read("/tmp/.shutil-ast.json");
  result = read_all(out_fd);
  close(out_fd);
  unlink("/tmp/.shutil-ast.json");

  source_popfd(&fd);
  return result;
}

/* never invoked (INVOKE_RUN=0, no callMain from the JS side) -- callers
 * use cwrap() on shutil_format/shutil_parse_ast directly. Emscripten
 * still wants a main to link against. */
int
main(void) {
  return 0;
}
