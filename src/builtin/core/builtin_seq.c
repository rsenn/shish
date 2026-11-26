#include "builtin_config.h"

#if BUILTIN_SEQ

#include "../../builtin.h"
#include "../../fdtable.h"
#include "../../../lib/fmt.h"
#include "../../../lib/scan.h"
#include "../../../lib/str.h"
#include "../../../lib/uint64.h"

const char help_seq[] = "    Print a sequence of integers.\n"
                        "\n"
                        "    -s sep          separate the numbers with sep (default: newline)\n"
                        "    -w              pad with zeros to equal width\n"
                        "\n"
                        "    first incr last first and incr default to 1; incr may be negative\n";

/* v, zero-padded after the sign to width columns */
static void
put_num(buffer* o, int64 v, size_t width) {
  char buf[FMT_ULONG + 1];
  const char* d = buf;
  size_t len = fmt_longlong(buf, v), i;

  if(*d == '-') {
    buffer_putc(o, '-');
    d++;
    len--;

    if(width)
      width--;
  }

  for(i = len; i < width; i++)
    buffer_putc(o, '0');

  buffer_put(o, d, len);
}

static int
num(const char* s, int64* v) {
  size_t n = str_len(s);

  return n && scan_longlong(s, v) == n ? 0 : -1;
}

int
builtin_seq(int argc, char* argv[]) {
  buffer* o = fd_out->w;
  const char* sep = "\n";
  int64 first = 1, incr = 1, last, i;
  size_t width = 0, m;
  int k = 1, n, pad = 0;
  char buf[FMT_ULONG + 1];

  while(k < argc && argv[k][0] == '-' && argv[k][1] && (argv[k][1] < '0' || argv[k][1] > '9')) {
    if(!str_diff(argv[k], "--")) {
      k++;
      break;
    }

    if(!str_diff(argv[k], "-w"))
      pad = 1;
    else if(!str_diffn(argv[k], "-s", 2)) {
      if(argv[k][2])
        sep = argv[k] + 2;
      else if(++k < argc)
        sep = argv[k];
      else
        return builtin_errmsg(argv, "-s", "option requires an argument");
    } else
      return builtin_errmsg(argv, argv[k], "invalid option");

    k++;
  }

  if((n = argc - k) < 1)
    return builtin_errmsg(argv, "operand", "missing");

  if(n > 3)
    return builtin_errmsg(argv, argv[k + 3], "extra operand");

  if(num(argv[k + n - 1], &last) || (n > 1 && num(argv[k], &first)) || (n > 2 && num(argv[k + 1], &incr)))
    return builtin_errmsg(argv, argv[k], "invalid integer argument");

  if(!incr)
    return builtin_errmsg(argv, argv[k + 1], "zero increment");

  if(pad) {
    width = fmt_longlong(buf, first);

    if((m = fmt_longlong(buf, last)) > width)
      width = m;
  }

  for(i = first; incr > 0 ? i <= last : i >= last; i += incr) {
    uint64 span = incr > 0 ? (uint64)last - (uint64)i : (uint64)i - (uint64)last;

    if(i != first)
      buffer_puts(o, sep);

    put_num(o, i, width);

    if(span < (incr > 0 ? (uint64)incr : -(uint64)incr))
      break;
  }

  if(incr > 0 ? first <= last : first >= last)
    buffer_putc(o, '\n');

  buffer_flush(o);
  return 0;
}

#endif /* BUILTIN_SEQ */
