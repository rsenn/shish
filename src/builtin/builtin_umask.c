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
  char *src, op;
  int who_mask, shift, all_classes;

  for(src = in; *src;) {
    uint16 bits = 0;
    size_t n;

    /* Parse who: u, g, o, a, or default to 'a' */
    who_mask = 0;
    shift = 0;
    all_classes = 0;
    while(*src && str_chr("ugoa", *src) < 4) {
      switch(*src) {
        case 'u': who_mask |= 0700; shift = 6; break;
        case 'g': who_mask |= 0070; shift = 3; break;
        case 'o': who_mask |= 0007; shift = 0; break;
        case 'a': all_classes = 1; break;
      }
      src++;
    }
    if(all_classes || who_mask == 0) {
      who_mask = 0777;
      shift = 0;  /* Will apply to all classes */
    }

    /* Parse operator: +, -, = */
    op = *src;
    if(str_chr("=+-", op) == 3)
      return 0;
    src++;

    /* Parse permissions: r, w, x */
    n = scan_rwx(src, &bits);
    src += n;

    /* Apply operation */
    if(all_classes || who_mask == 0777) {
      /* Apply to all classes */
      uint16 all_bits = (bits << 6) | (bits << 3) | bits;
      switch(op) {
        case '+':
          /* Add permissions = clear bits from mask */
          *umask &= ~all_bits;
          break;
        case '-':
          /* Remove permissions = set bits in mask */
          *umask |= all_bits;
          break;
        case '=':
          /* Set exactly = clear all bits, then set complementary bits */
          *umask = 0;
          *umask |= (~all_bits & 0777);
          break;
      }
    } else {
      /* Apply to specific class */
      uint16 shifted_bits = bits << shift;
      switch(op) {
        case '+':
          /* Add permissions = clear bits from mask */
          *umask &= ~shifted_bits;
          break;
        case '-':
          /* Remove permissions = set bits in mask */
          *umask |= shifted_bits;
          break;
        case '=':
          /* Set exactly = clear all bits for class, then set complementary bits */
          *umask &= ~who_mask;
          *umask |= (who_mask ^ shifted_bits);
          break;
      }
    }

    /* Check for comma separator */
    if(*src == ',')
      src++;
    else if(*src)
      break;
  }

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
