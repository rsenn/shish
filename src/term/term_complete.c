#include "../term.h"
#include "../../lib/str.h"
#include "../../lib/windoze.h"

#if !WINDOWS_NATIVE
#include <dirent.h>
#include <sys/stat.h>
#endif

/* minimal filename tab-completion.
 *
 * Finds the start of the word under the cursor (back to the previous
 * space/tab or the start of the line -- no quote/escape awareness,
 * this is deliberately simple), splits it into a directory part (the
 * text up to and including the last '/', or "." if there is none) and
 * a filename prefix, then lists that directory looking for entries
 * starting with the prefix. Any characters common to every match
 * beyond the prefix already typed are inserted via term_insertc() (so
 * the normal terminal redraw logic doesn't need duplicating here); if
 * that leaves exactly one candidate, a trailing '/' (directory) or
 * space (plain file) is appended too, same as bash/readline's minimal
 * behavior.
 * ----------------------------------------------------------------------- */
void
term_complete(void) {
#if !WINDOWS_NATIVE
  unsigned long start, wlen, dlen, blen, i;
  const char *word, *slash, *bstart;
  stralloc dir, base, common;
  DIR* dp;
  struct dirent* de;
  unsigned int nmatch = 0;

  start = term_pos;

  while(start > 0 && term_cmdline.s[start - 1] != ' ' && term_cmdline.s[start - 1] != '\t')
    start--;

  word = &term_cmdline.s[start];
  wlen = term_pos - start;

  slash = NULL;

  for(i = 0; i < wlen; i++)
    if(word[i] == '/')
      slash = &word[i];

  dlen = slash ? (unsigned long)(slash - word) + 1 : 0;
  bstart = slash ? slash + 1 : word;
  blen = wlen - dlen;

  stralloc_init(&dir);
  stralloc_init(&base);
  stralloc_init(&common);

  if(dlen)
    stralloc_catb(&dir, word, dlen);
  else
    stralloc_cats(&dir, ".");

  stralloc_nul(&dir);
  stralloc_catb(&base, bstart, blen);
  stralloc_nul(&base);

  if(!(dp = opendir(dir.s)))
    goto done;

  while((de = readdir(dp))) {
    unsigned long nlen = str_len(de->d_name);

    /* only offer dotfiles once the user has actually typed a leading
       "." -- same convention every other shell's completion uses */
    if(!base.len && de->d_name[0] == '.')
      continue;

    if(nlen < base.len || str_diffn(de->d_name, base.s, base.len))
      continue;

    nmatch++;

    if(nmatch == 1) {
      stralloc_copys(&common, de->d_name);
    } else {
      unsigned long j = 0;

      while(j < common.len && j < nlen && common.s[j] == de->d_name[j])
        j++;

      common.len = j;
    }
  }

  closedir(dp);

  if(nmatch && common.len > base.len) {
    for(i = base.len; i < common.len; i++)
      term_insertc(common.s[i]);

    if(nmatch == 1) {
      struct stat st;
      stralloc full;

      /* "dir" is "." when the word had no '/' of its own -- that's
         only meaningful to opendir(), not as a path prefix (there's
         no separator to join it to "common" with), so build the
         stat() target from the directory part actually typed (if
         any) instead of always going through "dir". */
      stralloc_init(&full);

      if(dlen)
        stralloc_catb(&full, word, dlen);

      stralloc_cat(&full, &common);
      stralloc_nul(&full);

      if(stat(full.s, &st) == 0 && S_ISDIR(st.st_mode))
        term_insertc('/');
      else
        term_insertc(' ');

      stralloc_free(&full);
    }
  }

done:
  stralloc_free(&dir);
  stralloc_free(&base);
  stralloc_free(&common);
#endif
}
