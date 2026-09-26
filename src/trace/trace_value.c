#include "../trace.h"

#ifdef DEBUG_OUTPUT

extern const char* debug_nodes[];
extern const unsigned debug_nodes_count;

const char* const trace_eval_flags[9] = {"E_EXIT", "E_ROOT", "E_BQUOTE", "E_JCTL", "E_LIST", "E_FUNCTION", "E_LOOP", "E_PRINT", "E_DEBUG"};
const char* const trace_redir_flags[9] = {"R_IN", "R_OUT", "R_OPEN", "R_DUP", "R_HERE", "R_STRIP", "R_APPEND", "R_CLOBBER", "R_NOW"};
#include "../source.h"
#include "../tree.h"
#include "../../lib/alloc.h"
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

/* a string that is not NUL terminated (stralloc contents) */
void
trace_strn(const char* key, const char* val, unsigned long len) {
  char tmp[512];
  unsigned long n = len < sizeof(tmp) - 1 ? len : sizeof(tmp) - 1;
  unsigned long i;

  for(i = 0; i < n; i++)
    tmp[i] = val[i];

  tmp[n] = 0;
  trace_str(key, tmp);

  if(n < len)
    trace_put("~", 1);
}

void
trace_node(const char* key, union node* node) {
  char* s;

  if(!node) {
    trace_str(key, NULL);
    return;
  }

  s = tree_string(node);
  trace_str(key, s);
  alloc_free(s);
}

void
trace_nodes(const char* key, union node* list) {
  trace_open(key, "[");

  for(; list; list = list->next)
    trace_node(NULL, list);

  trace_close("]");
}

void
trace_loc(const char* key, const struct location* loc) {
  char buf[FMT_LOC + 1];

  buf[fmt_location(buf, *loc)] = 0;
  trace_str(key, buf);
}

void
trace_kind(const char* key, int node_id) {
  trace_raw(key, node_id >= 0 && (unsigned)node_id < debug_nodes_count ? debug_nodes[node_id] : "?");
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
