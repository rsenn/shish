/* lib/str unit test; prints "<what>: OK|FAIL", exits non-zero on any FAIL */
#include "../../lib/str.h"
#include "../../lib/byte.h"
#include <stdio.h>

static int failed;

static int check(int ok, const char* what) { 
  printf("%s: %s\n", what, ok ? "OK" : "FAIL"); 
  failed |= !ok;
}

#define CHECK(condition) check((condition), #condition)

int
main(void) {
  char buf[64];

  /* TEST: size_t str_copy(out, in) */
  CHECK(str_copy(buf, "test") == 4);
  
  /* TEST: size_t str_copyn(out, in, max) */
  CHECK(str_copyn(buf, "blah", 3) == 3);
  CHECK(byte_equal(buf, 4, "bla\0"));

  /* TEST: int str_diff(a, b) */
  CHECK(str_diff("blah", "blah") == 0);
  CHECK(str_diff("blah", "test") != 0);

  /* TEST: int str_diffn(a, b, limit) */
  CHECK(str_diffn("blah", "bla", 3) == 0);
  CHECK(str_diffn("blah", "bla", 4) != 0);

  /* TEST: int str_case_diff(a, b) */
  CHECK(str_case_diff("BLAH", "blah") == 0);
  CHECK(str_case_diff("BLAH", "test") != 0);

  /* TEST: int str_case_diffn(a, b, limit) */
  CHECK(str_case_diffn("BLAH", "bla", 3) == 0);
  CHECK(str_case_diffn("BLAH", "bla", 4) != 0);

  /* TEST: size_t str_len(in) */
  CHECK(str_len("blah") == 4);
  CHECK(str_len("bla") == 3);
  CHECK(str_len("") == 0);
  
  /* TEST: char* str_ndup(in, len) */
  CHECK(str_equal(str_ndup("blah", 3), "bla"));
  CHECK(str_equal(str_ndup("blah", 4), "blah"));
  CHECK(str_equal(str_ndup("blah", 5), "blah"));
  CHECK(str_equal(str_ndup("", 1), ""));
  CHECK(str_equal(str_ndup("a", 0), ""));

  /* TEST: size_t str_chr(haystack, needle) */
  CHECK(str_chr("blahblah", 'a') == 2);
  CHECK(str_chr("blahblah", 'l') == 1);
  CHECK(str_chr("blahblah", 'b') == 0);
  CHECK(str_chr("blah", '-') == 4);
  CHECK(str_chr("blah", '\0') == 4);
  
  /* TEST: size_t str_rchr(haystack, needle) */
  CHECK(str_rchr("blahblah", 'a') == 6);
  CHECK(str_rchr("blahblah", 'l') == 5);
  CHECK(str_rchr("blahblah", 'b') == 4);
  CHECK(str_rchr("blah", '-') == 4);
  CHECK(str_rchr("blah", '\0') == 4);
  
  /* TEST: size_t str_rchrs(haystack, needles, nn) */
  CHECK(str_rchrs("blahbla", "hb", 2) == 4);
  CHECK(str_rchrs("blahbla", "hbl", 3) == 5);
  CHECK(str_rchrs("blahbla", "ahbl", 4) == 6);
  CHECK(str_rchrs("blah", "-.\n\0", 4) == 4);
  CHECK(str_rchrs("blah", "", 0) == 4);
  
  return failed;
}
