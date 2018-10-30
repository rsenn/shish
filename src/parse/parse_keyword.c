#include "source.h"
#include "tree.h"
#include "parse.h"

/* parse shell keywords
 * 
 * keywords are assumed to be alphanumerical and will be delimited
 * on other chars <- this is BUGGY NONSENSE, move this to word parser
 * 
 * 
 * 
 * ----------------------------------------------------------------------- */
int parse_keyword(struct parser *p) {
  unsigned int ti; /* token index */
  unsigned int si = 0; /* str index */
  stralloc *sa = &p->sa;

  if(p->tok != T_NAME) {
    if(sa->len != 1)
      return 0;
    
    switch(*sa->s) {
      case '{': p->tok = T_BEGIN; return 1;
      case '}': p->tok = T_END; return 1;
      case '!': p->tok = T_NOT; return 1;
    }
    
    return 0;
  }
  
  /* longest keyword is until/while */
  if(sa->len > 5)
    return p->tok;
  
  for(ti = TI_CASE; ti <= TI_WHILE; ti++) {
    /* surely not found when char current is below token char */
    if(sa->s[si] < parse_tokens[ti].name[si])
      break;

    for(si = 0; si <= sa->len; si++) {
      /* keyword match */
      if(si == sa->len) {
        if(parse_tokens[ti].name[si] == '\0') {
          p->tok = (1 << ti);
          return 1;
        }
      } else if(sa->s[si] == parse_tokens[ti].name[si])
        continue;
        
      si = 0;
      break;
    }
  }
  
  return 0;
}

