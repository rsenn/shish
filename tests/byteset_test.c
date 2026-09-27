/* lib/byteset unit test; prints "<what>: OK|FAIL", exits non-zero on any FAIL */
#include "../lib/byteset.h"
#include <stdio.h>

static int failed;

static void
check(int ok, const char* what) {
  printf("%s: %s\n", what, ok ? "OK" : "FAIL");
  failed |= !ok;
}

static int
members(const unsigned char* s) {
  int n = 0, c;

  for(c = byteset_next(s, 0); c >= 0; c = byteset_next(s, c + 1))
    n++;

  return n;
}

int
main(void) {
  unsigned char s[BYTESET_SIZE];

  byteset_zero(s);
  check(members(s) == 0, "a zeroed set is empty");

  byteset_add(s, 'a');
  byteset_add(s, 0);
  byteset_add(s, 255);
  check(byteset_has(s, 'a') && byteset_has(s, 0) && byteset_has(s, 255), "added bytes are members, including 0 and 255");
  check(!byteset_has(s, 'b') && members(s) == 3, "other bytes are not");

  byteset_zero(s);
  check(byteset_range(s, 'a', 'f') == 0 && members(s) == 6 && byteset_has(s, 'a') && byteset_has(s, 'f') && !byteset_has(s, 'g'), "a range is inclusive");
  check(byteset_range(s, 'z', 'a') == -1 && members(s) == 6, "a reversed range adds nothing and reports it");
  byteset_zero(s);
  check(byteset_range(s, 250, 255) == 0 && members(s) == 6 && byteset_has(s, 255), "a range may end at 255");

  byteset_zero(s);
  check(byteset_class(s, "digit", 5) == 1 && members(s) == 10 && byteset_has(s, '7'), "[:digit:] has the ten digits");
  check(byteset_class(s, "alpha", 5) == 1 && byteset_has(s, 'q') && byteset_has(s, 'Q'), "[:alpha:] adds to the set");
  check(byteset_class(s, "nonsense", 8) == 0 && byteset_class(s, "digit", 4) == 0, "an unknown or truncated class name is refused");

  byteset_zero(s);
  byteset_add(s, 'a');
  byteset_invert(s);
  check(!byteset_has(s, 'a') && members(s) == 255, "invert swaps members and non-members");

  byteset_zero(s);
  byteset_add(s, 'a');
  byteset_add(s, 'Z');
  byteset_add(s, '5');
  byteset_icase(s);
  check(byteset_has(s, 'A') && byteset_has(s, 'z') && members(s) == 5, "icase adds the other case of letters only");

  byteset_zero(s);
  byteset_add(s, 10);
  byteset_add(s, 200);
  check(byteset_next(s, 0) == 10 && byteset_next(s, 11) == 200 && byteset_next(s, 201) == -1, "next walks the members in order");

  return failed;
}
