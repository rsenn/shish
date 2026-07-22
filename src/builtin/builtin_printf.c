#include "../builtin.h"
#include "../fdtable.h"
#include "../../lib/buffer.h"
#include "../../lib/fmt.h"
#include "../../lib/scan.h"
#include "../../lib/str.h"
#include "../../lib/uint64.h"

/* expand a single backslash escape starting at p[0] (the '\\').
 * writes the resulting char to *out, returns the number of source
 * bytes consumed (including the leading backslash). If the sequence
 * is `\c`, sets *stop = 1 and returns the consumed count. */
static unsigned
printf_esc(const char* p, char* out, int* stop) {
  unsigned char ch = 0;
  unsigned n;

  switch(p[1]) {
    case 'a': *out = '\a'; return 2;
    case 'b': *out = '\b'; return 2;
    case 'f': *out = '\f'; return 2;
    case 'n': *out = '\n'; return 2;
    case 'r': *out = '\r'; return 2;
    case 't': *out = '\t'; return 2;
    case 'v': *out = '\v'; return 2;
    case '\\': *out = '\\'; return 2;
    case '"': *out = '"'; return 2;
    case '/': *out = '/'; return 2;
    case 'c': *stop = 1; return 2;
    case 'x':
      n = scan_xchar(&p[2], &ch);
      if(n == 0) {
        *out = '\\';
        return 1;
      }
      *out = (char)ch;
      return 2 + n;
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7': {
      unsigned int v;
      n = scan_8int(&p[1], &v);
      if(n == 0) {
        *out = '\\';
        return 1;
      }
      *out = (char)(v & 0xff);
      return 1 + n;
    }
    case '\0':
      *out = '\\';
      return 1;
    default:
      /* unknown escape: copy backslash verbatim */
      *out = '\\';
      return 1;
  }
}

/* emit format-literal text up to the next '%' or end. Returns position
 * advanced past the literal. Sets *stop if `\c` is encountered. */
static const char*
printf_emit_literal(const char* fmt, int* stop) {
  char buf[1];
  while(*fmt && *fmt != '%') {
    if(*fmt == '\\') {
      unsigned n = printf_esc(fmt, buf, stop);
      if(*stop)
        return fmt + n;
      buffer_put(fd_out->w, buf, 1);
      fmt += n;
    } else {
      buffer_put(fd_out->w, fmt, 1);
      fmt++;
    }
  }
  return fmt;
}

/* unescape an argument that is the target of %b (same rules as for the
 * format string itself, including \c which stops output) */
static void
printf_emit_bstring(const char* s, int* stop) {
  char buf[1];
  while(*s) {
    if(*s == '\\') {
      unsigned n = printf_esc(s, buf, stop);
      if(*stop)
        return;
      buffer_put(fd_out->w, buf, 1);
      s += n;
    } else {
      buffer_put(fd_out->w, s, 1);
      s++;
    }
  }
}

/* take next arg, treat empty as 0/empty per the spec */
static const char*
printf_arg(int argc, char* argv[], int* idx) {
  if(*idx >= argc)
    return "";
  return argv[(*idx)++];
}

static int64
printf_arg_long(int argc, char* argv[], int* idx) {
  const char* s = printf_arg(argc, argv, idx);
  int64 v = 0;
  if(s[0] == '\'' || s[0] == '"') {
    /* per POSIX: a leading single or double quote means the value
       is the numeric value of the character that follows */
    return (int64)(unsigned char)s[1];
  }
  scan_longlong(s, &v);
  return v;
}

static uint64
printf_arg_ulong(int argc, char* argv[], int* idx) {
  const char* s = printf_arg(argc, argv, idx);
  uint64 v = 0;
  if(s[0] == '\'' || s[0] == '"')
    return (uint64)(unsigned char)s[1];
  scan_ulonglong(s, &v);
  return v;
}

/* a parsed "%[flags][width][.precision]" directive, minus the final
 * conversion character */
typedef struct {
  int left, zero, plus, space, alt;
  int width; /* -1 if unspecified */
  int prec;  /* -1 if unspecified */
} printf_spec;

static const char*
printf_parse_flags(const char* f, printf_spec* sp) {
  for(;;) {
    switch(*f) {
      case '-': sp->left = 1; break;
      case '0': sp->zero = 1; break;
      case '+': sp->plus = 1; break;
      case ' ': sp->space = 1; break;
      case '#': sp->alt = 1; break;
      default: return f;
    }
    f++;
  }
}

/* parse a "%[flags][width][.precision]" spec starting right after the
 * '%'; a '*' width/precision consumes the next printf argument. Returns
 * a pointer to the conversion character. */
static const char*
printf_parse_spec(const char* f, printf_spec* sp, int argc, char* argv[],
                   int* idx) {
  sp->left = sp->zero = sp->plus = sp->space = sp->alt = 0;
  sp->width = -1;
  sp->prec = -1;

  f = printf_parse_flags(f, sp);

  if(*f == '*') {
    int64 w = printf_arg_long(argc, argv, idx);
    if(w < 0) {
      sp->left = 1;
      w = -w;
    }
    sp->width = (int)w;
    f++;
  } else if(*f >= '0' && *f <= '9') {
    unsigned int w = 0;
    f += scan_uint(f, &w);
    sp->width = (int)w;
  }

  if(*f == '.') {
    f++;
    if(*f == '*') {
      int64 p = printf_arg_long(argc, argv, idx);
      sp->prec = p < 0 ? -1 : (int)p;
      f++;
    } else {
      unsigned int p = 0;
      f += scan_uint(f, &p);
      sp->prec = (int)p;
    }
  }

  return f;
}

/* emit a (sign)(prefix)(zero-padding to precision)(digits) numeric
 * conversion, applying width/flag padding around it */
static void
printf_emit_numeric(const printf_spec* sp, char sign, const char* prefix,
                     const char* digits, size_t dlen) {
  size_t plen = prefix ? str_len(prefix) : 0;
  size_t prec_pad = 0;
  size_t body, width, pad;
  int zero_pad;

  if(sp->prec == 0 && dlen == 1 && digits[0] == '0')
    dlen = 0;
  else if(sp->prec > (int)dlen)
    prec_pad = (size_t)sp->prec - dlen;

  body = (sign ? 1u : 0u) + plen + prec_pad + dlen;
  width = sp->width > 0 ? (size_t)sp->width : 0;
  pad = width > body ? width - body : 0;

  zero_pad = sp->zero && !sp->left && sp->prec < 0;

  if(!sp->left && !zero_pad)
    while(pad--)
      buffer_put(fd_out->w, " ", 1);

  if(sign)
    buffer_put(fd_out->w, &sign, 1);
  if(plen)
    buffer_put(fd_out->w, prefix, plen);

  if(!sp->left && zero_pad)
    while(pad--)
      buffer_put(fd_out->w, "0", 1);

  while(prec_pad--)
    buffer_put(fd_out->w, "0", 1);

  if(dlen)
    buffer_put(fd_out->w, digits, dlen);

  if(sp->left)
    while(pad--)
      buffer_put(fd_out->w, " ", 1);
}

int
builtin_printf(int argc, char* argv[]) {
  const char* fmt;
  int idx, did_arg, stop = 0;
  char numbuf[FMT_8LONG + 1];

  if(argc < 2) {
    builtin_errmsg(argv, "printf", "usage: printf format [args...]");
    return 1;
  }

  fmt = argv[1];
  idx = 2;

  do {
    const char* f = fmt;
    did_arg = 0;

    while(*f && !stop) {
      f = printf_emit_literal(f, &stop);
      if(stop || !*f)
        break;
      /* *f == '%' */
      f++;
      if(*f == '%') {
        buffer_put(fd_out->w, "%", 1);
        f++;
        continue;
      }

      {
        const char* spec_start = f;
        printf_spec sp;
        f = printf_parse_spec(f, &sp, argc, argv, &idx);

        switch(*f) {
          case 's': {
            const char* s = printf_arg(argc, argv, &idx);
            size_t slen = str_len(s);
            size_t width, pad;
            if(sp.prec >= 0 && (size_t)sp.prec < slen)
              slen = (size_t)sp.prec;
            width = sp.width > 0 ? (size_t)sp.width : 0;
            pad = width > slen ? width - slen : 0;
            if(!sp.left)
              while(pad--)
                buffer_put(fd_out->w, " ", 1);
            if(slen)
              buffer_put(fd_out->w, s, slen);
            if(sp.left)
              while(pad--)
                buffer_put(fd_out->w, " ", 1);
            did_arg = 1;
            break;
          }
          case 'b': {
            const char* s = printf_arg(argc, argv, &idx);
            printf_emit_bstring(s, &stop);
            did_arg = 1;
            break;
          }
          case 'c': {
            const char* s = printf_arg(argc, argv, &idx);
            size_t clen = s[0] ? 1 : 0;
            size_t width = sp.width > 0 ? (size_t)sp.width : 0;
            size_t pad = width > clen ? width - clen : 0;
            if(!sp.left)
              while(pad--)
                buffer_put(fd_out->w, " ", 1);
            if(clen)
              buffer_put(fd_out->w, s, 1);
            if(sp.left)
              while(pad--)
                buffer_put(fd_out->w, " ", 1);
            did_arg = 1;
            break;
          }
          case 'd':
          case 'i': {
            int64 v = printf_arg_long(argc, argv, &idx);
            uint64 uv;
            char sign = 0;
            size_t n;
            if(v < 0) {
              sign = '-';
              uv = (uint64)(-(v + 1)) + 1;
            } else {
              uv = (uint64)v;
              if(sp.plus)
                sign = '+';
              else if(sp.space)
                sign = ' ';
            }
            n = fmt_ulonglong(numbuf, uv);
            printf_emit_numeric(&sp, sign, NULL, numbuf, n);
            did_arg = 1;
            break;
          }
          case 'u': {
            uint64 v = printf_arg_ulong(argc, argv, &idx);
            size_t n = fmt_ulonglong(numbuf, v);
            printf_emit_numeric(&sp, 0, NULL, numbuf, n);
            did_arg = 1;
            break;
          }
          case 'o': {
            uint64 v = printf_arg_ulong(argc, argv, &idx);
            size_t n = fmt_8longlong(numbuf, v);
            if(sp.alt && (n == 0 || numbuf[0] != '0') && sp.prec <= (int)n)
              sp.prec = (int)n + 1;
            printf_emit_numeric(&sp, 0, NULL, numbuf, n);
            did_arg = 1;
            break;
          }
          case 'x':
          case 'X': {
            uint64 v = printf_arg_ulong(argc, argv, &idx);
            size_t n = fmt_xlonglong(numbuf, v);
            const char* prefix = NULL;
            if(*f == 'X') {
              size_t i;
              for(i = 0; i < n; i++)
                if(numbuf[i] >= 'a' && numbuf[i] <= 'f')
                  numbuf[i] = (char)(numbuf[i] - ('a' - 'A'));
            }
            if(sp.alt && v != 0)
              prefix = (*f == 'X') ? "0X" : "0x";
            printf_emit_numeric(&sp, 0, prefix, numbuf, n);
            did_arg = 1;
            break;
          }
          default:
            /* unknown directive: emit the original "%..." text as a
               fallback, rather than aborting --- configure scripts
               sometimes pass through user-provided format strings */
            buffer_put(fd_out->w, "%", 1);
            buffer_put(fd_out->w, spec_start, (size_t)(f - spec_start));
            if(*f)
              buffer_put(fd_out->w, f, 1);
            break;
        }
      }
      if(*f)
        f++;
    }
    /* if there are still args left and the format used at least one
       arg this round, repeat the format -- POSIX behavior */
  } while(!stop && did_arg && idx < argc);

  buffer_flush(fd_out->w);
  return 0;
}
