#include "../../lib/alloc.h"
#include "../../lib/mmap.h"
#include "../../lib/str.h"
#include "../../lib/windoze.h"
#include "../history.h"
#include "../sh.h"
#include <sys/stat.h>
#if WINDOWS_NATIVE
#include <windows.h>
#define PATH_MAX MAX_PATH
#else
#include <limits.h>
#endif

char** history_session;
unsigned int history_session_cap;
unsigned int history_session_count;
unsigned int history_session_head;

const char* history_file_map;
size_t history_file_len;

size_t* history_cache_off;
size_t* history_cache_len;
unsigned int history_cache_count;
unsigned int history_cache_cap;
size_t history_cache_scanpos;
int history_cache_exhausted;

unsigned int history_cursor;
stralloc history_pending;

/* candidate history file basenames, tried in order; whichever is found
 * first is used for both reading and appending. if none exist yet, the
 * first ("history") is used to create one on the first history_add. */
static const char* history_names[] = {".history", ".sh_history", ".bash_history", NULL};

char history_file_path[PATH_MAX + 1];

/* resolve $HOME/<candidate> into history_file_path, trying each
 * candidate in turn until one exists (or, if none do, defaulting to the
 * first). only ever stat()s -- never reads the file's contents. */
static void
history_resolve_path(void) {
  size_t hlen;
  unsigned int i;

  history_file_path[0] = '\0';

  hlen = str_copyn(history_file_path, sh_home, PATH_MAX);

  if(hlen >= PATH_MAX - 16)
    return;

  if(hlen && history_file_path[hlen - 1] != '/')
    history_file_path[hlen++] = '/';

  for(i = 0; history_names[i]; i++) {
    struct stat st;
    size_t nlen = str_copyn(&history_file_path[hlen], history_names[i], PATH_MAX - hlen);

    if(stat(history_file_path, &st) == 0) {
      (void)nlen;
      return;
    }
  }

  /* nothing existed: default to the first candidate for later writes */
  str_copyn(&history_file_path[hlen], history_names[0], PATH_MAX - hlen);
}

/* cheap: resolves the history file path and mmap()s it (an O(1)
 * syscall -- the kernel doesn't read any of it in until something
 * touches a page), but never scans it. startup cost does not depend on
 * history file size.
 * ----------------------------------------------------------------------- */
void
history_init(void) {
  history_resolve_path();

  history_file_map = mmap_read(history_file_path, &history_file_len);

  if(!history_file_map)
    history_file_len = 0;

  history_cache_scanpos = history_file_len;

  /* skip a single trailing newline, if any, so the backward scan's
     first step lands on the last real entry rather than an empty one */
  if(history_cache_scanpos && history_file_map[history_cache_scanpos - 1] == '\n')
    history_cache_scanpos--;

  history_session_resize();

  stralloc_init(&history_pending);
  history_cursor = 0;
}

/* releases the mmap and all session/cache memory.
 * ----------------------------------------------------------------------- */
void
history_shutdown(void) {
  unsigned int i;

  if(history_file_map && history_file_len)
    mmap_unmap(history_file_map, history_file_len);

  history_file_map = NULL;
  history_file_len = 0;

  for(i = 0; i < history_session_count; i++)
    alloc_free(history_session[(history_session_head + i) % history_session_cap]);

  alloc_free(history_session);
  history_session = NULL;
  history_session_cap = history_session_count = history_session_head = 0;

  alloc_free(history_cache_off);
  alloc_free(history_cache_len);
  history_cache_off = NULL;
  history_cache_len = NULL;
  history_cache_count = history_cache_cap = 0;
  history_cache_exhausted = 0;

  stralloc_free(&history_pending);
  history_cursor = 0;
}
