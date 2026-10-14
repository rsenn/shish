/* lib/shell shell_getopt_long unit test; prints "<what>: OK|FAIL", exits non-zero on any FAIL */
#include "../../lib/shell.h"
#include "../../lib/str.h"
#include <stdio.h>

static int failed;
#define check(cond, what) \
  do { \
    int ok = (cond); \
    printf("%s: %s\n", (what), ok ? "OK" : "FAIL"); \
    failed |= !ok; \
  } while(0)

#define OPTS "ab:c"
#define LONG "all=a begin:=b color?=c verbose=v"

/* runs the parser over argv, returns the codes it gave as a string ("a", "b=x", "?") and the final optind */
static int last_ind;

static const char*
run(char* argv[]) {
  static char out[128];
  struct optstate st = {0};
  int argc = 0, c;
  size_t n = 0;

  while(argv[argc])
    argc++;

  while((c = shell_getopt_long_r(&st, argc, argv, OPTS, LONG)) > 0 || c == '?' || c == ':') {
    out[n++] = c == SHELL_OPT_HELP ? 'H' : (char)c;
    out[n++] = (char)('0' + st.err);

    if(st.arg) {
      out[n++] = '=';
      n += str_copy(out + n, st.arg);
    }

    out[n++] = ' ';
  }

  if(c == SHELL_OPT_HELP) {
    out[n++] = 'H';
    out[n++] = (char)('0' + st.err);
    out[n++] = ' ';
  }

  out[n] = 0;
  last_ind = st.ind;
  return out;
}

#define EQ(argv, want, what) \
  do { \
    const char* got = run(argv); \
    int ok = !str_diff(got, want); \
    printf("%s: %s%s%s\n", what, ok ? "OK" : "FAIL", ok ? "" : " got ", ok ? "" : got); \
    failed |= !ok; \
  } while(0)

int
main(void) {
  char* t1[] = {"x", "-a", "--all", "file", 0};
  char* t2[] = {"x", "--begin=1", "--begin", "2", "-b3", 0};
  char* t3[] = {"x", "--color", "--color=no", 0};
  char* t4[] = {"x", "--bogus", "--all", 0};
  char* t5[] = {"x", "--all=1", 0};
  char* t6[] = {"x", "--begin", 0};
  char* t7[] = {"x", "--", "--all", 0};
  char* t8[] = {"x", "-a", "--", "-c", 0};
  char* t9[] = {"x", "--help", 0};
  char* t10[] = {"x", "-Z", 0};
  char* t11[] = {"x", "--al", 0};
  char* t12[] = {"x", "-", "-a", 0};

  EQ(t1, "a0 a0 ", "short and long flags map to the same code");
  check(last_ind == 3, "the operand stops parsing (optind 3)");
  EQ(t2, "b0=1 b0=2 b0=3 ", "--name=V, --name V and -bV all give the argument");
  EQ(t3, "c0 c0=no ", "an optional argument only comes with '='");
  EQ(t4, "?1 a0 ", "an unknown long option is '?' (UNKNOWN) and parsing goes on");
  EQ(t5, "?3 ", "a flag with =value is NOARG");
  EQ(t6, ":2 ", "a missing required value is ':' (MISSING)");
  EQ(t7, "", "'--' ends the options");
  check(last_ind == 2, "optind is after '--'");
  EQ(t8, "a0 ", "'--' stops before a later -c");
  EQ(t9, "H4 ", "--help is known to every caller");
  EQ(t10, "?1 ", "an unknown short option is UNKNOWN");
  EQ(t11, "?1 ", "long names are not abbreviated");
  EQ(t12, "", "a lone '-' is an operand");

  return failed;
}
