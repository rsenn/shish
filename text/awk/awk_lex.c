#include "awk_internal.h"
#include "../../lib/str.h"
#include "../../lib/byte.h"
#include "../../lib/alloc.h"
#include <stdlib.h> /* strtod */

/* "does the previous token end an operand" -- POSIX's rule for
 * disambiguating '/' (division) from the start of an ERE literal. */
static int
ends_operand(int tok) {
  switch(tok) {
    case T_NAME:
    case T_NUMBER:
    case T_STRING:
    case T_ERE:
    case T_RPAREN:
    case T_RBRACKET:
    case T_INCR:
    case T_DECR:
    case T_BUILTIN:
    case T_DOLLAR: return 1;
    default: return 0;
  }
}

struct kw {
  const char* name;
  int tok;
};

static const struct kw keywords[] = {
    {"BEGIN", T_BEGIN},
    {"END", T_END},
    {"function", T_FUNCTION},
    {"func", T_FUNCTION},
    {"if", T_IF},
    {"else", T_ELSE},
    {"while", T_WHILE},
    {"for", T_FOR},
    {"do", T_DO},
    {"break", T_BREAK},
    {"continue", T_CONTINUE},
    {"next", T_NEXT},
    {"nextfile", T_NEXTFILE},
    {"exit", T_EXIT},
    {"return", T_RETURN},
    {"delete", T_DELETE},
    {"in", T_IN},
    {"print", T_PRINT},
    {"printf", T_PRINTF},
    {"getline", T_GETLINE},
    {NULL, 0},
};

static const char* const builtins[] = {
    "length", "substr",  "index",   "split",  "sub",   "gsub",   "match", "sprintf",
    "sin",    "cos",     "atan2",   "exp",    "log",   "sqrt",   "int",   "rand",
    "srand",  "tolower", "toupper", "system", "close", "fflush", NULL,
};

static int
is_builtin_name(const char* s, size_t n) {
  size_t i;

  for(i = 0; builtins[i]; i++)
    if(str_len(builtins[i]) == n && !byte_diff(builtins[i], n, s))
      return 1;

  return 0;
}

void
awk_lex_init(struct awk_lexer* lx, const char* src, size_t len) {
  byte_zero(lx, sizeof(*lx));
  lx->p = src;
  lx->end = src + len;
  lx->line = 1;
  lx->prev = T_NEWLINE; /* a program may start with a regex */
}

/* skip blanks/comments/backslash-newline; does not consume real newlines */
static void
skip_blank(struct awk_lexer* lx) {
  for(;;) {
    if(lx->p < lx->end && (*lx->p == ' ' || *lx->p == '\t')) {
      lx->p++;
    } else if(lx->p + 1 < lx->end && lx->p[0] == '\\' && lx->p[1] == '\n') {
      lx->p += 2;
      lx->line++;
    } else if(lx->p < lx->end && *lx->p == '#') {
      while(lx->p < lx->end && *lx->p != '\n')
        lx->p++;
    } else {
      break;
    }
  }
}

char*
awk_unescape(arena* a, const char* s, size_t n, size_t* outlen) {
  char* out = arena_alloc(a, n + 1, 1);
  size_t i = 0, o = 0;

  if(!out)
    return NULL;

  while(i < n) {
    unsigned char c = s[i];

    if(c != '\\' || i + 1 >= n) {
      out[o++] = c;
      i++;
      continue;
    }

    {
      unsigned char e = s[i + 1];

      switch(e) {
        case '\\':
          out[o++] = '\\';
          i += 2;
          break;
        case '"':
          out[o++] = '"';
          i += 2;
          break;
        case '/':
          out[o++] = '/';
          i += 2;
          break;
        case 'a':
          out[o++] = '\a';
          i += 2;
          break;
        case 'b':
          out[o++] = '\b';
          i += 2;
          break;
        case 'f':
          out[o++] = '\f';
          i += 2;
          break;
        case 'n':
          out[o++] = '\n';
          i += 2;
          break;
        case 'r':
          out[o++] = '\r';
          i += 2;
          break;
        case 't':
          out[o++] = '\t';
          i += 2;
          break;
        case 'v':
          out[o++] = '\v';
          i += 2;
          break;

        default:
          if(e >= '0' && e <= '7') {
            unsigned val = 0;
            size_t j = i + 1, lim = j + 3;

            while(j < n && j < lim && s[j] >= '0' && s[j] <= '7') {
              val = val * 8 + (s[j] - '0');
              j++;
            }

            out[o++] = (char)val;
            i = j;
          } else {
            /* unrecognized: keep both bytes literally, so regex
               metacharacter escapes (\. \* \[ ...) survive for
               dfa_compile to interpret */
            out[o++] = '\\';
            out[o++] = e;
            i += 2;
          }
      }
    }
  }

  out[o] = 0;
  *outlen = o;
  return out;
}

static int
lex_string(struct awk_lexer* lx) {
  const char* start;
  int had_nl = 0;

  lx->p++; /* opening '"' */
  start = lx->p;

  while(lx->p < lx->end && *lx->p != '"') {
    if(*lx->p == '\n') {
      had_nl = 1;
      break;
    }

    if(*lx->p == '\\' && lx->p + 1 < lx->end)
      lx->p++;

    lx->p++;
  }

  if(had_nl || lx->p >= lx->end) {
    lx->err = AWK_ESYNTAX;
    return T_EOF;
  }

  lx->sval = start;
  lx->slen = (size_t)(lx->p - start);
  lx->p++; /* closing '"' */
  return T_STRING;
}

static int
lex_ere(struct awk_lexer* lx) {
  const char* start;

  lx->p++; /* opening '/' */
  start = lx->p;

  while(lx->p < lx->end && *lx->p != '/' && *lx->p != '\n') {
    if(*lx->p == '\\' && lx->p + 1 < lx->end)
      lx->p++;

    lx->p++;
  }

  if(lx->p >= lx->end || *lx->p != '/') {
    lx->err = AWK_ESYNTAX;
    return T_EOF;
  }

  lx->sval = start;
  lx->slen = (size_t)(lx->p - start);
  lx->p++; /* closing '/' */
  return T_ERE;
}

static int
lex_name(struct awk_lexer* lx) {
  const char* start = lx->p;

  while(lx->p < lx->end && (*lx->p == '_' || (*lx->p >= 'a' && *lx->p <= 'z') ||
                            (*lx->p >= 'A' && *lx->p <= 'Z') || (*lx->p >= '0' && *lx->p <= '9')))
    lx->p++;

  lx->sval = start;
  lx->slen = (size_t)(lx->p - start);

  {
    size_t i;

    for(i = 0; keywords[i].name; i++)
      if(str_len(keywords[i].name) == lx->slen && !byte_diff(keywords[i].name, lx->slen, start))
        return keywords[i].tok;
  }

  if(is_builtin_name(start, lx->slen))
    return T_BUILTIN;

  /* FUNC_NAME: a name immediately followed by '(' with no blank */
  if(lx->p < lx->end && *lx->p == '(')
    return T_FUNC_NAME;

  return T_NAME;
}

static int
lex_number(struct awk_lexer* lx) {
  char* endp;
  double v = strtod(lx->p, &endp);

  lx->sval = lx->p;
  lx->slen = (size_t)(endp - lx->p);
  lx->p = endp;
  lx->numval = v;
  return T_NUMBER;
}

/* two-character operators fall through to their one-character base
   when the second character doesn't match. */
static int
lex_op(struct awk_lexer* lx, int want_ere) {
  char c = *lx->p++;
  char n = (lx->p < lx->end) ? *lx->p : 0;

#define TWO(ch, two_tok, one_tok) \
  if(n == (ch)) { \
    lx->p++; \
    return (two_tok); \
  } \
  return (one_tok)

  switch(c) {
    case '{': return T_LBRACE;
    case '}': return T_RBRACE;
    case '(': lx->paren_depth++; return T_LPAREN;
    case ')':
      if(lx->paren_depth > 0)
        lx->paren_depth--;
      return T_RPAREN;
    case '[': return T_LBRACKET;
    case ']': return T_RBRACKET;
    case ';': return T_SEMI;
    case ',': return T_COMMA;
    case '$': return T_DOLLAR;
    case '~': return T_TILDE;
    case '?': return T_QUESTION;
    case ':': return T_COLON;
    case '^': TWO('=', T_POW_ASSIGN, T_CARET);
    case '%': TWO('=', T_MOD_ASSIGN, T_PERCENT);
    case '=': TWO('=', T_EQ, T_ASSIGN);
    case '!':
      if(n == '=') {
        lx->p++;
        return T_NE;
      }
      if(n == '~') {
        lx->p++;
        return T_NOMATCH;
      }
      return T_NOT;
    case '<': TWO('=', T_LE, T_LT);
    case '>':
      if(n == '=') {
        lx->p++;
        return T_GE;
      }
      if(n == '>') {
        lx->p++;
        return T_APPEND;
      }
      return T_GT;
    case '+':
      if(n == '+') {
        lx->p++;
        return T_INCR;
      }
      if(n == '=') {
        lx->p++;
        return T_ADD_ASSIGN;
      }
      return T_PLUS;
    case '-':
      if(n == '-') {
        lx->p++;
        return T_DECR;
      }
      if(n == '=') {
        lx->p++;
        return T_SUB_ASSIGN;
      }
      return T_MINUS;
    case '*': TWO('=', T_MUL_ASSIGN, T_STAR);
    case '&':
      if(n == '&') {
        lx->p++;
        return T_ANDAND;
      }
      lx->err = AWK_ESYNTAX;
      return T_EOF;
    case '|':
      if(n == '|') {
        lx->p++;
        return T_OROR;
      }
      return T_PIPE;
    case '/':
      if(want_ere) {
        lx->p--; /* lex_ere expects to consume the opening '/' itself */
        return lex_ere(lx);
      }

      if(n == '=') {
        lx->p++;
        return T_DIV_ASSIGN;
      }
      return T_SLASH;
    default: lx->err = AWK_ESYNTAX; return T_EOF;
  }
#undef TWO
}

static int
lex_one(struct awk_lexer* lx) {
  skip_blank(lx);

  if(lx->p >= lx->end)
    return T_EOF;

  if(*lx->p == '\n') {
    lx->p++;
    lx->line++;
    return T_NEWLINE;
  }

  if(*lx->p == '"')
    return lex_string(lx);

  if(*lx->p == '_' || (*lx->p >= 'a' && *lx->p <= 'z') || (*lx->p >= 'A' && *lx->p <= 'Z'))
    return lex_name(lx);

  if((*lx->p >= '0' && *lx->p <= '9') ||
     (*lx->p == '.' && lx->p + 1 < lx->end && lx->p[1] >= '0' && lx->p[1] <= '9'))
    return lex_number(lx);

  return lex_op(lx, !ends_operand(lx->prev));
}

int
awk_lex_next(struct awk_lexer* lx) {
  if(lx->have_lookahead) {
    lx->have_lookahead = 0;
    lx->prev = lx->tok;
    return lx->tok;
  }

  lx->tok = lex_one(lx);
  lx->prev = lx->tok;
  return lx->tok;
}

int
awk_lex_peek(struct awk_lexer* lx) {
  if(!lx->have_lookahead) {
    int saved_prev = lx->prev;

    lx->tok = lex_one(lx);
    lx->have_lookahead = 1;
    lx->prev = saved_prev; /* not yet consumed */
  }

  return lx->tok;
}

int
awk_lex_force_ere(struct awk_lexer* lx) {
  /* only valid right after a peek that returned T_SLASH/T_DIV_ASSIGN
     and rewound nothing else -- the parser calls this instead of
     consuming that token when the grammar position requires a regex
     (e.g. as a sub()/match() argument) rather than division. */
  lx->have_lookahead = 0;
  lx->p -= (lx->tok == T_DIV_ASSIGN) ? 2 : 1;
  lx->tok = lex_ere(lx);
  lx->prev = lx->tok;
  return lx->tok;
}
