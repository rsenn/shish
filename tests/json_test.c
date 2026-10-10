/* src/json.h unit test; prints "<what>: OK|FAIL", exits non-zero on any FAIL */
#include "../lib/stralloc.h"
#include "../src/json.h"
#include <stdio.h>
#include <string.h>

static int failed;

/* run fn against a writer with the given indent and compare what it wrote */
static void
check(int indent, void (*fn)(struct json*), const char* want, const char* what) {
  stralloc sa;
  buffer b;
  struct json j;
  int ok;

  stralloc_init(&sa);
  buffer_tosa(&b, &sa);
  json_init(&j, &b, indent);
  fn(&j);
  buffer_flush(&b);

  ok = sa.len == strlen(want) && memcmp(sa.s, want, sa.len) == 0;
  printf("%s: %s\n", what, ok ? "OK" : "FAIL");

  if(!ok)
    printf("  want: %s\n  got:  %.*s\n", want, (int)sa.len, sa.s);

  failed |= !ok;
  stralloc_free(&sa);
}

static void
object(struct json* j) {
  json_open(j, '{');
  json_kstr(j, "kind", "word");
  json_kuint(j, "n", 3);
  json_khex(j, "flag", 0x1f);
  json_kstr(j, "none", NULL);
  json_close(j, '}');
}

static void
nested(struct json* j) {
  json_open(j, '{');
  json_key(j, "cmds");
  json_open(j, '[');
  json_open(j, '{');
  json_kuint(j, "a", 1);
  json_close(j, '}');
  json_open(j, '{');
  json_close(j, '}');
  json_close(j, ']');
  json_kuint(j, "z", 2);
  json_close(j, '}');
}

static void
empty(struct json* j) {
  json_open(j, '[');
  json_close(j, ']');
}

static void
escapes(struct json* j) {
  {
    const char* s = "a\"b\\c\n\r\t\001z\xc3\xa9";

    json_str(j, s, strlen(s));
  }
}

static void
ndjson(struct json* j) {
  int i;

  for(i = 0; i < 2; i++) {
    json_open(j, '{');
    json_kuint(j, "i", i);
    json_close(j, '}');
    buffer_putc(j->b, '\n');
  }
}

/* the flags are independent bits: bit 0 bare_keys, 1 single_quotes, 2 hex_numbers */
static int flag_bits;

static void
flags(struct json* j) {
  j->bare_keys = flag_bits & 1;
  j->single_quotes = (flag_bits >> 1) & 1;
  j->hex_numbers = (flag_bits >> 2) & 1;
}

static void
flag_object(struct json* j) {
  flags(j);
  json_open(j, '{');
  json_kstr(j, "s", "it's \"q\"");
  json_kuint(j, "n", 255);
  json_khex(j, "h", 255);
  json_kpair(j, "r", 10, 20);
  json_close(j, '}');
}

static void
flag_nested(struct json* j) {
  flags(j);
  nested(j);
}

static void
minified(struct json* j) {
  j->minify = 1;
  j->bare_keys = flag_bits & 1;
  j->hex_numbers = (flag_bits >> 2) & 1;
  nested(j);
}

static void
minified_object(struct json* j) {
  j->minify = 1;
  object(j);
}

int
main(void) {
  check(2, object, "{\n  \"kind\": \"word\",\n  \"n\": 3,\n  \"flag\": \"0x1f\",\n  \"none\": null\n}", "indented object");
  check(0, object, "{ \"kind\": \"word\", \"n\": 3, \"flag\": \"0x1f\", \"none\": null }", "compact object");
  check(2,
        nested,
        "{\n  \"cmds\": [\n    {\n      \"a\": 1\n    },\n    {\n    }\n  ],\n  \"z\": 2\n}",
        "nested containers, empty object, comma after a closed container");
  check(2, empty, "[\n]", "empty array breaks the line");
  check(0, escapes, "\"a\\\"b\\\\c\\n\\r\\t\\u0001z\xc3\xa9\"", "string escapes, UTF-8 passes through");
  check(0, ndjson, "{ \"i\": 0 }\n{ \"i\": 1 }\n", "two top-level documents need no state reset");

  flag_bits = 0;
  check(0, flag_object, "{ \"s\": \"it's \\\"q\\\"\", \"n\": 255, \"h\": \"0xff\", \"r\": [10,20] }", "no flags: plain JSON");
  flag_bits = 1;
  check(0, flag_object, "{ s: \"it's \\\"q\\\"\", n: 255, h: \"0xff\", r: [10,20] }", "bare_keys alone: only the keys change");
  flag_bits = 2;
  check(0, flag_object, "{ \"s\": 'it\\'s \"q\"', \"n\": 255, \"h\": \"0xff\", \"r\": [10,20] }", "single_quotes alone: ' escaped, \" not, hex stays a string");
  flag_bits = 4;
  check(0, flag_object, "{ \"s\": \"it's \\\"q\\\"\", \"n\": 0xff, \"h\": 0xff, \"r\": [0xa,0x14] }", "hex_numbers alone: every number, json_hex() is no longer a string");
  flag_bits = 7;
  check(0, flag_object, "{ s: 'it\\'s \"q\"', n: 0xff, h: 0xff, r: [0xa,0x14] }", "all three together");
  flag_bits = 7;
  check(2,
        flag_nested,
        "{\n  cmds: [\n    {\n      a: 0x1\n    },\n    {\n    }\n  ],\n  z: 0x2\n}",
        "the flags do not change the layout");

  flag_bits = 0;
  check(0, minified_object, "{\"kind\":\"word\",\"n\":3,\"flag\":\"0x1f\",\"none\":null}", "minify: no space after : or , nor inside { }");
  check(2, minified, "{\"cmds\":[{\"a\":1},{}],\"z\":2}", "minify overrides indent, an empty container is {}");
  flag_bits = 5;
  check(4, minified, "{cmds:[{a:0x1},{}],z:0x2}", "minify with bare_keys and hex_numbers");

  return failed;
}
