#include "../trace.h"

#ifdef DEBUG_OUTPUT
#include "../../lib/fmt.h"
#include "../../lib/str.h"

/* "..." with \" \\ \n \t \r and \xNN escapes, so argv round-trips */
static void
trace_quoted(const char* s) {
  static const char hex[] = "0123456789abcdef";
  char esc[4];

  trace_put("\"", 1);

  for(; *s; s++) {
    unsigned char c = (unsigned char)*s;

    switch(c) {
      case '"': trace_put("\\\"", 2); break;
      case '\\': trace_put("\\\\", 2); break;
      case '\n': trace_put("\\n", 2); break;
      case '\t': trace_put("\\t", 2); break;
      case '\r': trace_put("\\r", 2); break;
      default:
        if(c < 0x20 || c == 0x7f) {
          esc[0] = '\\';
          esc[1] = 'x';
          esc[2] = hex[c >> 4];
          esc[3] = hex[c & 15];
          trace_put(esc, 4);
        } else {
          trace_put((const char*)&c, 1);
        }
    }
  }

  trace_put("\"", 1);
}

void
trace_raw(const char* key, const char* text) {
  trace_key(key);
  trace_put(text, str_len(text));
}

void
trace_str(const char* key, const char* val) {
  trace_key(key);

  if(val)
    trace_quoted(val);
  else
    trace_put("NULL", 4);
}

void
trace_int(const char* key, long val) {
  char num[32];

  trace_key(key);
  trace_put(num, fmt_long(num, val));
}

void
trace_hex(const char* key, unsigned long val) {
  static const char hex[] = "0123456789abcdef";
  char num[24];
  int i = sizeof(num);

  trace_key(key);

  do {
    num[--i] = hex[val & 15];
    val >>= 4;
  } while(val);

  num[--i] = 'x';
  num[--i] = '0';
  trace_put(num + i, sizeof(num) - i);
}

void
trace_argv(const char* key, char* const* argv) {
  trace_key(key);

  if(!argv) {
    trace_put("NULL", 4);
    return;
  }

  trace_put("[", 1);

  for(; *argv; argv++) {
    trace_quoted(*argv);

    if(argv[1])
      trace_put(", ", 2);
  }

  trace_put("]", 1);
}

/* names[i] is the name of bit i, 0 for a bit without a name; prints A|B or 0 */
void
trace_flags(const char* key, unsigned long bits, const char* const* names, unsigned int n) {
  unsigned int i, any = 0;

  trace_key(key);

  for(i = 0; i < n; i++) {
    if((bits & (1ul << i)) && names[i]) {
      if(any)
        trace_put("|", 1);

      trace_put(names[i], str_len(names[i]));
      any = 1;
    }
  }

  if(!any)
    trace_put("0", 1);
}
#endif /* DEBUG_OUTPUT */
