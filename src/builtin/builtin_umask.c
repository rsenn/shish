#include "../builtin.h"
#include "../sh.h"
#include "../fdtable.h"
#include "../../lib/str.h"
#include "../../lib/fmt.h"
#include "../../lib/scan.h"
#include "../../lib/uint16.h"
#include <sys/types.h>
#include <sys/stat.h>

size_t
fmt_rwx(char* out, uint16 bits) {
  char* dst = out;

  if(bits & 4)
    *dst++ = 'r';

  if(bits & 2)
    *dst++ = 'w';

  if(bits & 1)
    *dst++ = 'x';

  return dst - out;
}

size_t
fmt_umask(char* out, uint16 umask) {
  char* dst = out;

  dst += str_copy(dst, "u=");
  dst += fmt_rwx(dst, umask >> 6);
  dst += str_copy(dst, ",g=");
  dst += fmt_rwx(dst, umask >> 3);
  dst += str_copy(dst, ",o=");
  dst += fmt_rwx(dst, umask);

  return dst - out;
}

size_t
scan_rwx(char* in, uint16* bits) {
  char* src;

  for(src = in; *src; src++) {
    switch(*src) {
      case 'r': *bits |= 4; continue;
      case 'w': *bits |= 2; continue;
      case 'x': *bits |= 1; continue;
    }

    break;
  }

  return src - in;
}

/* parse a symbolic mode: clause[,clause]...
 *
 *   clause  ::= [ugoa]* ( [+-=] ( [rwx]* | [ugo] ) )+
 *   perms       the mask's complement, edited in place
 *   returns     bytes consumed, 0 if `in` is not a valid mode
 * ----------------------------------------------------------------------- */
size_t
scan_umask(char* in, uint16* umask) {
  char* src = in;
  uint16 perms = ~*umask & 0777;

  for(;;) {
    uint16 who = 0;

    for(;; src++) {
      if(*src == 'u') who |= 0700;
      else if(*src == 'g') who |= 0070;
      else if(*src == 'o') who |= 0007;
      else if(*src == 'a') who |= 0777;
      else break;
    }

    if(!who)
      who = 0777;

    if(!*src || str_chr("=+-", *src) == 3)
      return 0;

    while(*src && str_chr("=+-", *src) < 3) {
      char op = *src++;
      uint16 bits = 0;

      if(*src && str_chr("ugo", *src) < 3) {
        uint16 cls = perms >> (*src == 'u' ? 6 : *src == 'g' ? 3 : 0) & 7;

        bits = cls << 6 | cls << 3 | cls;
        src++;
      } else {
        size_t n = scan_rwx(src, &bits);

        src += n;
        bits = bits << 6 | bits << 3 | bits;
      }

      bits &= who;

      if(op == '+')
        perms |= bits;
      else if(op == '-')
        perms &= ~bits;
      else
        perms = perms & ~who | bits;
    }

    if(*src == ',')
      src++;
    else
      break;
  }

  if(*src)
    return 0;

  *umask = ~perms & 0777;
  return src - in;
}

/* umask built-in
 * ----------------------------------------------------------------------- */
const char help_umask[] = "    Print or set the file mode creation mask.\n"
                          "\n"
                          "    -p              prefix the printed mask with 'umask ', reusable\n"
                          "                    as input\n"
                          "    -S              print/parse the mask symbolically (u=,g=,o=)\n"
                          "                    instead of as an octal number\n"
                          "    mode            new mask, octal or symbolic (see chmod)\n";

int
builtin_umask(int argc, char* argv[]) {
  int c, symbolic = 0, print = 0;

  /* check options, -p for print, -S for symbolic output */
  while((c = shell_getopt(argc, argv, "pS")) > 0) {
    switch(c) {
      case 'p': print = 1; break;
      case 'S': symbolic = 1; break;
      default: builtin_invopt(argv); return 1;
    }
  }

  if(shell_optind < argc) {
    uint16 num = sh->umask, prev = sh->umask;

    if(scan_8short(argv[shell_optind], &num))
      sh->umask = num;
    else {
      num = sh->umask;
      if(scan_umask(argv[shell_optind], &num))
        sh->umask = num;
      else {
        builtin_errmsg(argv, argv[shell_optind], "invalid mode");
        return 1;
      }
    }

    if(sh->umask != prev)
      umask(sh->umask);

  } else {
    /* print umask, suitable for re-input */
    char buf[64];
    size_t n = symbolic ? fmt_umask(buf, ~sh->umask) : fmt_8long(buf, sh->umask);

    if(print)
      buffer_puts(fd_out->w, "umask ");

    if(!symbolic && n < 4)
      buffer_putnc(fd_out->w, '0', 4 - n);

    buffer_put(fd_out->w, buf, n);
    buffer_putnlflush(fd_out->w);
    return 0;
  }

  return 0;
}
