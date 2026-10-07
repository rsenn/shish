#include <sys/types.h>

/* libarchive compiles filter_fork_posix.c to nothing without fork/vfork/posix_spawn;
 * these stand in for it, so filters that run a program ("lzop") report an error.
 * ----------------------------------------------------------------------- */
int
__archive_create_child(const char* cmd, int* child_stdin, int* child_stdout, pid_t* out_child) {
  (void)cmd, (void)child_stdin, (void)child_stdout, (void)out_child;
  return -1;
}

void
__archive_check_child(int in, int out) {
  (void)in, (void)out;
}
