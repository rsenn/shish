#include "../builtin.h"
#include "../fdtable.h"
#include "../../lib/buffer.h"
#include "../../lib/fmt.h"
#include "../../lib/scan.h"
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

      /* directive: just the conversion char for now -- configure relies
         almost entirely on %s, %d, and %b; width/precision are uncommon
         and can be added later if needed */
      switch(*f) {
        case 's': {
          const char* s = printf_arg(argc, argv, &idx);
          buffer_puts(fd_out->w, s);
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
          if(s[0])
            buffer_put(fd_out->w, s, 1);
          did_arg = 1;
          break;
        }
        case 'd':
        case 'i': {
          int64 v = printf_arg_long(argc, argv, &idx);
          size_t n = fmt_longlong(numbuf, v);
          buffer_put(fd_out->w, numbuf, n);
          did_arg = 1;
          break;
        }
        case 'u': {
          uint64 v = printf_arg_ulong(argc, argv, &idx);
          size_t n = fmt_ulonglong(numbuf, v);
          buffer_put(fd_out->w, numbuf, n);
          did_arg = 1;
          break;
        }
        case 'o': {
          uint64 v = printf_arg_ulong(argc, argv, &idx);
          size_t n = fmt_8longlong(numbuf, v);
          buffer_put(fd_out->w, numbuf, n);
          did_arg = 1;
          break;
        }
        case 'x':
        case 'X': {
          uint64 v = printf_arg_ulong(argc, argv, &idx);
          size_t n = fmt_xlonglong(numbuf, v);
          buffer_put(fd_out->w, numbuf, n);
          did_arg = 1;
          break;
        }
        default:
          /* unknown directive: emit the % and the char as a fallback,
             rather than aborting --- configure scripts sometimes pass
             through user-provided format strings */
          buffer_put(fd_out->w, "%", 1);
          if(*f)
            buffer_put(fd_out->w, f, 1);
          break;
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
