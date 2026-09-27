/* lib/utf8 unit test (code point functions); prints "<what>: OK|FAIL", exits non-zero on any FAIL */
#include "../lib/utf8.h"
#include <stdio.h>
#include <string.h>

static int failed;

static void
check(int ok, const char* what) {
  printf("%s: %s\n", what, ok ? "OK" : "FAIL");
  failed |= !ok;
}

static int
dec(const char* s, size_t n, unsigned* cp) {
  return u8decode(s, n, cp);
}

int
main(void) {
  unsigned cp = 0;
  char b[8];
  unsigned r;

  check(dec("A", 1, &cp) == 1 && cp == 'A', "ASCII decodes to itself");
  check(dec("\xc3\xa9", 2, &cp) == 2 && cp == 0xe9, "two-byte sequence (e acute)");
  check(dec("\xe2\x82\xac", 3, &cp) == 3 && cp == 0x20ac, "three-byte sequence (euro)");
  check(dec("\xf0\x9f\x98\x80", 4, &cp) == 4 && cp == 0x1f600, "four-byte sequence (emoji)");
  check(dec("", 0, &cp) == 0, "empty input is 0");
  check(dec("\x80", 1, &cp) == -1, "a lone continuation byte is invalid");
  check(dec("\xc0\x80", 2, &cp) == -1 && dec("\xc1\xbf", 2, &cp) == -1, "overlong two-byte forms are invalid");
  check(dec("\xe0\x80\x80", 3, &cp) == -1, "an overlong three-byte form is invalid");
  check(dec("\xed\xa0\x80", 3, &cp) == -1, "a surrogate (U+D800) is invalid");
  check(dec("\xf4\x90\x80\x80", 4, &cp) == -1 && dec("\xf5\x80\x80\x80", 4, &cp) == -1, "above U+10FFFF is invalid");
  check(dec("\xff", 1, &cp) == -1 && dec("\xfe", 1, &cp) == -1, "0xfe/0xff never start a sequence");
  check(dec("\xc3\x28", 2, &cp) == -1, "a bad continuation byte is invalid");
  check(dec("\xe2\x82", 2, &cp) == -2 && dec("\xc3", 1, &cp) == -2 && dec("\xf0\x9f\x98", 3, &cp) == -2, "a cut-off sequence is -2, not invalid");
  check(dec("\xe2\x28", 2, &cp) == -1, "a cut-off sequence with a bad byte is invalid, not -2");
  check(dec("\xc3\xa9x", 3, &cp) == 2, "decoding stops after one character");

  check(u8encode(b, 'A') == 1 && b[0] == 'A', "encode ASCII");
  check(u8encode(b, 0xe9) == 2 && !memcmp(b, "\xc3\xa9", 2), "encode two bytes");
  check(u8encode(b, 0x20ac) == 3 && !memcmp(b, "\xe2\x82\xac", 3), "encode three bytes");
  check(u8encode(b, 0x1f600) == 4 && !memcmp(b, "\xf0\x9f\x98\x80", 4), "encode four bytes");
  check(u8encode(b, 0xd800) == 0 && u8encode(b, 0x110000) == 0, "surrogates and values above U+10FFFF are refused");

  for(r = 0, cp = 0; r < 0x110000; r += 0x111) {
    unsigned back;

    if(r >= 0xd800 && r <= 0xdfff)
      continue;

    if(u8encode(b, r) <= 0 || u8decode(b, 4, &back) <= 0 || back != r)
      cp = 1;
  }
  check(cp == 0, "encode then decode returns the code point, across the range");

  check(u8charlen("\xc3\xa9", 2) == 2 && u8charlen("\xc3", 1) == 1 && u8charlen("\xff", 1) == 1 && u8charlen("", 0) == 0, "charlen: valid length, else 1, 0 when empty");
  check(u8count("a\xc3\xa9\xe2\x82\xac", 6) == 3, "count characters, not bytes");
  check(u8count("a\xff\xc3", 3) == 3, "each invalid or cut-off byte counts as one character");
  check(u8skip("a\xc3\xa9z", 4, 2) == 3 && u8skip("a\xc3\xa9z", 4, 9) == 4 && u8skip("abc", 3, 0) == 0, "skip k characters, clamped to the input");

  {
    static const unsigned short pair[] = {'a', 0xd83d, 0xde00, 0x20ac};
    static const unsigned short lone[] = {'x', 0xd800, 'y'};
    size_t used = 0;

    check(u8fromu16(b, 8, pair, 4, &used) == 8 && used == 4 && !memcmp(b, "a\xf0\x9f\x98\x80\xe2\x82\xac", 8), "UTF-16 with a surrogate pair converts to UTF-8");
    check(u8fromu16(b, 5, pair, 4, &used) == 5 && used == 3, "conversion stops before a character that does not fit");
    check(u8fromu16(b, 8, lone, 3, &used) == 5 && !memcmp(b, "x\xef\xbf\xbdy", 5), "a lone surrogate becomes U+FFFD");
  }

  check(u8locale("en_US.UTF-8", 0, 0) == 1 && u8locale(0, "C.utf8", 0) == 1 && u8locale(0, 0, "de_DE.UTF-8@euro") == 1 && u8locale(0, 0, "en_US.utf-8") == 1, "UTF-8 codesets are recognised in any spelling");
  check(u8locale("C", 0, "en_US.UTF-8") == 0 && u8locale("", "POSIX", "en_US.UTF-8") == 0, "the first non-empty variable decides");
  check(u8locale("", "", "en_US.UTF-8") == 1 && u8locale(0, 0, 0) == 0 && u8locale("en_US.ISO-8859-1", 0, 0) == 0 && u8locale("C.UTF-88", 0, 0) == 0, "an empty or missing variable falls through; other codesets are not UTF-8");

  check(text_charlen(0, "\xc3\xa9", 2) == 1 && text_charlen(1, "\xc3\xa9", 2) == 2, "text_charlen follows the mode");
  check(text_charcount(0, "a\xc3\xa9", 3) == 3 && text_charcount(1, "a\xc3\xa9", 3) == 2, "text_charcount follows the mode");
  check(text_charskip(0, "a\xc3\xa9z", 4, 2) == 2 && text_charskip(1, "a\xc3\xa9z", 4, 2) == 3 && text_charskip(0, "ab", 2, 9) == 2, "text_charskip follows the mode and clamps");

  return failed;
}
