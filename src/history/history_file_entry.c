#include "../../lib/alloc.h"
#include "../history.h"

/* find the '\n' immediately before position <before> in the mapped
 * file, i.e. the largest index i < before with map[i] == '\n', or
 * <before> itself if there is none. mirrors byte_chr's "not found"
 * convention, just scanning backward -- libowfat's byte_rchr isn't
 * part of the vendored subset in lib/.
 * ----------------------------------------------------------------------- */
static size_t
rfind_nl(const char* map, size_t before) {
  size_t i = before;

  while(i > 0) {
    i--;

    if(map[i] == '\n')
      return i;
  }

  return before;
}

/* take one more step backward through the mapped history file,
 * extending history_cache_{off,len} by exactly one entry. does no
 * work (and touches no new memory) if the scan already reached the
 * start of the file.
 * ----------------------------------------------------------------------- */
static void
history_cache_extend(void) {
  size_t nl;
  size_t start, len;

  if(history_cache_exhausted || history_cache_scanpos == 0)
    history_cache_exhausted = 1;

  if(history_cache_exhausted)
    return;

  nl = rfind_nl(history_file_map, history_cache_scanpos);

  if(nl == history_cache_scanpos) {
    /* no newline found before the scan position: this is the first
       (oldest) line in the file */
    start = 0;
    len = history_cache_scanpos;
    history_cache_scanpos = 0;
    history_cache_exhausted = 1;
  } else {
    start = nl + 1;
    len = history_cache_scanpos - start;
    history_cache_scanpos = nl;
  }

  if(history_cache_count == history_cache_cap) {
    history_cache_cap = history_cache_cap ? history_cache_cap * 2 : 32;
    history_cache_off = alloc_re(history_cache_off, history_cache_cap * sizeof(size_t));
    history_cache_len = alloc_re(history_cache_len, history_cache_cap * sizeof(size_t));
  }

  history_cache_off[history_cache_count] = start;
  history_cache_len[history_cache_count] = len;
  history_cache_count++;
}

/* get the file entry <index> positions back from the newest (index 0
 * is the most recently written line). extends the backward-scan cache
 * one step at a time, exactly as far as needed to satisfy this call --
 * an index that was already visited costs nothing new to look up
 * again. returns 0 once <index> runs past the start of the file.
 * ----------------------------------------------------------------------- */
int
history_file_entry(unsigned int index, const char** line, size_t* len) {
  while(index >= history_cache_count) {
    if(history_cache_exhausted)
      return 0;

    history_cache_extend();
  }

  *line = history_file_map + history_cache_off[index];
  *len = history_cache_len[index];
  return 1;
}
