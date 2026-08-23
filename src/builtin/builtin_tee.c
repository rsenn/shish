#include "../builtin.h"
#include "../fdtable.h"
#include "../../lib/shell.h"
#include "../../lib/open.h"
#include "../../lib/alloc.h"
#include <signal.h>
#include <unistd.h>

const char help_tee[] = "    Copy standard input to standard output and to each FILE.\n"
                        "\n"
                        "    -a              append to FILEs instead of overwriting them\n"
                        "    -i              ignore the SIGINT signal while copying\n"
                        "    file            file(s) to also write standard input to\n";

/* writes all of buf[0..len) to fd, looping over short writes.
 * Returns 0 on success, -1 on a real write(2) failure.
 * ----------------------------------------------------------------------- */
static int
full_write(int fd, const char* buf, size_t len) {
  while(len) {
    ssize_t n = write(fd, buf, len);

    if(n <= 0)
      return -1;

    buf += n;
    len -= n;
  }

  return 0;
}

int
builtin_tee(int argc, char* argv[]) {
  int c, append = 0, ignore_int = 0, ret = 0, i, nfiles;
  int* fds = NULL;
  char buf[4096];
  int n;
  void (*old_int)(int) = NULL;

  while((c = shell_getopt(argc, argv, "ai")) > 0) {
    switch(c) {
      case 'a': append = 1; break;
      case 'i': ignore_int = 1; break;
      default: builtin_invopt(argv); return 1;
    }
  }

  nfiles = argc - shell_optind;

  if(nfiles > 0) {
    fds = alloc(nfiles * sizeof(int));

    for(i = 0; i < nfiles; i++) {
      char* path = argv[shell_optind + i];

      fds[i] = append ? open_append(path) : open_trunc(path);

      if(fds[i] == -1) {
        builtin_error(argv, path);
        ret = 1;
      }
    }
  }

  if(ignore_int)
    old_int = signal(SIGINT, SIG_IGN);

  while((n = buffer_get_until(fd_in->r, buf, sizeof(buf), "", 0)) > 0) {
    buffer_put(fd_out->w, buf, n);
    buffer_flush(fd_out->w);

    for(i = 0; i < nfiles; i++) {
      if(fds[i] != -1 && full_write(fds[i], buf, n) == -1) {
        builtin_error(argv, argv[shell_optind + i]);
        ret = 1;
        fds[i] = -1;
      }
    }
  }

  if(n < 0) {
    builtin_error(argv, "-");
    ret = 1;
  }

  if(ignore_int)
    signal(SIGINT, old_int);

  for(i = 0; i < nfiles; i++) {
    if(fds[i] != -1)
      close(fds[i]);
  }

  alloc_free(fds);
  return ret;
}
