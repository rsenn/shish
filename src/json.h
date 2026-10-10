#ifndef JSON_H
#define JSON_H

#include "../lib/buffer.h"
#include "../lib/str.h"
#include "../lib/uint64.h"

/* header-only JSON writer: it tracks commas, newlines and indent, the caller only
 * says what comes next.
 *
 *   json_open(&j, '{');
 *   json_kstr(&j, "kind", "word");   ->  {
 *   json_kuint(&j, "n", 3);              "kind": "word",
 *   json_close(&j, '}');                 "n": 3
 *                                      }
 * ----------------------------------------------------------------------- */
struct json {
  buffer* b;
  int indent;         /* spaces per level; 0 = one line, items separated by ", " */
  int depth;          /* open containers */
  unsigned comma : 1; /* the next item needs a leading "," */
  unsigned key : 1;   /* a key was just written: its value follows without a separator */
  unsigned bare_keys : 1;     /* keys unquoted: kind: ... (JSON5) */
  unsigned single_quotes : 1; /* strings in '...' instead of "..." (JSON5) */
  unsigned hex_numbers : 1;   /* numbers as 0x1f, not 31 (JSON5) */
  unsigned minify : 1;        /* no whitespace at all, whatever indent says */
};

static inline void
json_init(struct json* j, buffer* b, int indent) {
  j->b = b;
  j->indent = indent;
  j->depth = 0;
  j->comma = 0;
  j->key = 0;
  j->bare_keys = 0;
  j->single_quotes = 0;
  j->hex_numbers = 0;
  j->minify = 0;
}

/* line break + indent for the current depth, one space when indent is 0, nothing when minify */
static inline void
json_break(struct json* j) {
  if(j->minify)
    return;

  if(j->indent > 0) {
    buffer_putc(j->b, '\n');
    buffer_putnspace(j->b, j->depth * j->indent);
  } else {
    buffer_putc(j->b, ' ');
  }
}

/* what precedes every item: nothing after a key, else "," when needed, then a break
 * (not at top level, where the caller owns the document boundary) */
static inline void
json_sep(struct json* j) {
  if(j->key) {
    j->key = 0;
    return;
  }

  if(j->comma)
    buffer_putc(j->b, ',');

  if(j->depth)
    json_break(j);
}

/* c is '{' or '[' */
static inline void
json_open(struct json* j, char c) {
  json_sep(j);
  buffer_putc(j->b, c);
  j->depth++;
  j->comma = 0;
}

/* c is '}' or ']'; always on a line of its own, an empty container included */
static inline void
json_close(struct json* j, char c) {
  j->depth--;
  json_break(j);
  buffer_putc(j->b, c);
  j->comma = j->depth > 0; /* a top-level value ends the document: the next starts clean */
}

/* k must not need escaping (it is a literal in the caller); with bare_keys it must also be
 * an identifier */
static inline void
json_key(struct json* j, const char* k) {
  json_sep(j);

  if(j->bare_keys) {
    buffer_puts(j->b, k);
    buffer_putc(j->b, ':');
  } else {
    buffer_putc(j->b, '"');
    buffer_puts(j->b, k);
    buffer_put(j->b, "\":", 2);
  }

  if(!j->minify)
    buffer_putc(j->b, ' ');

  j->key = 1;
  j->comma = 1;
}

static inline void
json_null(struct json* j) {
  json_sep(j);
  buffer_put(j->b, "null", 4);
  j->comma = 1;
}

/* a number: decimal, or 0x... with hex_numbers */
static inline void
json_num(struct json* j, uint64 v) {
  if(j->hex_numbers) {
    buffer_put(j->b, "0x", 2);
    buffer_putxlonglong(j->b, v);
  } else {
    buffer_putulonglong(j->b, v);
  }
}

static inline void
json_uint(struct json* j, uint64 v) {
  json_sep(j);
  json_num(j, v);
  j->comma = 1;
}

/* JSON has no hex literal, so it is the string "0x1f"; with hex_numbers it is the number 0x1f */
static inline void
json_hex(struct json* j, uint64 v) {
  json_sep(j);

  if(j->hex_numbers) {
    json_num(j, v);
  } else {
    buffer_put(j->b, "\"0x", 3);
    buffer_putxlonglong(j->b, v);
    buffer_putc(j->b, '"');
  }

  j->comma = 1;
}

/* "k": [a,b] -- a pair on one line, whatever the indent */
static inline void
json_kpair(struct json* j, const char* k, uint64 a, uint64 b) {
  json_key(j, k);
  j->key = 0;
  buffer_putc(j->b, '[');
  json_num(j, a);
  buffer_putc(j->b, ',');
  json_num(j, b);
  buffer_putc(j->b, ']');
}

/* escapes the quote (" or ', with single_quotes), \\ \n \r \t and the other control characters as \u00XX;
 * bytes >= 0x80 pass through */
static inline void
json_str(struct json* j, const char* s, size_t n) {
  static const char hex[] = "0123456789abcdef";
  char q = j->single_quotes ? '\'' : '"';

  json_sep(j);
  buffer_putc(j->b, q);

  for(; n; n--, s++) {
    unsigned char c = (unsigned char)*s;

    if(c == (unsigned char)q || c == '\\') {
      buffer_putc(j->b, '\\');
      buffer_putc(j->b, c);
    } else if(c == '\n') {
      buffer_put(j->b, "\\n", 2);
    } else if(c == '\r') {
      buffer_put(j->b, "\\r", 2);
    } else if(c == '\t') {
      buffer_put(j->b, "\\t", 2);
    } else if(c < 0x20) {
      buffer_put(j->b, "\\u00", 4);
      buffer_putc(j->b, hex[c >> 4]);
      buffer_putc(j->b, hex[c & 15]);
    } else {
      buffer_putc(j->b, c);
    }
  }

  buffer_putc(j->b, q);
  j->comma = 1;
}

/* key + value in one call; a NULL s is null */
static inline void
json_kstr(struct json* j, const char* k, const char* s) {
  json_key(j, k);

  if(s)
    json_str(j, s, str_len(s));
  else
    json_null(j);
}

static inline void
json_kuint(struct json* j, const char* k, uint64 v) {
  json_key(j, k);
  json_uint(j, v);
}

static inline void
json_khex(struct json* j, const char* k, uint64 v) {
  json_key(j, k);
  json_hex(j, v);
}

#endif /* JSON_H */
