#include "../history.h"
#include "../term.h"
#include "../../lib/byte.h"
#include "../../lib/str.h"
#include "../../lib/windoze.h"

#if !WINDOWS_NATIVE
/* decodes history entry <k> (1 = newest) into <out>; 0 if there is none */
static int
term_search_entry(unsigned int k, stralloc* out) {
  stralloc_zero(out);

  if(k >= 1 && k <= history_session_count) {
    const char* s = history_session[(history_session_head + history_session_count - k) % history_session_cap];

    stralloc_copyb(out, s, str_len(s));
    return 1;
  } else if(k > history_session_count) {
    const char* line;
    size_t len;

    if(history_file_entry(k - history_session_count - 1, &line, &len)) {
      history_decode(line, len, out);
      return 1;
    }
  }

  return 0;
}

static int
term_search_contains(const stralloc* hay, const stralloc* needle) {
  size_t i;

  if(needle->len > hay->len)
    return 0;

  for(i = 0; i + needle->len <= hay->len; i++)
    if(!byte_diff(&hay->s[i], needle->len, needle->s))
      return 1;

  return 0;
}

/* first entry at or after <from> (going back in time) containing <query>;
 * 0 if none. the entry is left in <out> */
static unsigned int
term_search_find(const stralloc* query, unsigned int from, stralloc* out) {
  unsigned int k;

  for(k = from ? from : 1; term_search_entry(k, out); k++)
    if(term_search_contains(out, query))
      return k;

  return 0;
}

/* (reverse-i-search)`query': line */
static void
term_search_show(const stralloc* query, const stralloc* line, int failed) {
  term_erase();
  buffer_puts(term_output, failed ? "(failed reverse-i-search)`" : "(reverse-i-search)`");
  buffer_put(term_output, query->s, query->len);
  buffer_puts(term_output, "': ");
  buffer_put(term_output, line->s, line->len);
  buffer_flush(term_output);
}
#endif

/* incremental reverse history search (control-r).
 *
 *   <chars>     narrow the query; the newest entry containing it is shown
 *   ^R          next older match
 *   backspace   shorten the query
 *   ^G ^C       cancel, restore the line as it was
 *   other       accept: the entry becomes the edit line and the key is
 *               handled as usual (enter runs it, arrows edit it)
 * ----------------------------------------------------------------------- */
void
term_search(void) {
#if !WINDOWS_NATIVE
  stralloc query, line, saved;
  unsigned int k = 0, saved_cursor = history_cursor;
  int failed = 0, cancel = 0;
  char c;

  stralloc_init(&query);
  stralloc_init(&line);
  stralloc_init(&saved);
  stralloc_copy(&saved, &term_cmdline);
  stralloc_copy(&line, &term_cmdline);

  term_search_show(&query, &line, 0);

  while(buffer_getc(&term_input, &c) > 0) {
    unsigned int m;
    stralloc found;

    if(c == 18) {
      if(!query.len)
        continue;
      m = 0;
      stralloc_init(&found);
      m = term_search_find(&query, k + 1, &found);
    } else if(c == 127 || c == '\b') {
      if(query.len)
        query.len--;
      stralloc_init(&found);
      m = query.len ? term_search_find(&query, 1, &found) : 0;
      if(!query.len) {
        k = 0;
        stralloc_copy(&line, &saved);
        failed = 0;
        stralloc_free(&found);
        term_search_show(&query, &line, 0);
        continue;
      }
    } else if(c == 7 || c == 3) {
      cancel = 1;
      break;
    } else if(c < 32 || c == '\033') {
      term_input.p--; /* hand the key back to term_read() */
      break;
    } else {
      stralloc_catb(&query, &c, 1);
      stralloc_init(&found);
      m = term_search_find(&query, k, &found);
    }

    if(m) {
      k = m;
      stralloc_copy(&line, &found);
      failed = 0;
    } else {
      failed = 1;
    }

    stralloc_free(&found);
    term_search_show(&query, &line, failed);
  }

  term_erase();

  if(cancel || !k) {
    history_cursor = saved_cursor;
    stralloc_copy(&term_cmdline, &saved);
  } else {
    if(saved_cursor == 0) {
      stralloc_free(&history_pending);
      stralloc_copy(&history_pending, &saved);
    }

    history_cursor = k;
    stralloc_copy(&term_cmdline, &line);
  }

  term_pos = term_cmdline.len;
  term_complete_redraw();

  stralloc_free(&query);
  stralloc_free(&line);
  stralloc_free(&saved);
#endif
}
