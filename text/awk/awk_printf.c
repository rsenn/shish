/* awk's printf/sprintf: parses the format itself (flags/width/
 * precision, '*' for either), then hands each conversion off to
 * libc snprintf with a spec string built purely from the parsed
 * numeric pieces plus one fixed conversion letter -- never from the
 * user-controlled bytes directly -- so there is no format-string
 * injection risk despite CONVFMT/OFMT-style trust elsewhere. */
#include "awk_internal.h"
#include "../../lib/str.h"
#include "../../lib/byte.h"
#include "../../lib/alloc.h"
#include <stdio.h> /* snprintf */

static awk_cell
next_arg(struct awk_state* st, struct anode** argp) {
  awk_cell c;

  byte_zero(&c, sizeof(c));

  if(!*argp)
    return c;

  c = awk_eval(st, *argp);
  *argp = (*argp)->next;
  return c;
}

static void
emit(stralloc* out, const char* buf, int n) {
  if(n > 0)
    stralloc_catb(out, buf, (size_t)n);
}

int
awk_sprintf(
    struct awk_state* st, stralloc* out, const char* fmt, size_t fmtlen, struct anode* args) {
  const char* p = fmt;
  const char* end = fmt + fmtlen;

  while(p < end) {
    int left = 0, plus = 0, space = 0, zero = 0, alt = 0;
    long width = -1, prec = -1;
    int haswidth = 0, hasprec = 0;
    char conv;
    char spec[40];
    size_t sl = 0;
    const char* specstart = p;

    if(*p != '%') {
      stralloc_catc(out, *p);
      p++;
      continue;
    }

    p++;

    if(p < end && *p == '%') {
      stralloc_catc(out, '%');
      p++;
      continue;
    }

    for(; p < end; p++) {
      if(*p == '-')
        left = 1;
      else if(*p == '+')
        plus = 1;
      else if(*p == ' ')
        space = 1;
      else if(*p == '0')
        zero = 1;
      else if(*p == '#')
        alt = 1;
      else
        break;
    }

    if(p < end && *p == '*') {
      awk_cell c = next_arg(st, &args);

      width = (long)awk_tonum(st, &c);
      haswidth = 1;
      p++;

      if(width < 0) {
        left = 1;
        width = -width;
      }
    } else {
      int any = 0;
      long w = 0;

      while(p < end && *p >= '0' && *p <= '9') {
        w = w * 10 + (*p - '0');
        p++;
        any = 1;
      }

      if(any) {
        width = w;
        haswidth = 1;
      }
    }

    if(p < end && *p == '.') {
      p++;
      hasprec = 1;

      if(p < end && *p == '*') {
        awk_cell c = next_arg(st, &args);

        prec = (long)awk_tonum(st, &c);
        p++;

        if(prec < 0) {
          hasprec = 0;
          prec = -1;
        }
      } else {
        long pr = 0;

        while(p < end && *p >= '0' && *p <= '9') {
          pr = pr * 10 + (*p - '0');
          p++;
        }

        prec = pr;
      }
    }

    /* clamp: keeps both the spec buffer below and the per-conversion
       output allocation (48/64 + width/prec) from an absurd %*d
       argument turning into a multi-gigabyte alloc() */
    if(haswidth && width > 100000)
      width = 100000;
    if(hasprec && prec > 100000)
      prec = 100000;

    if(p >= end) {
      /* trailing lone '%' or an unterminated spec: emit literally */
      stralloc_catb(out, specstart, (size_t)(end - specstart));
      break;
    }

    conv = *p++;

    /* build "%<flags><width><.prec>[ll]<conv>" -- every piece here
       came from parsed digits/flags above, never from `fmt` verbatim,
       so this spec is safe to hand to snprintf regardless of what the
       program's format string contained. */
    spec[sl++] = '%';

    if(left)
      spec[sl++] = '-';
    if(plus)
      spec[sl++] = '+';
    if(space)
      spec[sl++] = ' ';
    if(zero)
      spec[sl++] = '0';
    if(alt)
      spec[sl++] = '#';

    if(haswidth)
      sl += (size_t)snprintf(spec + sl, sizeof(spec) - sl, "%ld", width);

    if(hasprec)
      sl += (size_t)snprintf(spec + sl, sizeof(spec) - sl, ".%ld", prec);

    switch(conv) {
      case 'd':
      case 'i':
      case 'o':
      case 'x':
      case 'X':
      case 'u':
        spec[sl++] = 'l';
        spec[sl++] = 'l';
        break;
      default: break;
    }

    spec[sl++] = conv;
    spec[sl] = 0;

    switch(conv) {
      case 'd':
      case 'i': {
        awk_cell c = next_arg(st, &args);
        long long v = (long long)awk_tonum(st, &c);
        size_t cap = 48 + (haswidth && width > 0 ? (size_t)width : 0);
        char* buf = alloc(cap);

        emit(out, buf, snprintf(buf, cap, spec, v));
        alloc_free(buf);
        break;
      }

      case 'o':
      case 'x':
      case 'X':
      case 'u': {
        awk_cell c = next_arg(st, &args);
        unsigned long long v = (unsigned long long)(long long)awk_tonum(st, &c);
        size_t cap = 48 + (haswidth && width > 0 ? (size_t)width : 0);
        char* buf = alloc(cap);

        emit(out, buf, snprintf(buf, cap, spec, v));
        alloc_free(buf);
        break;
      }

      case 'e':
      case 'E':
      case 'f':
      case 'F':
      case 'g':
      case 'G': {
        awk_cell c = next_arg(st, &args);
        double v = awk_tonum(st, &c);
        size_t cap = 64 + (haswidth && width > 0 ? (size_t)width : 0) +
                     (hasprec && prec > 0 ? (size_t)prec : 0);
        char* buf = alloc(cap);

        emit(out, buf, snprintf(buf, cap, spec, v));
        alloc_free(buf);
        break;
      }

      case 'c': {
        awk_cell c = next_arg(st, &args);
        int ch;
        size_t cap = 32 + (haswidth && width > 0 ? (size_t)width : 0);
        char* buf = alloc(cap);
        char cspec[24];
        size_t csl = 0;

        if(c.type == CELL_STR || c.type == CELL_STRNUM)
          ch = c.str && c.str[0] ? (unsigned char)c.str[0] : 0;
        else
          ch = (int)awk_tonum(st, &c);

        /* %c ignores precision */
        cspec[csl++] = '%';

        if(left)
          cspec[csl++] = '-';

        if(haswidth)
          csl += (size_t)snprintf(cspec + csl, sizeof(cspec) - csl, "%ld", width);

        cspec[csl++] = 'c';
        cspec[csl] = 0;

        emit(out, buf, snprintf(buf, cap, cspec, ch));
        alloc_free(buf);
        break;
      }

      case 's': {
        awk_cell c = next_arg(st, &args);
        const char* s = awk_tostr(st, &c, 0);
        size_t cap = str_len(s) + 32 + (haswidth && width > 0 ? (size_t)width : 0);
        char* buf = alloc(cap);

        emit(out, buf, snprintf(buf, cap, spec, s));
        alloc_free(buf);
        break;
      }

      default:
        /* unknown conversion: emit the spec text literally, no argument consumed */
        stralloc_catb(out, specstart, (size_t)(p - specstart));
        break;
    }
  }

  return 0;
}
