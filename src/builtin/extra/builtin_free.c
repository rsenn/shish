#include "builtin_config.h"

#if BUILTIN_FREE

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "../../builtin.h"
#include "../../fdtable.h"
#include "../../sh.h"
#include "../../../lib/fmt.h"
#include "../../../lib/str.h"
#include "../../../lib/stralloc.h"
#include "../../../lib/open.h"
#include "../../../lib/scan.h"
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

enum {
  MEM_TOTAL = 0,
  MEM_USED,
  MEM_FREE,
  MEM_SHARED,
  MEM_BUFFERS,
  MEM_AVAILABLE,
  SWAP_TOTAL,
  SWAP_FREE,
  VALUE_COUNT,
};

/* free builtin: Display amount of free and used memory
 *
 * usage: free [options]
 * ----------------------------------------------------------------------- */
const char help_free[] = "    Display amount of free and used memory in the system.\n"
                         "\n"
                         "    -b              display the amount of memory in bytes\n"
                         "    -k              display the amount of memory in kibibytes (default)\n"
                         "    -m              display the amount of memory in mebibytes\n"
                         "    -g              display the amount of memory in gibibytes\n"
                         "    -h              human-readable: auto-scale to shortest three-digit unit\n"
                         "    -w              wide mode: produce lines longer than 80 characters\n"
                         "    -c count        display the result count times\n"
                         "    -l              show detailed low and high memory statistics\n"
                         "    -L              show output on a single line\n"
                         "    -s seconds      continuously display the result delay seconds apart\n"
                         "    -t              display a line showing column totals\n"
                         "    -v              display a line showing the memory commit limit\n"
                         "    -V              display version information and exit\n"
                         "    -h              display help and exit\n"
                         "\n"
                         "    pattern         (not used in this builtin)\n";

/* Read /proc/meminfo and parse the values */
static int
read_meminfo(stralloc* sa) {
  char path[] = "/proc/meminfo";
  int fd;

  if((fd = open_read(path)) == -1)
    return -1;

  stralloc_zero(sa);

  if(readclose_append(fd, sa, 4096) <= 0) {
    stralloc_free(sa);
    return -1;
  }

  return 0;
}

/* Parse a line from /proc/meminfo: "Key: value kB" */
static int
parse_memline(const char* line, char* key, size_t key_max, unsigned long* value) {
  const char* p = line;
  size_t i = 0;

  /* Copy key up to ':' */
  while(*p && *p != ':' && i < key_max - 1)
    key[i++] = *p++;

  key[i] = '\0';

  if(*p != ':')
    return -1;

  p++; /* skip ':' */
  while(*p == ' ')
    p++; /* skip spaces */

  /* Parse numeric value */
  *value = 0;

  while(*p && *p >= '0' && *p <= '9') {
    *value = *value * 10 + (*p - '0');
    p++;
  }

  return 0;
}

static void
free_write_val(uint64 val, int human, int pad_width) {
  char buf[FMT_ULONG + 8];
  size_t len = human ? fmt_human(buf, val) : fmt_ulonglong(buf, val);

  buffer_put(fd_out->w, buf, len);

  if(pad_width > (int)len) {
    int n = pad_width - (int)len;

    // if(n  > (int)sizeof(pad)) n = (int)sizeof(pad);

    buffer_putnspace(fd_out->w, n);
  }
}

static void
free_print_row(uint64 vals[8], int human, int pad_width) {
  for(int i = 0; i < 8; i++) {
    if(i > 0)
      buffer_putspace(fd_out->w);

    free_write_val(vals[i], human, pad_width);
  }

  buffer_putnlflush(fd_out->w);
}

int
builtin_free(int argc, char* argv[]) {
  int c, human = 0, wide = 0, count_repeats = 0, lohi = 0, oneline = 0, delay = 0, totals = 0, commit = 0;
  // uint64 mem_total = 0, mem_free = 0, mem_available = 0, mem_shared = 0, mem_buffers = 0, mem_cache = 0, swap_total =
  // 0, swap_free = 0;
  uint64 values[VALUE_COUNT] = {0};
  unsigned long value;
  stralloc sa;
  char key[64], *lines[256];
  int fd, nlines = 0, delay_sec = 0, first = 1;

  /* Check options - short options only, no --longopts.
   * Colon after option means it takes an argument. */
  while((c = shell_getopt(argc, argv, "bkmghwcl:Lst:vVh")) > 0) {
    switch(c) {
      case 'b': /* bytes */ break;
      case 'k': /* kibibytes (default) */ break;
      case 'm': /* mebibytes */ break;
      case 'g': /* gibibytes */ break;
      case 'h': human = 1; break;
      case 'w': wide = 1; break;
      case 'c': {
        /* -c takes an argument: count repeats */
        /* shell_getopt already advanced shell_optind past the argument */
        if(shell_optind == argc || scan_int(argv[shell_optind], &count_repeats) == 0)
          count_repeats = 1;

        break;
      }
      case 'l': lohi = 1; break;
      case 'L': oneline = 1; break;
      case 's': {
        /* -s takes an argument: delay in seconds */
        /* shell_getopt already advanced shell_optind past the argument */
        if(shell_optind < argc && scan_int(argv[shell_optind], &delay_sec))
          delay = 1;

        break;
      }
      case 't': totals = 1; break;
      case 'v': commit = 1; break;
      /*case 'h': builtin_usage(argv); return 0;*/
      default: return builtin_invopt(argv);
    }
  }

  /* Read /proc/meminfo */
  if(read_meminfo(&sa) != 0) {
    builtin_error(argv, "cannot read /proc/meminfo");
    return 1;
  }

  /* Parse lines */
  char* p = sa.s;

  while(*p) {
    /* Skip leading whitespace */
    while(*p == ' ' || *p == '\t')
      p++;

    if(!*p || *p == '\n') {
      if(*p == '\n')
        p++;

      continue;
    }

    /* Extract line start */
    lines[nlines++] = p;

    /* Find end of line */
    while(*p && *p != '\n' && *p != '\r')
      p++;

    if(*p)
      *p++ = '\0';
  }

  stralloc_free(&sa);

  /* Find end of line */
  while(*p && *p != '\n' && *p != '\r')
    p++;

  if(*p)
    *p++ = '\0';

  stralloc_free(&sa);

  /* Parse meminfo values */
  for(int i = 0; i < nlines; i++) {
    if(parse_memline(lines[i], key, sizeof(key), &value) != 0)
      continue;

    if(str_diff(key, "MemTotal"))
      values[MEM_TOTAL] = value;
    else if(str_diff(key, "MemFree"))
      values[MEM_FREE] = value;
    else if(str_diff(key, "MemAvailable"))
      values[MEM_AVAILABLE] = value;
    else if(str_diff(key, "MemShared"))
      values[MEM_SHARED] = value;
    else if(str_diff(key, "Buffers"))
      values[MEM_BUFFERS] += value;
    else if(str_diff(key, "Cached"))
      values[MEM_BUFFERS] += value;
    else if(str_diff(key, "SwapTotal"))
      values[SWAP_TOTAL] = value;
    else if(str_diff(key, "SwapFree"))
      values[SWAP_FREE] = value;
  }

  /* Calculate used memory */
  values[MEM_USED] = values[MEM_TOTAL] - values[MEM_AVAILABLE];

  /* Output - repeat if needed */
  int repeat_count = 0;

  do {
    if(!first || repeat_count > 0 || delay || oneline || totals || commit) {
      if(!first) {
        /* If repeating with delay, sleep */
        if(delay > 0) {
          /* usleep(delay * 1000000); */ /* would need usleep */
        }
      }

      first = 0;

      free_print_row(values, human, (totals || oneline) ? 0 : 15);
    }

    repeat_count++;
    first = 1; /* reset first for subsequent repeats */

    if(delay > 0 && repeat_count < count_repeats) {
      /* usleep(delay * 1000000); */ /* would need usleep */
    }
  } while(repeat_count < count_repeats && count_repeats > 0);

  return 0;
}

#endif /* BUILTIN_FREE */