#include "sed_internal.h"
#include "../../lib/alloc.h"
#include "../../lib/byte.h"
#include "../../lib/str.h"
#include "../../lib/scan.h"

#define SED_MAXNEST 256 /* '{' nesting depth */

struct sed_label {
  char* name;
  size_t len;
  size_t cmd; /* ':' command's index */
};

struct sed_branch_fixup {
  size_t cmd;   /* the b/t/T command's index */
  char* name;   /* target label text (owned), NULL = branch to end of script */
  size_t len;
};

struct sed_builder {
  struct sed_cmd* cmds;
  size_t n, cap;

  size_t brace_stack[SED_MAXNEST];
  size_t nbrace;

  struct sed_label* labels;
  size_t nlabel, labelcap;

  struct sed_branch_fixup* fixups;
  size_t nfixup, fixupcap;

  unsigned flags;
  int err;
  struct sed* prog; /* for wfile interning */
};

static struct sed_cmd*
push_cmd(struct sed_builder* b) {
  if(b->n == b->cap) {
    size_t ncap = b->cap ? b->cap * 2 : 32;
    struct sed_cmd* nc = alloc_re(b->cmds, ncap * sizeof(*nc));

    if(!nc) {
      b->err = SED_ENOMEM;
      return NULL;
    }

    b->cmds = nc;
    b->cap = ncap;
  }

  byte_zero(&b->cmds[b->n], sizeof(b->cmds[0]));
  return &b->cmds[b->n++];
}

static const char*
skip_blank(const char* p, const char* end) {
  while(p < end && (*p == ' ' || *p == '\t'))
    p++;
  return p;
}

/* skip blanks, newlines, ';' separators, and '#...\n' comments between
 * commands. POSIX only recognizes '#' as a comment at the start of a
 * line, not after ';'; this treats it as a comment wherever a command
 * is expected instead, which is what every real sed accepts in
 * practice and is simpler to parse.
 * ----------------------------------------------------------------------- */
static const char*
skip_sep(const char* p, const char* end) {
  for(;;) {
    p = skip_blank(p, end);

    if(p < end && (*p == '\n' || *p == ';')) {
      p++;
      continue;
    }

    if(p < end && *p == '#') {
      while(p < end && *p != '\n')
        p++;
      continue;
    }

    break;
  }

  return p;
}

static int
looks_like_addr(const char* p, const char* end) {
  if(p >= end)
    return 0;
  if(*p >= '0' && *p <= '9')
    return 1;
  if(*p == '$' || *p == '/')
    return 1;
  if(*p == '\\' && p + 1 < end)
    return 1;
  return 0;
}

static int
max_addr(unsigned char letter) {
  switch(letter) {
  case 'a': case 'i': case 'q': case 'r': case '=': return 1;
  case ':': case '#': return 0;
  default: return 2;
  }
}

static int
label_push(struct sed_builder* b, const char* name, size_t len, size_t cmd) {
  if(b->nlabel == b->labelcap) {
    size_t ncap = b->labelcap ? b->labelcap * 2 : 8;
    struct sed_label* nl = alloc_re(b->labels, ncap * sizeof(*nl));

    if(!nl)
      return SED_ENOMEM;

    b->labels = nl;
    b->labelcap = ncap;
  }

  b->labels[b->nlabel].name = alloc(len ? len : 1);

  if(!b->labels[b->nlabel].name)
    return SED_ENOMEM;

  byte_copy(b->labels[b->nlabel].name, len, name);
  b->labels[b->nlabel].len = len;
  b->labels[b->nlabel].cmd = cmd;
  b->nlabel++;
  return SED_OK;
}

static int
fixup_push(struct sed_builder* b, size_t cmd, const char* name, size_t len) {
  if(b->nfixup == b->fixupcap) {
    size_t ncap = b->fixupcap ? b->fixupcap * 2 : 8;
    struct sed_branch_fixup* nf = alloc_re(b->fixups, ncap * sizeof(*nf));

    if(!nf)
      return SED_ENOMEM;

    b->fixups = nf;
    b->fixupcap = ncap;
  }

  if(len) {
    b->fixups[b->nfixup].name = alloc(len);

    if(!b->fixups[b->nfixup].name)
      return SED_ENOMEM;

    byte_copy(b->fixups[b->nfixup].name, len, name);
  } else {
    b->fixups[b->nfixup].name = NULL;
  }

  b->fixups[b->nfixup].len = len;
  b->fixups[b->nfixup].cmd = cmd;
  b->nfixup++;
  return SED_OK;
}

/* parse_label_word: a label or b/t/T target reads to ';', blank, or
 * newline (GNU-compatible; POSIX itself only says "to the end of the
 * line" but the common ":a;N;$!ba" idiom relies on ';' terminating,
 * and a label containing ';' is not valid POSIX use anyway). */
static const char*
parse_label_word(const char* p, const char* end, const char** name, size_t* len) {
  const char* start = p;

  while(p < end && *p != '\n' && *p != ';' && *p != ' ' && *p != '\t')
    p++;

  *name = start;
  *len = (size_t)(p - start);
  return p;
}

static int
parse_command(struct sed_builder* b, const char** pp, const char* end) {
  const char* p = *pp;
  struct sed_cmd* c;
  int naddr = 0;
  struct sed_addr a1, a2;
  int negate = 0;
  int rc;

  byte_zero(&a1, sizeof(a1));
  byte_zero(&a2, sizeof(a2));

  if(looks_like_addr(p, end)) {
    if((rc = sed_addr_parse(&p, end, &a1, b->flags)) != SED_OK)
      return rc;

    naddr = 1;
    p = skip_blank(p, end);

    if(p < end && *p == ',') {
      p++;
      p = skip_blank(p, end);

      if((rc = sed_addr_parse(&p, end, &a2, b->flags)) != SED_OK)
        return rc;

      naddr = 2;
    }
  }

  p = skip_blank(p, end);

  while(p < end && *p == '!') {
    negate = !negate;
    p++;
    p = skip_blank(p, end);
  }

  if(p >= end)
    return SED_ECMD;

  if(naddr > max_addr((unsigned char)*p))
    return SED_ECMD;

  c = push_cmd(b);

  if(!c)
    return SED_ENOMEM;

  c->naddr = (unsigned char)naddr;
  c->negate = (unsigned char)negate;
  c->a1 = a1;
  c->a2 = a2;
  c->letter = (unsigned char)*p;

  switch(c->letter) {
  case '{': {
    if(b->nbrace == SED_MAXNEST)
      return SED_ESIZE;

    b->brace_stack[b->nbrace++] = b->n - 1;
    p++;
    break;
  }

  case '}': {
    if(naddr || negate || b->nbrace == 0)
      return SED_EBRACE;

    b->nbrace--;
    b->n--; /* '}' is not a real command: it only closes the matching '{' */
    b->cmds[b->brace_stack[b->nbrace]].jump = b->n;
    p++;
    break;
  }

  case ':': {
    const char* name;
    size_t len;

    if(naddr)
      return SED_ECMD;

    p = skip_blank(p + 1, end);
    p = parse_label_word(p, end, &name, &len);

    if(len == 0)
      return SED_ELABEL;

    if((rc = label_push(b, name, len, b->n - 1)) != SED_OK)
      return rc;

    break;
  }

  case 'b':
  case 't': {
    const char* name;
    size_t len;

    p = skip_blank(p + 1, end);
    p = parse_label_word(p, end, &name, &len);

    if((rc = fixup_push(b, b->n - 1, name, len)) != SED_OK)
      return rc;

    break;
  }

  case '=':
  case 'd':
  case 'D':
  case 'g':
  case 'G':
  case 'h':
  case 'H':
  case 'n':
  case 'N':
  case 'p':
  case 'P':
  case 'x': {
    p++;
    break;
  }

  case 'q': {
    unsigned long v = 0;

    p = skip_blank(p + 1, end);

    if(p < end && *p >= '0' && *p <= '9') {
      size_t used = scan_ulong(p, &v);

      p += used;
    }

    c->u.qstatus = (int)v;
    break;
  }

  case 'a':
  case 'i':
  case 'c': {
    p++;

    if((rc = sed_text_parse(&p, end, &c->u.text)) != SED_OK)
      return rc;

    break;
  }

  case 'r':
  case 'w': {
    const char* name;

    p = skip_blank(p + 1, end);
    name = p;

    while(p < end && *p != '\n')
      p++;

    if(p == name)
      return SED_EUNTERM;

    if(c->letter == 'r') {
      c->u.rfile = alloc((size_t)(p - name) + 1);

      if(!c->u.rfile)
        return SED_ENOMEM;

      byte_copy(c->u.rfile, (size_t)(p - name), name);
      c->u.rfile[p - name] = 0;
    } else {
      int idx = sed_wfile_intern(b->prog, name, (size_t)(p - name));

      if(idx < 0)
        return SED_ENOMEM;

      c->u.wfile = idx;
    }

    break;
  }

  case 's': {
    p++;

    if((rc = sed_subst_parse(&p, end, &c->u.s, b->flags, b->prog)) != SED_OK)
      return rc;

    break;
  }

  case 'y': {
    p++;

    if((rc = sed_y_parse(&p, end, c->u.y)) != SED_OK)
      return rc;

    break;
  }

  case 'l': {
    p = skip_blank(p + 1, end);

    if(p < end && *p >= '0' && *p <= '9')
      return SED_ECMD; /* GNU's line-wrap-width argument: not supported */

    break;
  }

  default: return SED_ECMD;
  }

  *pp = p;
  return SED_OK;
}

int
sed_parse(struct sed* prog, const char* script, size_t len, unsigned flags) {
  struct sed_builder b;
  const char* p = script;
  const char* end = script + len;
  int rc = SED_OK;
  size_t i;

  byte_zero(&b, sizeof(b));
  b.flags = flags;
  b.prog = prog;

  if(flags & SED_NOAUTOPRINT)
    prog->autoprint_off = 1;

  if(len >= 2 && p[0] == '#' && p[1] == 'n' && (len == 2 || p[2] == '\n')) {
    prog->autoprint_off = 1;
    p += (len == 2) ? 2 : 3;
  }

  while((p = skip_sep(p, end)) < end) {
    if((rc = parse_command(&b, &p, end)) != SED_OK)
      goto done;
  }

  if(b.nbrace) {
    rc = SED_EBRACE;
    goto done;
  }

  for(i = 0; i < b.nfixup; i++) {
    struct sed_branch_fixup* fx = &b.fixups[i];
    struct sed_cmd* c = &b.cmds[fx->cmd];

    if(fx->len == 0) {
      c->jump = b.n;
    } else {
      size_t j;
      size_t found = (size_t)-1;

      for(j = 0; j < b.nlabel; j++) {
        if(b.labels[j].len == fx->len && byte_diff(b.labels[j].name, fx->len, fx->name) == 0) {
          found = b.labels[j].cmd;
          break;
        }
      }

      if(found == (size_t)-1) {
        rc = SED_ELABEL;
        goto done;
      }

      c->jump = found;
    }
  }

done:
  for(i = 0; i < b.nlabel; i++)
    alloc_free(b.labels[i].name);
  alloc_free(b.labels);

  for(i = 0; i < b.nfixup; i++)
    alloc_free(b.fixups[i].name);
  alloc_free(b.fixups);

  if(rc != SED_OK) {
    for(i = 0; i < b.n; i++) {
      struct sed_cmd* c = &b.cmds[i];

      sed_addr_free(&c->a1);
      sed_addr_free(&c->a2);

      switch(c->letter) {
      case 's': sed_subst_free(&c->u.s); break;
      case 'a': case 'i': case 'c': alloc_free(c->u.text.s); break;
      case 'r': alloc_free(c->u.rfile); break;
      default: break;
      }
    }

    alloc_free(b.cmds);
    return rc;
  }

  prog->cmds = b.cmds;
  prog->ncmd = b.n;
  return SED_OK;
}
