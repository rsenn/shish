#include "../expand.h"
#include "../tree.h"
#include "../sh.h"
#include "../../lib/alloc.h"
#include "../../lib/byte.h"

/* Brace expansion ("{a,b,c}" -> "a" "b" "c", combined across multiple
 * groups in the same word -- "{a,b}{1,2}" -> "a1" "a2" "b1" "b2").
 *
 * Only fires on a word that is entirely one unquoted literal chunk --
 * no $var/`cmd`/quoting mixed in ("{a,$X}" stays literal). An escaped
 * brace/comma ("\{", "\,") isn't distinguished from a real one; only
 * quoting suppresses expansion.
 * ----------------------------------------------------------------------- */

struct brace_list {
  char** v;
  size_t n;
  size_t a;
};

static void
brace_list_init(struct brace_list* bl) {
  bl->v = NULL;
  bl->n = 0;
  bl->a = 0;
}

static void
brace_list_add(struct brace_list* bl, const char* s, size_t len) {
  char* dup;

  if(bl->n >= bl->a) {
    bl->a = bl->a ? bl->a * 2 : 8;
    bl->v = alloc_re(bl->v, bl->a * sizeof(char*));
  }

  dup = alloc(len + 1);
  byte_copy(dup, len, s);
  dup[len] = 0;
  bl->v[bl->n++] = dup;
}

static void
brace_list_free(struct brace_list* bl) {
  size_t i;

  for(i = 0; i < bl->n; i++)
    alloc_free(bl->v[i]);

  alloc_free(bl->v);
}

/* finds the first "{...}" in [text,len) that is balanced and contains
 * at least one top-level comma:
 * - a "{foo}" with no comma isn't a valid group -- scanning continues
 *   past it, looking for a later one (matches bash).
 * - an unmatched '{' stops the search for good; the rest is literal.
 *
 *   const char*  text     word text to scan
 *   size_t       len      length of text
 *   size_t*      gopen    out: offset of the matched group's '{'
 *   size_t*      gclose   out: offset of the matched group's '}'
 * ----------------------------------------------------------------------- */
static int
find_valid_group(const char* text, size_t len, size_t* gopen, size_t* gclose) {
  size_t i = 0;

  while(i < len) {
    if(text[i] == '{') {
      size_t depth = 1, j = i + 1, close = len, k;
      int has_comma = 0;

      while(j < len) {
        if(text[j] == '{')
          depth++;
        else if(text[j] == '}') {
          depth--;

          if(depth == 0) {
            close = j;
            break;
          }
        }

        j++;
      }

      if(close == len) /* unmatched '{' -- nothing past here can expand */
        return 0;

      depth = 0;

      for(k = i + 1; k < close; k++) {
        if(text[k] == '{')
          depth++;
        else if(text[k] == '}')
          depth--;
        else if(text[k] == ',' && depth == 0) {
          has_comma = 1;
          break;
        }
      }

      if(has_comma) {
        *gopen = i;
        *gclose = close;
        return 1;
      }

      i = close + 1;
      continue;
    }

    i++;
  }

  return 0;
}

/* expands [text,len), appending every resulting string to *out. Always
 * appends at least one string -- [text,len) itself, unchanged, when it
 * contains no valid group at all (the recursion's base case, and also
 * how a top-level call reports "nothing to expand" to its caller). */
static void
expand_brace_text(const char* text, size_t len, struct brace_list* out) {
  size_t gopen, gclose, altstart, k, depth;
  struct brace_list tails;

  if(!find_valid_group(text, len, &gopen, &gclose)) {
    brace_list_add(out, text, len);
    return;
  }

  brace_list_init(&tails);
  expand_brace_text(text + gclose + 1, len - gclose - 1, &tails);

  altstart = gopen + 1;
  depth = 0;

  for(k = gopen + 1; k <= gclose; k++) {
    int is_sep = (k == gclose) || (text[k] == ',' && depth == 0);

    if(k < gclose) {
      if(text[k] == '{')
        depth++;
      else if(text[k] == '}')
        depth--;
    }

    if(is_sep) {
      size_t ti;

      for(ti = 0; ti < tails.n; ti++) {
        stralloc piece;

        stralloc_init(&piece);
        stralloc_catb(&piece, text, gopen);
        stralloc_catb(&piece, text + altstart, k - altstart);
        stralloc_cats(&piece, tails.v[ti]);
        brace_list_add(out, piece.s, piece.len);
        stralloc_free(&piece);
      }

      altstart = k + 1;
    }
  }

  brace_list_free(&tails);
}

/* rewrites one raw argument word, in isolation, into N sibling N_ARG
 * nodes (linked via ->next, caller relinks them into place) if it's
 * eligible and contains at least one valid brace group; returns NULL
 * (word untouched) otherwise. Never mutates the passed-in `arg`. */
static union node*
expand_brace_word(union node* arg) {
  union node* chunk;
  struct brace_list result;
  union node *head = NULL, **tail = &head;
  size_t i;

  if(!arg || arg->id != N_ARG || !(chunk = arg->narg.list))
    return NULL;

  if(chunk->id != N_ARGSTR || (chunk->nargstr.flag & S_TABLE) != S_UNQUOTED || chunk->next)
    return NULL;

  if(byte_chr(chunk->nargstr.stra.s, chunk->nargstr.stra.len, '{') >= chunk->nargstr.stra.len)
    return NULL;

  brace_list_init(&result);
  expand_brace_text(chunk->nargstr.stra.s, chunk->nargstr.stra.len, &result);

  if(result.n <= 1) {
    brace_list_free(&result);
    return NULL;
  }

  for(i = 0; i < result.n; i++) {
    union node* n = tree_newnode(N_ARG);
    union node* s = tree_newnode(N_ARGSTR);

    stralloc_init(&s->nargstr.stra);
    stralloc_cats(&s->nargstr.stra, result.v[i]);
    s->nargstr.flag = S_UNQUOTED | (chunk->nargstr.flag & S_GLOB);
    s->nargstr.loc = chunk->nargstr.loc;

    n->narg.list = s;

    *tail = n;
    tail = &n->next;
  }

  brace_list_free(&result);
  return head;
}

/* top-level entry point: rewrites `args` in place so that any
 * brace-eligible word becomes its N generated sibling words, returning
 * the (possibly different) resulting list.
 *
 * `args` must be a private, disposable copy the caller owns outright,
 * never the permanent parsed command tree -- expand_args.c
 * tree_copy()'s before calling this and tree_free()'s the result.
 * ----------------------------------------------------------------------- */
union node*
expand_brace_args(union node* args) {
  union node *arg, **link;

  if(!sh->opts.braceexpand)
    return args;

  for(link = &args; (arg = *link);) {
    union node* alts = expand_brace_word(arg);

    if(alts) {
      union node* rest = arg->next;

      arg->next = NULL;
      tree_free(arg);
      *link = alts;

      while(*link)
        link = &(*link)->next;

      *link = rest;
    } else {
      link = &arg->next;
    }
  }

  return args;
}
