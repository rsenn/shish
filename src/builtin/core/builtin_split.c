#include "builtin_config.h"

#if BUILTIN_SPLIT

#include "../../builtin.h"
#include "../../fdtable.h"
#include "../../../lib/shell.h"
#include "../../../lib/alloc.h"
#include "../../../lib/byte.h"
#include "../../../lib/open.h"
#include "../../../lib/str.h"
#include "../../../lib/stralloc.h"
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>

#ifndef NAME_MAX
#define NAME_MAX 255
#endif

const char help_split[] = "    Split a file into pieces.\n"
                          "\n"
                          "    -l lines        lines per piece (default 1000)\n"
                          "    -b n[k|m]       bytes per piece instead (k: *1024, m: *1048576)\n"
                          "    -a length       letters in the suffix (default 2)\n"
                          "    file            file to split; '-' or omitted means stdin\n"
                          "    name            prefix of the pieces (default x): xaa, xab, ...\n";

/* "N", "Nk" or "Nm"; 0 on a bad size */
static unsigned long
split_size(const char* s) {
  unsigned long v = 0;
  size_t i = 0;

  if(!s[0])
    return 0;

  while(s[i] >= '0' && s[i] <= '9') {
    unsigned long d = (unsigned long)(s[i++] - '0');

    if(v > (ULONG_MAX - d) / 10)
      return 0;

    v = v * 10 + d;
  }

  if(i == 0)
    return 0;

  if(s[i] == 'k' || s[i] == 'K')
    v *= 1024, i++;
  else if(s[i] == 'm' || s[i] == 'M')
    v *= 1048576, i++;

  return s[i] ? 0 : v;
}

/* base-26 suffix number n as exactly len letters; 0 when n needs more */
static int
split_suffix(char* out, unsigned long n, unsigned long len) {
  unsigned long i;

  for(i = len; i > 0; i--) {
    out[i - 1] = (char)('a' + n % 26);
    n /= 26;
  }

  out[len] = '\0';
  return n == 0;
}

int
builtin_split(int argc, char* argv[]) {
  int c, in = 0, bytes = 0, out = -1, ret = 0;
  unsigned long count = 1000, alen = 2, piece = 0, left = 0;
  const char *file = 0, *prefix = "x";
  stralloc path;
  char suffix[32];
  char buf[8192];
  size_t plen, base;
  ssize_t n;

  while((c = shell_getopt(argc, argv, "l:b:a:")) > 0) {
    switch(c) {
      case 'l':
      case 'b':
        bytes = c == 'b';

        if(c == 'b' ? !(count = split_size(shell_optarg)) : (filter_opt_count(shell_optarg, &count) < 0 || !count)) {
          builtin_errmsg(argv, shell_optarg, c == 'b' ? "invalid number of bytes" : "invalid number of lines");
          return 1;
        }

        break;
      case 'a':
        if(filter_opt_count(shell_optarg, &alen) < 0 || !alen || alen >= sizeof(suffix)) {
          builtin_errmsg(argv, shell_optarg, "invalid suffix length");
          return 1;
        }

        break;
      default: builtin_invopt(argv); return 1;
    }
  }

  if(argc - shell_optind > 2) {
    builtin_errmsg(argv, argv[shell_optind + 2], "extra operand");
    return 1;
  }

  file = argv[shell_optind];

  if(file && argv[shell_optind + 1])
    prefix = argv[shell_optind + 1];

  /* the prefix's own name plus the suffix must fit a file name */
  plen = str_len(prefix);
  base = plen;

  while(base > 0 && prefix[base - 1] != '/')
    base--;

  if(plen - base + alen > NAME_MAX) {
    builtin_errmsg(argv, (char*)prefix, "file name too long");
    return 1;
  }

  if(file && !str_equal(file, "-") && (in = open(file, O_RDONLY)) == -1) {
    builtin_error(argv, (char*)file);
    return 1;
  }

  stralloc_init(&path);

  /* a piece is only created once there is data for it */
  for(;;) {
    const char* p;
    size_t avail;

    if(in == 0 && (!file || str_equal(file, "-"))) {
      /* the shell's own standard input: it may not be a real descriptor */
      buffer* b = fdtable[0]->r;

      if((n = buffer_feed(b)) < 0)
        break;

      if(n > (ssize_t)sizeof(buf))
        n = (ssize_t)sizeof(buf);

      byte_copy(buf, (size_t)n, buffer_PEEK(b));
      buffer_SEEK(b, (size_t)n);
    } else if((n = read(in, buf, sizeof(buf))) < 0) {
      if(errno == EINTR)
        continue;

      builtin_error(argv, (char*)file);
      ret = 1;
      break;
    }

    if(n == 0)
      break;

    p = buf;
    avail = (size_t)n;

    while(avail) {
      size_t take;

      if(out == -1) {
        if(!split_suffix(suffix, piece, alen)) {
          builtin_errmsg(argv, (char*)prefix, "output file suffixes exhausted");
          ret = 1;
          goto done;
        }

        path.len = 0;
        stralloc_cats(&path, prefix);
        stralloc_cats(&path, suffix);
        stralloc_nul(&path);

        if((out = open_trunc(path.s)) == -1) {
          builtin_error(argv, path.s);
          ret = 1;
          goto done;
        }

        piece++;
        left = count;
      }

      if(bytes) {
        take = avail < left ? avail : left;
        left -= take;
      } else {
        /* up to and including the line that completes the piece */
        size_t i = 0;

        while(i < avail && left) {
          if(p[i++] == '\n')
            left--;
        }

        take = i;
      }

      while(take) {
        ssize_t w = write(out, p, take);

        if(w < 0) {
          if(errno == EINTR)
            continue;

          builtin_error(argv, path.s);
          ret = 1;
          goto done;
        }

        p += w;
        avail -= (size_t)w;
        take -= (size_t)w;
      }

      if(!left) {
        close(out);
        out = -1;
      }
    }
  }

done:
  if(out != -1)
    close(out);

  if(in > 0)
    close(in);

  stralloc_free(&path);
  return ret;
}
#endif /* BUILTIN_SPLIT */
