#if GREP_USE_SYSTEM_REGEX
#include <regex.h>
#else
#include "../../../text/dfa.h"
#endif
#include "../../builtin.h"
#include "../../fdtable.h"
#include "../../term.h"
#include "../../../lib/fmt.h"
#include "../../../lib/shell.h"
#include "../../../lib/str.h"
#include "../../../lib/open.h"

#if GREP_USE_SYSTEM_REGEX
#define RE_FREE(re) regfree(re)
#else
#define RE_FREE(re) dfa_free(re)
#endif

const char help_grep[] = "    Search files for patterns.\n"
                    "\n"
                    "    -E              use Extended Regular Expressions (ERE)\n"
                    "    -v              select non-matching lines\n"
                    "    -n              precede each line by its line number\n"
                    "    -q              quiet (exit 0 on match, no output)\n"
                    "    -c              print a count of matching lines instead of the lines\n"
                    "    pattern         regular expression pattern\n"
                    "    file            file to search; '-' or omitted means stdin\n";

int
builtin_grep(int argc, char* argv[]) {
  int c, ret = 1, extended = 0, invert = 0, show_lineno = 0, quiet = 0, count_only = 0;
  char* pattern = NULL;
  char* arg;
#if GREP_USE_SYSTEM_REGEX
  regex_t re;
#else
  struct dfa re = {0};
#endif

  while((c = shell_getopt(argc, argv, "Evnqc")) > 0) {
    switch(c) {
      case 'E': extended = 1; break;
      case 'v': invert = 1; break;
      case 'n': show_lineno = 1; break;
      case 'q': quiet = 1; break;
      case 'c': count_only = 1; break;
      default: builtin_invopt(argv); return 2;
    }
  }

  if(argv[shell_optind] == NULL) {
    builtin_error(argv, "no pattern given");
    return 2;
  }

  pattern = argv[shell_optind++];

#if GREP_USE_SYSTEM_REGEX
  if(regcomp(&re, pattern, extended ? REG_EXTENDED : 0) != 0) {
    builtin_error(argv, "invalid regular expression");
    return 2;
  }
#else
  if(dfa_compile(&re, pattern, str_len(pattern), extended ? DFA_ERE : 0) != DFA_OK) {
    builtin_error(argv, "invalid regular expression");
    return 2;
  }
#endif

  if(argv[shell_optind] == NULL) {
    argv[shell_optind] = "-";
    argc++;
  }

  while((arg = argv[shell_optind])) {
    char buf[1024], rbuf[1024];
    buffer inb, *in;
    ssize_t r;
    unsigned long lineno = 1;
    unsigned long matchcount = 0;
    /* argc (not argv[shell_optind+1]): when no file operand was
       given, argv[shell_optind] was just set to "-" in place of the
       NULL terminator, so the slot after it is past the real array
       and unsafe to read. */
    int multiple_files = (shell_optind + 1 < argc);

    if(!str_diff(arg, "-")) {
      in = fd_in->r;
    } else {
      in = &inb;
      if(buffer_mmapread(in, arg)) {
        int rfd = open_read(arg);
        if(rfd == -1) {
          builtin_error(argv, arg);
          goto next_file;
        }
        buffer_init(in, &buffer_op_read, rfd, rbuf, sizeof(rbuf));
      }
    }

    for(;;) {
      if((r = buffer_get_until(in, buf, sizeof(buf) - 1, "\n", 1)) <= 0)
        break;

      buf[r] = '\0';
      if(r > 0 && buf[r - 1] == '\n')
        buf[--r] = '\0';
      if(r > 0 && buf[r - 1] == '\r')
        buf[--r] = '\0';

#if GREP_USE_SYSTEM_REGEX
      int matched = (regexec(&re, buf, 0, NULL, 0) == 0);
#else
      int matched = dfa_test(&re, buf, (size_t)r);
#endif
      if(invert) matched = !matched;

      if(matched) {
        ret = 0;
        if(quiet) {
          RE_FREE(&re);
          return 0;
        }

        if(count_only) {
          matchcount++;
        } else {
          if(multiple_files) {
            buffer_puts(fd_out->w, arg);
            buffer_puts(fd_out->w, ":");
          }

          if(show_lineno) {
            char lbuf[32];
            size_t ln = fmt_ulong(lbuf, lineno);
            buffer_put(fd_out->w, lbuf, ln);
            buffer_puts(fd_out->w, ":");
          }

          buffer_puts(fd_out->w, buf);
          buffer_putnlflush(fd_out->w);
        }
      }

      lineno++;
    }

    if(count_only) {
      char cbuf[32];
      size_t cn = fmt_ulong(cbuf, matchcount);

      if(multiple_files) {
        buffer_puts(fd_out->w, arg);
        buffer_puts(fd_out->w, ":");
      }

      buffer_put(fd_out->w, cbuf, cn);
      buffer_putnlflush(fd_out->w);
    }

  next_file:
    if(++shell_optind == argc)
      break;
  }

  RE_FREE(&re);
  return ret;
}

