#include "../builtin.h"
#include "../term.h"
#include "../tree.h"
#include "../prompt.h"
#include "../var.h"
#include "../expand.h"
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

extern union node* functions;

/* reserved words that start a construct or a command list */
static const char* const term_complete_keywords[] = {
    "case", "do", "elif", "else", "for", "function", "if", "then", "until", "while", NULL};

/* words after which another command name follows: "if <cmd>", "{ <cmd>" */
static const char* const term_complete_leaders[] = {
    "{", "!", "if", "then", "elif", "else", "while", "until", "do", NULL};

static int
term_complete_is_delim(char c) {
  return c == ' ' || c == '\t' || c == ';' || c == '&' || c == '|' || c == '(' || c == '`';
}

/* is the word starting at 'start' in command position?
 *
 *   "ec|"   "a; ec|"   "x | ec|"   "(ec|"   "if ec|"   "{ ec|"
 * ----------------------------------------------------------------------- */
static int
term_complete_is_command(unsigned long start) {
  const char* s = term_cmdline.s;
  unsigned long e = start, b;
  unsigned int i;

  while(e > 0 && (s[e - 1] == ' ' || s[e - 1] == '\t'))
    e--;

  if(e == 0 || (s[e - 1] != ' ' && term_complete_is_delim(s[e - 1])))
    return 1;

  for(b = e; b > 0 && !term_complete_is_delim(s[b - 1]); b--)
    ;

  for(i = 0; term_complete_leaders[i]; i++)
    if(str_len(term_complete_leaders[i]) == e - b && !str_diffn(term_complete_leaders[i], &s[b], e - b))
      return 1;

  return 0;
}

/* adds 'name' to the candidate list unless already there, narrowing the
 * common prefix; a candidate that does not start with 'base' is ignored.
 *
 *   const char*  name    candidate
 *   stralloc*    base    prefix typed so far
 *   stralloc*    common  longest prefix shared by all candidates so far
 *   char***      names   candidate array
 *   unsigned*    nmatch  number of candidates
 *   unsigned*    nalloc  allocated size of *names
 * ----------------------------------------------------------------------- */
static void
term_complete_add(const char* name,
                  const stralloc* base,
                  stralloc* common,
                  char*** names,
                  unsigned int* nmatch,
                  unsigned int* nalloc) {
  unsigned long nlen = str_len(name), j;

  if(nlen < base->len || str_diffn(name, base->s, base->len))
    return;

  for(j = 0; j < *nmatch; j++)
    if(!str_diff((*names)[j], name))
      return;

  if(++*nmatch == 1) {
    stralloc_copys(common, name);
  } else {
    j = 0;

    while(j < common->len && j < nlen && common->s[j] == name[j])
      j++;

    common->len = j;
  }

  if(*nmatch > *nalloc) {
    *nalloc = *nalloc ? *nalloc * 2 : 16;
    *names = alloc_re(*names, *nalloc * sizeof(char*));
  }

  (*names)[*nmatch - 1] = str_dup(name);
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
#ifdef HAVE_WINSIZE
  ncols = term_size.ws_col / colwidth;
#else
  ncols = 80 / colwidth; /* no TIOCGWINSZ: assume a stock 80-column terminal */
#endif

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

/* minimal filename tab-completion; in command position (first word of a
 * simple command) also completes reserved words, builtins and functions.
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
  stralloc dir, base, common, realdir;
  DIR* dp;
  struct dirent* de;
  unsigned int nmatch = 0;
  char** names = NULL;
  unsigned int nalloc = 0;
  unsigned int nfile;

  start = term_pos;

  while(start > 0 && !term_complete_is_delim(term_cmdline.s[start - 1]))
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
  stralloc_init(&realdir);

  if(dlen)
    stralloc_catb(&dir, word, dlen);
  else
    stralloc_cats(&dir, ".");

  stralloc_nul(&dir);
  stralloc_catb(&base, bstart, blen);
  stralloc_nul(&base);

  /* resolve a leading tilde-prefix in the typed directory part against
     the real filesystem for opendir()/stat() purposes only -- what's
     actually shown/inserted on the command line (word/term_cmdline)
     never gets touched, so "~/doc<TAB>" completes against $HOME/doc*
     on disk but the line still reads "~/documents/" afterward,
     matching bash/readline. */
  {
    stralloc home;
    size_t prefixlen;

    stralloc_init(&home);

    if(expand_tilde_lookup(dir.s, dir.len, 0, &home, &prefixlen)) {
      stralloc_cat(&realdir, &home);
      stralloc_catb(&realdir, dir.s + prefixlen, dir.len - prefixlen);
    } else {
      stralloc_cat(&realdir, &dir);
    }

    stralloc_free(&home);
  }

  stralloc_nul(&realdir);

  if((dp = opendir(realdir.s))) {
    while((de = readdir(dp))) {
      /* only offer dotfiles once the user has actually typed a leading
         "." -- same convention every other shell's completion uses */
      if(!base.len && de->d_name[0] == '.')
        continue;

      term_complete_add(de->d_name, &base, &common, &names, &nmatch, &nalloc);
    }

    closedir(dp);
  }

  nfile = nmatch;

  /* first word of a command, and something typed: also offer the
     reserved words, builtins and functions (not for a bare TAB, which
     would list every one of them) */
  if(!dlen && base.len && term_complete_is_command(start)) {
    union node* f;
    struct builtin_cmd* b;

    for(i = 0; term_complete_keywords[i]; i++)
      term_complete_add(term_complete_keywords[i], &base, &common, &names, &nmatch, &nalloc);

    for(b = builtin_table; b->name; b++)
      term_complete_add(b->name, &base, &common, &names, &nmatch, &nalloc);

    for(f = functions; f; f = f->next)
      term_complete_add(f->nfunc.name, &base, &common, &names, &nmatch, &nalloc);
  }

  if(nmatch && common.len > base.len)
    for(i = base.len; i < common.len; i++)
      term_insertc(common.s[i]);

  if(nmatch == 1 && !nfile) {
    term_insertc(' ');
  } else if(nmatch == 1) {
    struct stat st;
    stralloc full;

    /* "dir"/"realdir" is "." when the word had no '/' of its own --
       that's only meaningful to opendir(), not as a path prefix
       (there's no separator to join it to "common" with), so build
       the stat() target from the directory part actually typed (if
       any) instead of always going through "realdir". Using
       "realdir" rather than the raw "word" here (unlike the comment
       below used to say) is what makes a tilde-prefixed directory
       part stat()-able at all. */
    stralloc_init(&full);

    if(dlen)
      stralloc_cat(&full, &realdir);

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
  stralloc_free(&realdir);
#endif
}
