#include "../fdtable.h"
#include "../sh.h"

/* output help
 *
 * every letter here also works identically via the "set" builtin at
 * runtime (src/builtin/builtin_set.c owns the actual behavior behind
 * each one, and "-o name"/"+o name" reuses its set_longopts table
 * directly, so the two can't drift apart on *names*) -- "-c"/"-i"/"-s"
 * are startup-only and don't exist as "set" options at all.
 * ----------------------------------------------------------------------- */
void
sh_usage(void) {
  buffer_puts(fd_err->w, "usage: ");
  buffer_puts(fd_err->w, sh_name);
  buffer_puts(fd_err->w, " [options] [-c command_string [command_name [arg ...]]]\n");
  buffer_puts(fd_err->w, "       ");
  buffer_puts(fd_err->w, sh_name);
  buffer_puts(fd_err->w, " [options] [-s] [script] [arg ...]\n");
  buffer_puts(fd_err->w,
              "\n"
              "    -c command_string   run command_string instead of a script/stdin\n"
              "    -i                  force interactive mode, even without a terminal\n"
              "    -s                  read commands from standard input\n"
              "                        (implied if no script and no -c is given)\n"
              "    -a                  export every variable assignment\n"
              "    -e                  exit if a simple command fails\n"
              "    -f                  disable pathname expansion (globbing)\n"
              "    -h                  remember command locations as they're looked up\n"
              "    -m                  enable job control (monitor mode)\n"
              "    -n                  read commands but don't execute them\n"
              "    -p                  privileged mode: don't process $ENV\n"
              "    -u                  treat unset variables as an error on expansion\n"
              "    -x                  print each command before running it\n"
              "    -B                  enable brace expansion\n"
              "    -C                  don't let '>' clobber an existing file\n"
              "    -H                  enable history expansion ('!')\n"
              "    -o name / +o name   same as the matching letter option above, by name\n"
              "    +option             turn a letter option off instead of on\n"
              "    -                   end options (also turns -x off)\n"
              "    --                  end options\n");
  buffer_flush(fd_err->w);
}
