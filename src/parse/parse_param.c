#include "../expand.h"
#include "../parse.h"
#include "../source.h"
#include "../tree.h"
#include "../../lib/scan.h"

/* parse parameter substitutions
 * ----------------------------------------------------------------------- */
int
parse_param(struct parser* p) {
  char c;
  int braces = 0;
  struct parser newp;
  struct nargparam* param;
  stralloc varname = {NULL, 0, 0};

  if(source_peek(&c) <= 0)
    return -1;

  if(c == '{') {
    braces++;
    parse_skip(p);
  }

  if(source_get(&c) <= 0)
    return -1;

  /* link in a new node */
  if(p->node && p->node->id == N_ARGSTR && p->node->nargstr.stra.len == 0)
    p->node->id = N_ARGPARAM;
  else
    parse_newnode(p, N_ARGPARAM);

  param = &p->node->nargparam;

  param->flag = p->quot;
  param->name = NULL;
  param->word = NULL;

  /* if we have # as first char in substitution and we're inside a ${}
     then check if the next char is a valid parameter char. if so then
     it's a string length subst */
  if(c == '#' && braces) {
    char nextc;

    if(source_peek(&nextc) > 0 && parse_isparam(nextc)) {
      param->flag |= S_STRLEN;
      source_get(&c);
    }

  }

  /* check for special arguments */
  switch(c) {
    case '#': param->flag |= S_ARGC; break;
    case '*': param->flag |= S_ARGV; break;
    case '@': param->flag |= S_ARGVS; break;
    case '?': param->flag |= S_EXITCODE; break;
    case '-': param->flag |= S_FLAGS; break;
    case '!': param->flag |= S_BGEXCODE; break;
    case '$': param->flag |= S_PID; break;
  }

  /* add the first char to the varname */
  stralloc_catc(&varname, c);
  stralloc_nul(&varname);

  if(!(param->flag & S_SPECIAL)) {
    /* check if it is numeric, if so assume S_ARG */
    if(parse_isdigit(c)) {
      param->flag |= S_ARG;

      /* now get the complete parameter number */
      if(braces) {
        while(source_peek(&c) > 0 && parse_isdigit(c)) {
          stralloc_catc(&varname, c);
          parse_skip(p);
        }
      }
    } else {
      /* now get the complete variable name */
      while(source_peek(&c) > 0 && parse_isname(c, varname.len)) {
        stralloc_catc(&varname, c);
        parse_skip(p);
      }
    }
  }

  stralloc_nul(&varname);

  /* scan parameter number on S_ARG */
  if(param->flag & S_ARG) {
    param->numb = 0;
    scan_uint(varname.s, (unsigned int*)&param->numb);
  }

  param->name = varname.s;

  /* skip whitespace if inside braces (unusual), otherwise return */
  if(!braces)
    return 0;

  while(source_peek(&c) && parse_isspace(c)) parse_skip(p);

  /* done parsing? */
  if(c == '}') {
    parse_skip(p);
    return 0;
  }

  /* check for remove prefix/suffix pattern */
  if(c == '%' || c == '#') {
    char nextc;
    param->flag |= (c == '%') ? S_RSSFX : S_RSPFX;
    if(source_next(&nextc) > 0 && nextc == c) {
      param->flag += (1 << 8);
      parse_skip(p);
    }
  }
  /* check for the other substitution stuff */
  else {
    /* : before -, =, ?, + means take care of set but null and not only of unset
     */
    if(c == ':') {
      param->flag |= S_NULL;
      source_next(&c);
    }

    switch(c) {
      case '-':
        param->flag |= S_DEFAULT;
        parse_skip(p);
        break;
      case '=':
        param->flag |= S_ASGNDEF;
        parse_skip(p);
        break;
      case '?':
        param->flag |= S_ERRNULL;
        parse_skip(p);
        break;
      case '+':
        param->flag |= S_ALTERNAT;
        parse_skip(p);
        break;
    }
  }

  if(p->node && p->node->id == N_ARGPARAM)
    if(p->flags & P_ARITH)
      param->flag |= S_ARITH;

  /* return now if we don't have a variable */
  /*  if((param->flag & S_SPECIAL))
    {
      if(braces)
        while(source_get(&c) > 0 && c != '}');

      return 0;
    }*/

  parse_init(&newp, P_SUBSTW);
  parse_word(&newp);

  p->node->nargparam.word = parse_getarg(&newp);

  return 0;
}
