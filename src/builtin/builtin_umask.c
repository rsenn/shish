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

size_t
scan_umask(char* in, uint16* umask) {
  char *src, c, op;
  for(src = in; *src; src++) {
    uint16_t bits = 0, shift = 0;
    size_t n;
    c = *src++;
    op = *src++;
    if(str_chr("=+-", op) == 3)
      return 0;
    n = scan_rwx(src, &bits);
    switch(c) {
      case 'u': shift = 6; break;
      case 'g': shift = 3; break;
      case 'o': shift = 0; break;
      default: return 0;
    }

    switch(op) {
      case '=':
        *umask |= (0x7 << shift);
        *umask &= ~(bits << shift);
        break;
      case '+': *umask &= ~(bits << shift); break;
      case '-': *umask |= (bits << shift); break;
    }

    if(*(src += n) != ',')
      break;
  }
  //*umask ^= 0777;
  return src - in;
}

/* umask built-in
 * ----------------------------------------------------------------------- */
int
builtin_umask(int argc, char* argv[]) {
  int c;
  int symbolic = 0, print = 0;

  /* check options, -p for print, -S for symbolic output */
  while((c = shell_getopt(argc, argv, "pS")) > 0) {
    switch(c) {
      case 'p': print = 1; break;
      case 'S': symbolic = 1; break;
      default: builtin_invopt(argv); return 1;
    }
  }

  if(shell_optind < argc) {
    uint16 num, prev = sh->umask;

    if(scan_8short(argv[shell_optind], &num) || scan_umask(argv[shell_optind], &num))
      sh->umask = num;

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
