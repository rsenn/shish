#include "../parse.h"
#include "../redir.h"
#include "../source.h"
#include "../tree.h"

/* parse a redirection word according to 3.7
 *
 * [n]<operator>word
 *
 * input operators: <, <<, <<-, <&, <>
 * output operators: >, >>, >|, >, <>
 *
 * store [n] in word_rfd, <operator> stuff in word_rflags,
 * and 'word' in word_rfile
 *
 * ----------------------------------------------------------------------- */
int
redir_parse(struct parser* p, int rf, int fd) {
  /* initialize fd to 0 for input, 1 for output */
  char c;

  if(fd == -1)
    fd = rf;

  /* get next character */
  source_peek(&c);
  stralloc_catc(&p->sa, c);
  if(source_next(&c) <= 0)
    return T_EOF;

  /* parse input redirection operator (3.7.1) */
  if(rf & R_IN) {
    switch(c) {
      /* << is here-doc (3.7.4) */
      case '<':
        rf |= R_HERE;
        stralloc_catb(&p->sa, &c, 1);

        if(source_next(&c) <= 0)
          return T_EOF;

        /* <<- means strip leading tabs and trailing whitespace */
        if(c == '-') {
          rf |= R_STRIP;
          stralloc_catc(&p->sa, c);
          source_skip();
        }

        /* do not subst delimiter for here-docs because
           here-docs are parsed before any expansion is done */
        p->flags |= P_NOSUBST;
        break;

      /* <& dups input file descriptor (3.7.5) */
      case '&':
        rf |= R_DUP;
        stralloc_catc(&p->sa, c);
        source_skip();
        break;

      /* <> opens file r/w (3.7.7) */
      case '>':
        rf |= R_OUT;
        stralloc_catc(&p->sa, c);
        source_skip();

      /* < opens input file */
      default: rf |= R_OPEN; break;
    }
  }
  /* parse output redirection operator (3.7.2) */
  if(rf & R_OUT) {
    switch(c) {
      /* >& is dup2() (3.7.6) */
      case '&':
        rf |= R_DUP;
        stralloc_catc(&p->sa, c);
        source_skip();
        break;

      /* >> is appending-mode (3.7.3) */
      case '>':
        rf |= R_APPEND | R_OPEN;
        stralloc_catc(&p->sa, c);
        source_skip();
        break;

      /* >| is no-clobbering-mode */
      case '|':
        rf |= R_CLOBBER;
        stralloc_catc(&p->sa, c);
        source_skip();

      /* > opens output file */
      default: rf |= R_OPEN; break;
    }
  }

  if(parse_gettok(p, P_DEFAULT) & (T_NAME | T_WORD)) {
    union node* node;

    node = tree_newnode(N_REDIR);
    node->nredir.flag = rf;
    node->nredir.fdes = fd;
    node->nredir.list = p->tree;
    p->tree = node;

    if(node->nredir.flag & R_HERE)
      redir_addhere(&node->nredir);

    p->tok = T_REDIR;
    return 1;
  }

  return 0;
}
