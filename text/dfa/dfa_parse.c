#include "dfa_prog.h"
#include <ctype.h>

#define DFA_MAXALT 64

/* group_index: the group opened by the '(' at src gets a number the
 * first time it's seen; a later call with the same src (re-parsing
 * one atom's source span for {m,n} duplication) reuses it, so a
 * repeated group's captures always land in the same slot.
 * ----------------------------------------------------------------------- */
static size_t
group_index(struct dfa_parser* ps, const char* src) {
  size_t i;

  for(i = 0; i < ps->ngroup; i++)
    if(ps->gsrc[i] == src)
      return i;

  if(ps->ngroup >= DFA_MAXGROUPS) {
    ps->err = DFA_ESIZE;
    return 0;
  }

  ps->gsrc[ps->ngroup] = src;
  return ps->ngroup++;
}

/* insert_split: makes room for one SPLIT instruction at index `pos`
 * (everything at or after `pos` moves up by one) and sets it to
 * {SPLIT, pos+1, -1} (its "else" target is unresolved, -1 = not yet
 * known, patched by the caller). -1 as the "unknown" sentinel (not 0)
 * matters here: every already-emitted SPLIT/JMP anywhere in the
 * program, not just inside the shifted block, that targets pos or
 * later must move with it, and 0 could be confused with a real target
 * once pos reaches 0. Returns pos (the split's only possible index).
 * ----------------------------------------------------------------------- */
static int
insert_split(struct dfa_parser* ps, size_t pos) {
  int pc;
  size_t i;

  pc = dfa_emit(ps, DFA_SPLIT, 0, 0);

  if(ps->err)
    return -1;

  for(i = (size_t)pc; i > pos; i--)
    ps->prog[i] = ps->prog[i - 1];

  ps->prog[pos].op = DFA_SPLIT;
  ps->prog[pos].x = (int)pos + 1;
  ps->prog[pos].y = -1;

  for(i = 0; i < ps->proglen; i++) {
    if(i == pos)
      continue;
    if(ps->prog[i].op == DFA_SPLIT || ps->prog[i].op == DFA_JMP) {
      /* strictly > pos, not >=: a target of exactly pos means "enter
         whatever comes next", which after this insert is the new
         split itself (its own x already says "enter the body" at
         pos+1) -- not the body directly, so it must NOT shift. */
      if(ps->prog[i].x > (int)pos)
        ps->prog[i].x++;
      if(ps->prog[i].op == DFA_SPLIT && ps->prog[i].y > (int)pos)
        ps->prog[i].y++;
    }
  }

  return (int)pos;
}

/* wrap [body_start, ps->proglen) as `e?` (or, if always_skip, as
 * dead code that's always bypassed -- {0,0}). Greedy: body first. */
static void
wrap_optional(struct dfa_parser* ps, size_t body_start, int always_skip) {
  int split_pc = insert_split(ps, body_start);

  if(ps->err)
    return;

  ps->prog[split_pc].x = always_skip ? (int)ps->proglen : (int)body_start + 1;
  ps->prog[split_pc].y = (int)ps->proglen;
}

/* wrap [body_start, ps->proglen) as `e*` (greedy: body tried first) */
static void
wrap_star(struct dfa_parser* ps, size_t body_start) {
  int split_pc = (int)body_start;

  wrap_optional(ps, body_start, 0);

  if(ps->err)
    return;

  dfa_emit(ps, DFA_JMP, (int)body_start, 0);

  if(ps->err)
    return;

  ps->prog[split_pc].y = (int)ps->proglen;
}

/* wrap [body_start, ps->proglen) as `e+` (run once, then loop) */
static void
wrap_plus(struct dfa_parser* ps, size_t body_start) {
  int split_pc = dfa_emit(ps, DFA_SPLIT, (int)body_start, -1);

  if(ps->err)
    return;

  ps->prog[split_pc].y = (int)ps->proglen;
}

static void parse_alt(struct dfa_parser* ps);
static void parse_atom(struct dfa_parser* ps);

/* alt_len: bytes of an alternation separator at ps->p, or 0 if none
 * -- ERE '|', or BRE '\|' (a GNU extension, not POSIX, but what every
 * BRE implementation grep/sed actually ship supports by default). */
static int
alt_len(struct dfa_parser* ps) {
  if(ps->flags & DFA_ERE)
    return (ps->p < ps->end && *ps->p == '|') ? 1 : 0;

  return (ps->p + 1 < ps->end && ps->p[0] == '\\' && ps->p[1] == '|') ? 2 : 0;
}

/* parse_group: '(' / '\(' already at *ps->p. Emits SAVE, the body
 * (via parse_alt, so ERE '|' works inside groups too), SAVE, and
 * expects the matching close.
 * ----------------------------------------------------------------------- */
static void
parse_group(struct dfa_parser* ps, int ere) {
  const char* open_src = ps->p;
  size_t g;

  ps->p += ere ? 1 : 2;

  if(++ps->nest > DFA_MAXNEST) {
    ps->err = DFA_ESIZE;
    return;
  }

  g = group_index(ps, open_src);

  if(ps->err)
    return;

  dfa_emit(ps, DFA_SAVE, (int)(2 * g), 0);
  parse_alt(ps);

  if(ps->err)
    return;

  if(ere) {
    if(ps->p >= ps->end || *ps->p != ')') {
      ps->err = DFA_EPAREN;
      return;
    }
    ps->p++;
  } else {
    if(ps->p + 1 >= ps->end || ps->p[0] != '\\' || ps->p[1] != ')') {
      ps->err = DFA_EPAREN;
      return;
    }
    ps->p += 2;
  }

  ps->nest--;
  dfa_emit(ps, DFA_SAVE, (int)(2 * g + 1), 0);
}

/* parse_interval_spec: *ps->p is the digit right after the opening
 * \{ or {. Fills m and n (n = -1 = unbounded); leaves ps->p at the
 * closing \} / }, which the caller consumes. */
static int
parse_interval_spec(struct dfa_parser* ps, long* m, long* n) {
  const char* p = ps->p;
  long a = 0, b = -1;
  int gotdigit = 0;

  while(p < ps->end && isdigit((unsigned char)*p)) {
    a = a * 10 + (*p - '0');
    if(a > DFA_MAXDUP) {
      ps->err = DFA_EBRACE;
      return 0;
    }
    p++;
    gotdigit = 1;
  }

  if(!gotdigit) {
    ps->err = DFA_EBRACE;
    return 0;
  }

  if(p < ps->end && *p == ',') {
    p++;

    while(p < ps->end && isdigit((unsigned char)*p)) {
      if(b < 0)
        b = 0;
      b = b * 10 + (*p - '0');
      if(b > DFA_MAXDUP) {
        ps->err = DFA_EBRACE;
        return 0;
      }
      p++;
    }
    /* "\{m,\}": no second number, b stays -1 = unbounded */
  } else {
    b = a; /* "\{m\}" */
  }

  if(b >= 0 && b < a) {
    ps->err = DFA_EBRACE;
    return 0;
  }

  ps->p = p;
  *m = a;
  *n = b;
  return 1;
}

/* apply {m,n} given the first copy already sitting at
 * [body_start, ps->proglen), whose source is [src, srcend). */
static void
apply_interval(struct dfa_parser* ps, size_t body_start, const char* src, const char* srcend, long m,
               long n) {
  long i;

  if(m == 0 && n == 0) {
    wrap_optional(ps, body_start, 1);
    return;
  }

  if(m == 0)
    wrap_optional(ps, body_start, 0);

  for(i = 1; i < m && !ps->err; i++) {
    const char* save_p = ps->p;
    const char* save_end = ps->end;

    ps->p = src;
    ps->end = srcend;
    parse_atom(ps);
    ps->p = save_p;
    ps->end = save_end;
  }

  if(ps->err)
    return;

  if(n < 0) {
    /* {m,}: one more copy, as the body of a * loop */
    size_t loop_start = ps->proglen;
    const char* save_p = ps->p;
    const char* save_end = ps->end;

    ps->p = src;
    ps->end = srcend;
    parse_atom(ps);
    ps->p = save_p;
    ps->end = save_end;

    if(ps->err)
      return;

    wrap_star(ps, loop_start);
    return;
  }

  for(i = m; i < n && !ps->err; i++) {
    size_t opt_start = ps->proglen;
    const char* save_p = ps->p;
    const char* save_end = ps->end;

    ps->p = src;
    ps->end = srcend;
    parse_atom(ps);
    ps->p = save_p;
    ps->end = save_end;

    if(ps->err)
      return;

    wrap_optional(ps, opt_start, 0);
  }
}

/* parse_piece: one atom, plus an optional repeat suffix.
 * ----------------------------------------------------------------------- */
static void
parse_piece(struct dfa_parser* ps) {
  int ere = (ps->flags & DFA_ERE) != 0;
  size_t body_start = ps->proglen;
  const char* atom_src = ps->p;
  const char* atom_srcend;

  parse_atom(ps);

  if(ps->err)
    return;

  atom_srcend = ps->p;

  if(ps->p < ps->end && *ps->p == '*') {
    ps->p++;
    wrap_star(ps, body_start);
  } else if(ere && ps->p < ps->end && *ps->p == '+') {
    ps->p++;
    wrap_plus(ps, body_start);
  } else if(ere && ps->p < ps->end && *ps->p == '?') {
    ps->p++;
    wrap_optional(ps, body_start, 0);
  } else if((ere && ps->p < ps->end && *ps->p == '{' && ps->p + 1 < ps->end &&
             isdigit((unsigned char)ps->p[1])) ||
            (!ere && ps->p + 1 < ps->end && ps->p[0] == '\\' && ps->p[1] == '{')) {
    long m, n;

    ps->p += ere ? 1 : 2;

    if(!parse_interval_spec(ps, &m, &n))
      return;

    if(ere) {
      if(ps->p >= ps->end || *ps->p != '}') {
        ps->err = DFA_EBRACE;
        return;
      }
      ps->p++;
    } else {
      if(ps->p + 1 >= ps->end || ps->p[0] != '\\' || ps->p[1] != '}') {
        ps->err = DFA_EBRACE;
        return;
      }
      ps->p += 2;
    }

    apply_interval(ps, body_start, atom_src, atom_srcend, m, n);
  }
}

/* parse_seq: one or more pieces, i.e. one branch (concatenation) */
static void
parse_seq(struct dfa_parser* ps) {
  int ere = (ps->flags & DFA_ERE) != 0;

  for(;;) {
    if(ps->p >= ps->end)
      break;
    if(alt_len(ps))
      break;
    if(ere && *ps->p == ')')
      break;
    if(!ere && ps->p + 1 < ps->end && ps->p[0] == '\\' && ps->p[1] == ')')
      break;

    parse_piece(ps);

    if(ps->err)
      return;
  }
}

/* parse_alt: alternation (ERE '|', or BRE '\|'), N branches chained
 * through SPLITs, each non-final branch ending in a JMP to the shared
 * end. */
static void
parse_alt(struct dfa_parser* ps) {
  size_t start = ps->proglen;
  int split_pc;
  size_t jmp_pcs[DFA_MAXALT];
  size_t njmp = 0;
  int len;

  parse_seq(ps);

  if(ps->err || !(len = alt_len(ps)))
    return;

  split_pc = insert_split(ps, start);

  if(ps->err)
    return;

  for(;;) {
    int jp = dfa_emit(ps, DFA_JMP, -1, 0);

    if(ps->err)
      return;

    if(njmp < DFA_MAXALT)
      jmp_pcs[njmp++] = (size_t)jp;

    ps->prog[split_pc].y = (int)ps->proglen;

    if(!(len = alt_len(ps)))
      break;

    ps->p += len;

    {
      size_t branch_start = ps->proglen;

      parse_seq(ps);

      if(ps->err)
        return;

      if(!(len = alt_len(ps)))
        break;

      split_pc = insert_split(ps, branch_start);

      if(ps->err)
        return;
    }
  }

  {
    size_t i;

    for(i = 0; i < njmp; i++)
      ps->prog[jmp_pcs[i]].x = (int)ps->proglen;
  }
}

/* parse_atom: one atom -- '.', a bracket expression, a group, a
 * back-reference, an anchor in an anchoring position, or a literal.
 * ----------------------------------------------------------------------- */
static void
parse_atom(struct dfa_parser* ps) {
  int ere = (ps->flags & DFA_ERE) != 0;

  if(ps->p >= ps->end) {
    ps->err = DFA_EBADRPT;
    return;
  }

  if(*ps->p == '.') {
    ps->p++;
    dfa_emit(ps, DFA_ANY, 0, 0);
    return;
  }

  if(*ps->p == '[') {
    unsigned char set[32];
    int idx;

    if(!dfa_bracket_compile(ps, set))
      return;

    idx = dfa_addset(ps, set);
    dfa_emit(ps, DFA_SET, idx, 0);
    return;
  }

  if((ere && *ps->p == '(') || (!ere && ps->p + 1 < ps->end && ps->p[0] == '\\' && ps->p[1] == '(')) {
    parse_group(ps, ere);
    return;
  }

  /* anchors: BRE only at the true start/end of the whole pattern;
     ERE anywhere (POSIX: BRE position-restricted, ERE always special) */
  if(*ps->p == '^' && (ere || ps->p == ps->start)) {
    ps->p++;
    dfa_emit(ps, DFA_BOL, 0, 0);
    return;
  }

  if(*ps->p == '$' && (ere || ps->p + 1 == ps->end)) {
    ps->p++;
    dfa_emit(ps, DFA_EOL, 0, 0);
    return;
  }

  if(*ps->p == '\\') {
    if(ps->p + 1 >= ps->end) {
      ps->err = DFA_EESCAPE;
      return;
    }

    if(ps->p[1] >= '1' && ps->p[1] <= '9') {
      int g = ps->p[1] - '0';

      if((size_t)g > ps->ngroup) {
        ps->err = DFA_ESUBREG;
        return;
      }

      ps->p += 2;
      ps->has_backref = 1;
      dfa_emit(ps, DFA_BACKREF, g, 0);
      return;
    }

    /* unrecognized \X: literal X (also covers \. \* \[ \\ etc, the
       BRE convention -- and \{ \} \+ \? \| when DFA_GNU isn't set) */
    {
      unsigned char c = (unsigned char)ps->p[1];

      ps->p += 2;

      if((ps->flags & DFA_ICASE) && isalpha(c)) {
        unsigned char set[32] = {0};

        set[tolower(c) >> 3] |= (unsigned char)(1u << (tolower(c) & 7));
        set[toupper(c) >> 3] |= (unsigned char)(1u << (toupper(c) & 7));
        dfa_emit(ps, DFA_SET, dfa_addset(ps, set), 0);
      } else {
        dfa_emit(ps, DFA_CHAR, c, 0);
      }
      return;
    }
  }

  {
    unsigned char c = (unsigned char)*ps->p;

    ps->p++;

    if((ps->flags & DFA_ICASE) && isalpha(c)) {
      unsigned char set[32] = {0};

      set[tolower(c) >> 3] |= (unsigned char)(1u << (tolower(c) & 7));
      set[toupper(c) >> 3] |= (unsigned char)(1u << (toupper(c) & 7));
      dfa_emit(ps, DFA_SET, dfa_addset(ps, set), 0);
    } else {
      dfa_emit(ps, DFA_CHAR, c, 0);
    }
  }
}

/* dfa_parse_pattern: entry point, called once by dfa_compile(). */
void
dfa_parse_pattern(struct dfa_parser* ps) {
  parse_alt(ps);

  if(ps->err)
    return;

  if(ps->p != ps->end) {
    ps->err = DFA_EPAREN; /* leftover input: an unmatched ')' */
    return;
  }

  dfa_emit(ps, DFA_MATCH, 0, 0);
}
