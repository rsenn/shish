#include "../parse.h"
#include "../tree.h"

/* 3.9.4.3 - parse case statement
 *
 *  The format for the case construct is as follows.
 *
 *        case word in
 *             [(]pattern1)          compound-list;;
 *             [(]pattern2|pattern3) compound-list;;
 *             ...
 *        esac
 *
 *  The ;; is optional for the last compound-list.
 *
 * ----------------------------------------------------------------------- */
union node*
parse_case(struct parser* p) {
  enum tok_id tok;
  union node* node;
  union node** cptr;
  union node** pptr;
  union node* word;

  /* next tok must be a word */
  if(!parse_expect(p, P_DEFAULT, T_WORD | T_NAME | T_ASSIGN, NULL))
    return NULL;

  word = parse_getarg(p);
  p->pushback++;

  /* then the keyword 'in' must follow */

  tok = parse_gettok(p, P_DEFAULT);
  if(p->node->id != N_ARGSTR || stralloc_diffs(&p->node->nargstr.stra, "in")) {
    tree_free(word);
    return -1;
  }

  /*
  if(!parse_expect(p, P_DEFAULT, T_IN, word))
    return NULL;*/

  /* create new node and move the word to it */
  node = tree_newnode(N_CASE);
  node->ncase.word = word;

  /* initialize tree for the cases */
  tree_init(node->ncase.list, cptr);

  /* parse the cases */
  while(!(parse_gettok(p, P_SKIPNL) & T_ESAC)) {
    /* patterns may be introduced with '(' */
    if(!(p->tok & T_LP))
      p->pushback++;

    *cptr = tree_newnode(N_CASENODE);
    tree_init((*cptr)->ncasenode.pats, pptr);

    /* parse the pattern list */
    while(parse_gettok(p, P_SKIPNL) & (T_WORD | T_NAME | T_ASSIGN)) {
      *pptr = parse_getarg(p);
      pptr = &(*pptr)->list.next;

      if(!(parse_gettok(p, P_DEFAULT) & T_PIPE))
        break;
    }

    p->pushback++;
    if(!parse_expect(p, P_DEFAULT, T_RP | T_PIPE, node))
      return NULL;

    /* parse the compound list */
    (*cptr)->ncasenode.cmds = parse_compound_list(p);

    /* expect esac or ;; */
    if(!parse_expect(p, P_DEFAULT, T_ESAC | T_ECASE, node))
      return NULL;

    if(p->tok & T_ESAC)
      break;

    tree_next(cptr);
  }

  return node;
}
