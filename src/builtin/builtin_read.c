#include "builtin_config.h"

#if BUILTIN_READ

#include <unistd.h>
#include "../builtin.h"
#include "../fdtable.h"
#include "../expand.h"
#include "../var.h"
#include "../debug.h"
#include "../term.h"
#include "../../lib/byte.h"
#include "../../lib/scan.h"
#include "../../lib/str.h"
#include "../../lib/fmt.h"
#include "../../lib/buffer.h"
#include "../../lib/stralloc.h"

struct predicate_data {
  const char* delim;
  int ndelim;
  int nchars;
};

static int
predicate_function(stralloc* sa, void* ptr) {
  struct predicate_data* p = ptr;

  if(p->delim && p->ndelim > 0) {
    if(sa->len && byte_chr(p->delim, p->ndelim, sa->s[sa->len - 1]) < (size_t)p->ndelim)
      return 1;
  }

  if(p->nchars > 0)
    return sa->len >= (size_t)p->nchars;

  return 0;
}

/* one input character; without -r a backslash escapes the next one
 * ----------------------------------------------------------------------- */
static int
read_get(const char* s, size_t n, size_t* i, int raw, int* esc) {
  int c = (unsigned char)s[(*i)++];

  *esc = 0;

  if(!raw && c == '\\' && *i < n) {
    c = (unsigned char)s[(*i)++];
    *esc = 1;
  }

  return c;
}

/* is the character at s[*i] an unescaped IFS char (ws: whitespace one only)? */
static int
read_ifs(const char* s, size_t n, size_t i, int raw, const char* ifs, int ws) {
  int esc, c;

  if(i >= n)
    return 0;

  c = read_get(s, n, &i, raw, &esc);

  if(esc || !c || ifs[str_chr(ifs, c)] == 0)
    return 0;

  return ws ? (c == ' ' || c == '\t' || c == '\n') : 1;
}

static size_t
read_skipws(const char* s, size_t n, size_t i, int raw, const char* ifs) {
  int esc;

  while(read_ifs(s, n, i, raw, ifs, 1))
    read_get(s, n, &i, raw, &esc);

  return i;
}

/* splits the line into the variables per POSIX 2.6.5; returns 0 if an
 * assignment failed
 * ----------------------------------------------------------------------- */
static int
read_assign(stralloc* line, const char* ifs, int raw, char** vars, int nvars) {
  const char* s = line->s;
  size_t n = line->len, i = 0;
  int idx, ok = 1, esc;
  stralloc f;

  stralloc_init(&f);
  i = read_skipws(s, n, i, raw, ifs);

  for(idx = 0; idx < nvars; idx++) {
    int last = idx == nvars - 1;
    size_t start = i;

    f.len = 0;

    while(i < n && !read_ifs(s, n, i, raw, ifs, 0)) {
      int c = read_get(s, n, &i, raw, &esc);

      stralloc_catb(&f, (const char*)&c, 1);
    }

    /* the delimiter: ws* [nonws ws*] */
    if(i < n) {
      int c = read_ifs(s, n, i, raw, ifs, 1);

      read_get(s, n, &i, raw, &esc);
      i = read_skipws(s, n, i, raw, ifs);

      if(c && read_ifs(s, n, i, raw, ifs, 0)) {
        read_get(s, n, &i, raw, &esc);
        i = read_skipws(s, n, i, raw, ifs);
      }
    }

    /* the last variable takes the rest, minus trailing IFS whitespace */
    if(last && i < n) {
      size_t j = start, keep = 0;

      f.len = 0;

      while(j < n) {
        int ws = read_ifs(s, n, j, raw, ifs, 1);
        int c = read_get(s, n, &j, raw, &esc);

        stralloc_catb(&f, (const char*)&c, 1);

        if(!ws)
          keep = f.len;
      }

      f.len = keep;
    }

    stralloc_nul(&f);

    if(!var_setv(vars[idx], f.s, f.len, 0))
      ok = 0;
  }

  stralloc_free(&f);
  return ok;
}

/* read built-in
 *
 * ----------------------------------------------------------------------- */
const char help_read[] = "    Read a line and split it into variables.\n"
                         "\n"
                         "    -r              don't treat backslash as an escape character\n"
                         "    -s              don't echo input (e.g. for passwords)\n"
                         "    -d delim        read until delim instead of newline\n"
                         "    -n nchars       read at most nchars characters\n"
                         "    -N nchars       read exactly nchars characters, ignoring delim\n"
                         "    -p prompt       print prompt before reading (if input is a tty)\n"
                         "    -u fd           read from fd instead of stdin\n"
                         "    name            variable(s) to split the input into by $IFS\n";

int
builtin_read(int argc, char* argv[]) {
  int c, raw = 0, silent = 0, fd = 0, num_args, index;
  char** argp;
  const char* prompt = 0;
  stralloc data;
  struct predicate_data p = {"\n", 1, -1};

  while((c = shell_getopt(argc, argv, "d:n:N:p:rsu:")) > 0) {
    switch(c) {
      case 'd':
        p.delim = shell_optarg;
        p.ndelim = str_len(p.delim);
        break;
      case 'N': p.delim = 0; p.ndelim = 0;
      case 'n': scan_int(shell_optarg, &p.nchars); break;
      case 'p': prompt = shell_optarg; break;
      case 'r': raw = 1; break;
      case 's':
        silent = 1;
        break;
      case 'u': scan_int(shell_optarg, &fd); break;
      default: builtin_invopt(argv); return 1;
    }
  }

  argp = &argv[shell_optind];
  num_args = argc - shell_optind;

  for(index = 0; index < num_args; index++) {
    if(!var_valid(argp[index])) {
      builtin_errmsg(argv, argp[index], "not a valid identifier");
      return 1;
    }
  }

  if(prompt)
    buffer_putsflush(fd_out->w, prompt);

  {
    int ret, status = 0;
    const char* ifs;
    size_t len;
    char *ptr, *end;
    index = 0;
    struct termios attrs;
    buffer* input = fdtable[fd]->r;

    stralloc_init(&data);

    if(silent)
      term_attr(input->fd, 1, &attrs);

    if((ret = buffer_get_token_sa_pred(input, &data, predicate_function, &p)) > 0) {
      /* strip the delimiter that actually terminated the read. The
         default newline delimiter also trims a preceding '\r', to
         cope with CRLF line endings; a custom -d delimiter has no
         such convention, so only its own character(s) are trimmed */
      if(p.delim && p.ndelim == 1 && p.delim[0] == '\n')
        stralloc_trimr(&data, "\r\n", 2);
      else if(p.delim && p.ndelim > 0)
        stralloc_trimr(&data, p.delim, p.ndelim);
    } else {
      status = 1;
    }

    if(silent)
      term_restore(input->fd, &attrs);

    ifs = var_vdefault("IFS", IFS_DEFAULT, &len);

    /* backslash-newline continues the line (not with -r) */
    while(!raw && status == 0 && data.len && data.s[data.len - 1] == '\\') {
      size_t bs = 0;

      while(bs < data.len && data.s[data.len - 1 - bs] == '\\')
        bs++;

      if(bs % 2 == 0)
        break;

      data.len--;
      {
        stralloc more;

        stralloc_init(&more);

        if(buffer_get_token_sa_pred(input, &more, predicate_function, &p) > 0) {
          if(p.delim && p.ndelim == 1 && p.delim[0] == '\n')
            stralloc_trimr(&more, "\r\n", 2);
          else if(p.delim && p.ndelim > 0)
            stralloc_trimr(&more, p.delim, p.ndelim);
        } else
          status = 1;

        stralloc_cat(&data, &more);
        stralloc_free(&more);
      }
    }

    /* seekable input: hand the read-ahead back, so the next reader
       (this shell's parser, a child) continues right after the line.
       Only a buffer backed by a real descriptor: a here-document's buffer
       says fd 0 but is in memory, and fd 0 itself is then the script. */
    if(fdtable[fd]->e >= 0 && input->fd == fdtable[fd]->e && input->p < input->n && lseek(input->fd, -(off_t)(input->n - input->p), SEEK_CUR) != (off_t)-1)
      input->p = input->n = 0;

    if(!read_assign(&data, ifs, raw, argp, num_args))
      status = status ? status : 2;

    return status;
  }
}
#endif /* BUILTIN_READ */
