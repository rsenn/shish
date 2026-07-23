#include "../term.h"
#include "../prompt.h"
#include "../var.h"
#include "../../lib/str.h"
#include "../../lib/alloc.h"
#include "../../lib/windoze.h"

#if !WINDOWS_NATIVE
#include <dirent.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#endif

#if !WINDOWS_NATIVE
/* reprints "prompt + current cmdline" from scratch (used after the
 * match listing below has scribbled a block of text underneath the
 * in-progress line) and repositions the cursor back where it was --
 * term_left() needs term_pos to already reflect where the cursor
 * visually is, which a raw buffer_put() of the line text does not
 * update itself. */
static void
term_complete_redraw(void) {
  const char* prompt;
  size_t plen;
  unsigned long tail;

  if(prompt_number == 1) {
    prompt = prompt_expansion.s;
    plen = prompt_expansion.len;
  } else {
    prompt = var_vdefault(prompt_var, ">", &plen);
  }

  if(prompt)
    buffer_put(term_output, prompt, plen);

  buffer_put(term_output, term_cmdline.s, term_cmdline.len);

  tail = term_cmdline.len - term_pos;
  term_pos = term_cmdline.len;
  buffer_flush(term_output);
  term_left(tail);
}

static int
term_complete_cmp(const void* a, const void* b) {
  return str_diff(*(char* const*)a, *(char* const*)b);
}

/* prints the list of completion candidates in a multi-column layout,
 * column-major (down each column before moving to the next), same
 * arrangement `ls`/bash use -- then redraws the in-progress line
 * underneath it since the listing has scrolled past it. */
static void
term_complete_list(char** names, unsigned int nmatch) {
  unsigned long maxlen = 0, colwidth, i;
  unsigned int ncols, nrows, row, col;

  qsort(names, nmatch, sizeof(char*), term_complete_cmp);

  for(i = 0; i < nmatch; i++) {
    unsigned long l = str_len(names[i]);

    if(l > maxlen)
      maxlen = l;
  }

  colwidth = maxlen + 2;
  ncols = term_size.ws_col / colwidth;

  if(ncols < 1)
    ncols = 1;

  nrows = (nmatch + ncols - 1) / ncols;

  buffer_puts(term_output, "\r\n");

  for(row = 0; row < nrows; row++) {
    for(col = 0; col < ncols; col++) {
      unsigned int idx = col * nrows + row;

      if(idx >= nmatch)
        continue;

      buffer_puts(term_output, names[idx]);

      /* pad out to the column width, but only if this row actually
       * continues into the next column -- no trailing spaces on the
       * last entry of a row */
      if(idx + nrows < nmatch) {
        unsigned long pad = colwidth - str_len(names[idx]);

        while(pad--)
          buffer_putc(term_output, ' ');
      }
    }

    buffer_puts(term_output, "\r\n");
  }

  buffer_flush(term_output);
  term_complete_redraw();
}
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
 * behavior. If there is more than one candidate, every match is also
 * printed in a column view below the line, again like bash.
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
  char** names = NULL;
  unsigned int nalloc = 0;

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

    if(nmatch > nalloc) {
      nalloc = nalloc ? nalloc * 2 : 16;
      names = alloc_re(names, nalloc * sizeof(char*));
    }

    names[nmatch - 1] = str_dup(de->d_name);
  }

  closedir(dp);

  if(nmatch && common.len > base.len)
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
  } else if(nmatch > 1) {
    term_complete_list(names, nmatch);
  }

  for(i = 0; i < nmatch; i++)
    alloc_free(names[i]);

  alloc_free(names);

done:
  stralloc_free(&dir);
  stralloc_free(&base);
  stralloc_free(&common);
#endif
}
