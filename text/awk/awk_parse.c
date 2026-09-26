/* awk program parser: a small recursive-descent ladder, one function
 * per precedence level (see TODO.md Goal 11's table), rather than a
 * single table-driven climber -- awk's operator set has enough
 * grammar-level exceptions ($ prefix tightness, unary-vs-^, print's
 * '>' ambiguity, concatenation with no token of its own, '(i,j) in a')
 * that a plain ladder is easier to get right than exceptions bolted
 * onto a generic climber.
 * ----------------------------------------------------------------------- */
#include "awk_internal.h"
#include "../../lib/str.h"
#include "../../lib/byte.h"
#include "../../lib/alloc.h"

static struct anode*
new_node(struct awk_parser* p, int op) {
  struct anode* n = arena_new(p->a, struct anode);

  if(!n) {
    p->err = AWK_ENOMEM;
    return NULL;
  }

  n->op = (short)op;
  return n;
}

static int
cur(struct awk_parser* p) {
  return awk_lex_peek(&p->lx);
}

static int
advance(struct awk_parser* p) {
  return awk_lex_next(&p->lx);
}

/* consumes cur() if it matches; otherwise records a syntax error and
   leaves the token stream alone (callers keep going -- one error per
   parse is enough to report, but we must not loop forever). */
static int
expect(struct awk_parser* p, int tok) {
  if(cur(p) == tok) {
    advance(p);
    return 1;
  }

  if(!p->err) {
    p->err = AWK_ESYNTAX;
    p->errline = p->lx.line;
  }

  return 0;
}

static void
skip_newlines(struct awk_parser* p) {
  while(cur(p) == T_NEWLINE)
    advance(p);
}

static void
skip_terms(struct awk_parser* p) {
  while(cur(p) == T_NEWLINE || cur(p) == T_SEMI)
    advance(p);
}

static int
at_stmt_end(struct awk_parser* p) {
  int t = cur(p);
  return t == T_EOF || t == T_SEMI || t == T_NEWLINE || t == T_RBRACE;
}

/* ---- name interning / resolution ---------------------------------- */

static char*
intern(struct awk_parser* p, const char* s, size_t n) {
  char* r = arena_strndup(p->a, s, n);

  if(!r)
    p->err = AWK_ENOMEM;

  return r;
}

static long
find_local(struct awk_parser* p, const char* s, size_t n) {
  size_t i;

  if(!p->params)
    return -1;

  for(i = 0; i < p->nparams; i++)
    if(str_len(p->params[i]) == n && !byte_diff(p->params[i], n, s))
      return (long)i;

  return -1;
}

static long
find_or_add_global(struct awk_parser* p, const char* s, size_t n) {
  size_t i;

  for(i = 0; i < p->nglobals; i++)
    if(str_len(p->globals[i]) == n && !byte_diff(p->globals[i], n, s))
      return (long)i;

  if(p->nglobals == p->globalcap) {
    size_t newcap = p->globalcap ? p->globalcap * 2 : 32;

    p->globals = alloc_re(p->globals, newcap * sizeof(char*));
    p->globalcap = newcap;
  }

  p->globals[p->nglobals] = intern(p, s, n);
  return (long)(p->nglobals++);
}

static struct anode*
mk_var(struct awk_parser* p, const char* s, size_t n) {
  struct anode* nd = new_node(p, A_VAR);
  long li;

  if(!nd)
    return NULL;

  li = find_local(p, s, n);

  if(li >= 0) {
    nd->flags |= F_LOCAL;
    nd->idx = li;
  } else {
    nd->idx = find_or_add_global(p, s, n);
  }

  return nd;
}

static int
is_lvalue(struct anode* n) {
  return n && (n->op == A_VAR || n->op == A_INDEX || n->op == A_FIELD);
}

/* '(' ... ')' / '[' ... ']' both clear print's "no bare '>'" rule for
   their own contents (POSIX: '>' means redirection unless
   parenthesized); every call site that consumes a paren/bracket pair
   saves p->no_gt, clears it, and restores it afterwards. */

/* ---- expression parser (see awk_internal.h for the anode op list) --- */

static struct anode* parse_expr(struct awk_parser* p);
static struct anode* parse_ternary(struct awk_parser* p);
static struct anode* parse_or(struct awk_parser* p);
static struct anode* parse_and(struct awk_parser* p);
static struct anode* parse_in(struct awk_parser* p);
static struct anode* parse_match(struct awk_parser* p);
static struct anode* parse_rel(struct awk_parser* p);
static struct anode* parse_concat(struct awk_parser* p);
static struct anode* parse_add(struct awk_parser* p);
static struct anode* parse_mul(struct awk_parser* p);
static struct anode* parse_unary(struct awk_parser* p);
static struct anode* parse_pow(struct awk_parser* p);
static struct anode* parse_postfix(struct awk_parser* p);
static struct anode* parse_primary(struct awk_parser* p);
static struct anode* parse_stmt(struct awk_parser* p);
static struct anode* parse_block(struct awk_parser* p);

static struct anode*
mk1(struct awk_parser* p, int op, struct anode* a) {
  struct anode* n = new_node(p, op);

  if(n)
    n->a = a;

  return n;
}

static struct anode*
mk2(struct awk_parser* p, int op, struct anode* a, struct anode* b) {
  struct anode* n = new_node(p, op);

  if(n) {
    n->a = a;
    n->b = b;
  }

  return n;
}

static struct anode*
mk3(struct awk_parser* p, int op, struct anode* a, struct anode* b, struct anode* c) {
  struct anode* n = new_node(p, op);

  if(n) {
    n->a = a;
    n->b = b;
    n->c = c;
  }

  return n;
}

static struct anode*
list_append(struct anode* head, struct anode** tail, struct anode* item) {
  if(!item)
    return head;

  if(*tail)
    (*tail)->next = item;
  else
    head = item;

  *tail = item;
  return head;
}

/* '(' expr (',' expr)* ')', clearing no_gt for its own contents.
   Returns the single expr, or an A_GROUP node chaining >1 of them
   (consumed by parse_in for "(i,j) in a", or unwrapped by print's
   argument-list parser for "print (a,b)"). */
static struct anode*
parse_paren_list(struct awk_parser* p) {
  struct anode *head = NULL, *tail = NULL;
  int saved_no_gt = p->no_gt;

  expect(p, T_LPAREN);
  p->no_gt = 0;

  for(;;) {
    struct anode* e = parse_expr(p);

    head = list_append(head, &tail, e);

    if(cur(p) == T_COMMA) {
      advance(p);
      skip_newlines(p);
      continue;
    }

    break;
  }

  expect(p, T_RPAREN);
  p->no_gt = saved_no_gt;

  if(head && head->next)
    return mk1(p, A_GROUP, head);

  return head;
}

/* subscript list: 'name' '[' expr (',' expr)* ']' */
static struct anode*
parse_subscripts(struct awk_parser* p) {
  struct anode *head = NULL, *tail = NULL;
  int saved_no_gt = p->no_gt;

  expect(p, T_LBRACKET);
  p->no_gt = 0;

  for(;;) {
    struct anode* e = parse_expr(p);

    head = list_append(head, &tail, e);

    if(cur(p) == T_COMMA) {
      advance(p);
      skip_newlines(p);
      continue;
    }

    break;
  }

  expect(p, T_RBRACKET);
  p->no_gt = saved_no_gt;
  return head;
}

static struct anode*
parse_call_args(struct awk_parser* p) {
  struct anode *head = NULL, *tail = NULL;
  int saved_no_gt = p->no_gt;

  expect(p, T_LPAREN);
  p->no_gt = 0;

  if(cur(p) != T_RPAREN) {
    for(;;) {
      struct anode* e = parse_expr(p);

      head = list_append(head, &tail, e);

      if(cur(p) == T_COMMA) {
        advance(p);
        skip_newlines(p);
        continue;
      }

      break;
    }
  }

  expect(p, T_RPAREN);
  p->no_gt = saved_no_gt;
  return head;
}

/* an lvalue only, no operators: used by 'getline var' and 'cmd|getline var' */
static struct anode*
parse_lvalue_opt(struct awk_parser* p) {
  if(cur(p) == T_DOLLAR) {
    advance(p);
    return mk1(p, A_FIELD, parse_primary(p));
  }

  if(cur(p) == T_NAME) {
    const char* s = p->lx.sval;
    size_t n = p->lx.slen;
    struct anode* v;

    advance(p);
    v = mk_var(p, s, n);

    if(cur(p) == T_LBRACKET)
      return mk2(p, A_INDEX, v, parse_subscripts(p));

    return v;
  }

  return NULL;
}

static struct anode*
parse_getline(struct awk_parser* p) {
  struct anode *var, *file = NULL, *n;
  long idx = 0;

  advance(p); /* GETLINE */
  var = parse_lvalue_opt(p);

  if(var)
    idx |= GL_VAR;

  if(cur(p) == T_LT) {
    advance(p);
    idx |= GL_FILE;
    file = parse_unary(p);
  }

  n = new_node(p, A_GETLINE);

  if(n) {
    n->idx = idx;
    n->a = var;
    n->b = file;
  }

  return n;
}

static struct anode*
parse_primary(struct awk_parser* p) {
  int t = cur(p);

  switch(t) {
    case T_NUMBER: {
      struct anode* n = new_node(p, A_NUM);

      if(n)
        n->u.num = p->lx.numval;

      advance(p);
      return n;
    }

    case T_STRING: {
      struct anode* n = new_node(p, A_STR);
      size_t len;

      if(n)
        n->u.str = awk_unescape(p->a, p->lx.sval, p->lx.slen, &len);

      advance(p);
      return n;
    }

    case T_ERE: {
      struct anode* n = new_node(p, A_REGEX);
      char* pat;
      size_t len;

      pat = awk_unescape(p->a, p->lx.sval, p->lx.slen, &len);
      advance(p);

      if(n) {
        n->u.re = arena_new(p->a, struct dfa);

        if(!n->u.re || dfa_compile(n->u.re, pat, len, DFA_ERE) != DFA_OK) {
          p->err = AWK_EREGEX;
          p->errline = p->lx.line;
        } else {
          if(p->nregexes == p->regexcap) {
            p->regexcap = p->regexcap ? p->regexcap * 2 : 8;
            p->regexes = alloc_re(p->regexes, p->regexcap * sizeof(struct dfa*));
          }

          p->regexes[p->nregexes++] = n->u.re;
        }
      }

      return n;
    }

    case T_DOLLAR: {
      advance(p);
      return mk1(p, A_FIELD, parse_primary(p));
    }

    case T_INCR:
    case T_DECR: {
      long idx = 1 /* pre */ | (t == T_DECR ? 2 : 0);
      struct anode *operand, *n;

      advance(p);
      operand = parse_primary(p);

      if(!is_lvalue(operand) && !p->err) {
        p->err = AWK_ESYNTAX;
        p->errline = p->lx.line;
      }

      n = mk1(p, A_INCDEC, operand);

      if(n)
        n->idx = idx;

      return n;
    }

    case T_LPAREN: return parse_paren_list(p);

    case T_NAME: {
      const char* s = p->lx.sval;
      size_t n = p->lx.slen;
      struct anode* v;

      advance(p);
      v = mk_var(p, s, n);

      if(cur(p) == T_LBRACKET)
        return mk2(p, A_INDEX, v, parse_subscripts(p));

      return v;
    }

    case T_FUNC_NAME: {
      char* name = intern(p, p->lx.sval, p->lx.slen);
      struct anode* n;

      advance(p);
      n = new_node(p, A_CALL);

      if(n) {
        n->u.str = name;
        n->a = parse_call_args(p);
      } else {
        parse_call_args(p);
      }

      return n;
    }

    case T_BUILTIN: {
      static const struct {
        const char* name;
        int id;
      } tbl[] = {{"length", BI_LENGTH},
                 {"substr", BI_SUBSTR},
                 {"index", BI_INDEX},
                 {"split", BI_SPLIT},
                 {"sub", BI_SUB},
                 {"gsub", BI_GSUB},
                 {"match", BI_MATCH},
                 {"sprintf", BI_SPRINTF},
                 {"sin", BI_SIN},
                 {"cos", BI_COS},
                 {"atan2", BI_ATAN2},
                 {"exp", BI_EXP},
                 {"log", BI_LOG},
                 {"sqrt", BI_SQRT},
                 {"int", BI_INT},
                 {"rand", BI_RAND},
                 {"srand", BI_SRAND},
                 {"tolower", BI_TOLOWER},
                 {"toupper", BI_TOUPPER},
                 {"system", BI_SYSTEM},
                 {"close", BI_CLOSE},
                 {"fflush", BI_FFLUSH},
                 {NULL, 0}};
      const char* s = p->lx.sval;
      size_t n = p->lx.slen;
      struct anode* nd;
      size_t i;
      int id = -1;

      for(i = 0; tbl[i].name; i++)
        if(str_len(tbl[i].name) == n && !byte_diff(tbl[i].name, n, s)) {
          id = tbl[i].id;
          break;
        }

      advance(p);
      nd = new_node(p, A_CALLBUILTIN);

      if(nd)
        nd->idx = id;

      if(cur(p) == T_LPAREN) {
        struct anode* args = parse_call_args(p);

        if(nd)
          nd->a = args;
      }

      return nd;
    }

    case T_GETLINE: return parse_getline(p);

    default:
      if(!p->err) {
        p->err = AWK_ESYNTAX;
        p->errline = p->lx.line;
      }

      advance(p);
      return NULL;
  }
}

static struct anode*
parse_postfix(struct awk_parser* p) {
  struct anode* base = parse_primary(p);

  while(is_lvalue(base) && (cur(p) == T_INCR || cur(p) == T_DECR)) {
    int dec = (cur(p) == T_DECR);
    struct anode* n;

    advance(p);
    n = mk1(p, A_INCDEC, base);

    if(n)
      n->idx = dec ? 2 : 0;

    base = n;
  }

  return base;
}

static struct anode*
parse_pow(struct awk_parser* p) {
  struct anode* base = parse_postfix(p);

  if(cur(p) == T_CARET) {
    struct anode* rhs;

    advance(p);
    rhs = parse_unary(p);
    return mk2(p, A_POW, base, rhs);
  }

  return base;
}

static struct anode*
parse_unary(struct awk_parser* p) {
  int t = cur(p);

  if(t == T_NOT || t == T_PLUS || t == T_MINUS) {
    int op = (t == T_NOT) ? A_NOT : (t == T_MINUS) ? A_UMINUS : A_UPLUS;

    advance(p);
    return mk1(p, op, parse_unary(p));
  }

  return parse_pow(p);
}

static struct anode*
parse_mul(struct awk_parser* p) {
  struct anode* left = parse_unary(p);

  for(;;) {
    int t = cur(p), op;

    if(t == T_STAR)
      op = A_MUL;
    else if(t == T_SLASH)
      op = A_DIV;
    else if(t == T_PERCENT)
      op = A_MOD;
    else
      break;

    advance(p);
    left = mk2(p, op, left, parse_unary(p));
  }

  return left;
}

static struct anode*
parse_add(struct awk_parser* p) {
  struct anode* left = parse_mul(p);

  for(;;) {
    int t = cur(p), op;

    if(t == T_PLUS)
      op = A_ADD;
    else if(t == T_MINUS)
      op = A_SUB;
    else
      break;

    advance(p);
    left = mk2(p, op, left, parse_mul(p));
  }

  return left;
}

static int
starts_concat_operand(int t) {
  switch(t) {
    case T_NUMBER:
    case T_STRING:
    case T_ERE:
    case T_NAME:
    case T_FUNC_NAME:
    case T_BUILTIN:
    case T_DOLLAR:
    case T_NOT:
    case T_INCR:
    case T_DECR:
    case T_LPAREN:
    case T_GETLINE: return 1;
    default: return 0;
  }
}

static struct anode*
parse_concat(struct awk_parser* p) {
  struct anode* left = parse_add(p);

  while(starts_concat_operand(cur(p)))
    left = mk2(p, A_CONCAT, left, parse_add(p));

  return left;
}

static struct anode*
parse_rel(struct awk_parser* p) {
  struct anode* left = parse_concat(p);

  if(cur(p) == T_PIPE) {
    struct anode *cmdvar, *n;

    advance(p);
    expect(p, T_GETLINE);
    cmdvar = parse_lvalue_opt(p);
    n = new_node(p, A_GETLINE);

    if(n) {
      n->idx = GL_CMD | (cmdvar ? GL_VAR : 0);
      n->a = cmdvar;
      n->b = left;
    }

    left = n;
  }

  {
    int t = cur(p), kind = -1;

    switch(t) {
      case T_LT: kind = CMP_LT; break;
      case T_LE: kind = CMP_LE; break;
      case T_GT:
        if(!p->no_gt)
          kind = CMP_GT;
        break;
      case T_GE: kind = CMP_GE; break;
      case T_EQ: kind = CMP_EQ; break;
      case T_NE: kind = CMP_NE; break;
      default: break;
    }

    if(kind >= 0) {
      struct anode* right;
      struct anode* n;

      advance(p);
      right = parse_concat(p);
      n = new_node(p, A_CMP);

      if(n) {
        n->idx = kind;
        n->a = left;
        n->b = right;
      }

      left = n;
    }
  }

  return left;
}

static struct anode*
parse_match(struct awk_parser* p) {
  struct anode* left = parse_rel(p);

  while(cur(p) == T_TILDE || cur(p) == T_NOMATCH) {
    int neg = (cur(p) == T_NOMATCH);
    struct anode* n;

    advance(p);
    n = mk2(p, A_MATCH, left, parse_rel(p));

    if(n)
      n->idx = neg;

    left = n;
  }

  return left;
}

static struct anode*
parse_in(struct awk_parser* p) {
  struct anode* left = parse_match(p);

  while(cur(p) == T_IN) {
    struct anode *arr, *sub, *n;
    const char* s;
    size_t slen;

    advance(p);

    if(cur(p) != T_NAME) {
      if(!p->err) {
        p->err = AWK_ESYNTAX;
        p->errline = p->lx.line;
      }
      break;
    }

    s = p->lx.sval;
    slen = p->lx.slen;
    advance(p);
    arr = mk_var(p, s, slen);
    sub = (left && left->op == A_GROUP) ? left->a : left;
    n = mk2(p, A_IN, sub, arr);
    left = n;
  }

  return left;
}

static struct anode*
parse_and(struct awk_parser* p) {
  struct anode* left = parse_in(p);

  while(cur(p) == T_ANDAND) {
    advance(p);
    skip_newlines(p);
    left = mk2(p, A_AND, left, parse_in(p));
  }

  return left;
}

static struct anode*
parse_or(struct awk_parser* p) {
  struct anode* left = parse_and(p);

  while(cur(p) == T_OROR) {
    advance(p);
    skip_newlines(p);
    left = mk2(p, A_OR, left, parse_and(p));
  }

  return left;
}

static struct anode*
parse_ternary(struct awk_parser* p) {
  struct anode* cond = parse_or(p);

  if(cur(p) == T_QUESTION) {
    struct anode *a, *b;

    advance(p);
    skip_newlines(p);
    a = parse_ternary(p);
    skip_newlines(p);
    expect(p, T_COLON);
    skip_newlines(p);
    b = parse_ternary(p);
    return mk3(p, A_COND, cond, a, b);
  }

  return cond;
}

static struct anode*
parse_expr(struct awk_parser* p) {
  struct anode* left = parse_ternary(p);

  if(is_lvalue(left)) {
    int t = cur(p), addop = -1;

    switch(t) {
      case T_ASSIGN: addop = 0; break;
      case T_ADD_ASSIGN: addop = ADDOP_ADD; break;
      case T_SUB_ASSIGN: addop = ADDOP_SUB; break;
      case T_MUL_ASSIGN: addop = ADDOP_MUL; break;
      case T_DIV_ASSIGN: addop = ADDOP_DIV; break;
      case T_MOD_ASSIGN: addop = ADDOP_MOD; break;
      case T_POW_ASSIGN: addop = ADDOP_POW; break;
      default: break;
    }

    if(addop >= 0) {
      struct anode *rhs, *n;

      advance(p);
      rhs = parse_expr(p);
      n = new_node(p, A_ASSIGNOP);

      if(n) {
        n->idx = addop;
        n->a = left;
        n->b = rhs;
      }

      return n;
    }
  }

  return left;
}

/* ---- statements ------------------------------------------------------ */

static struct anode*
parse_block(struct awk_parser* p) {
  struct anode *head = NULL, *tail = NULL;

  expect(p, T_LBRACE);
  skip_terms(p);

  while(cur(p) != T_RBRACE && cur(p) != T_EOF && !p->err) {
    struct anode* s = parse_stmt(p);

    head = list_append(head, &tail, s);
    skip_terms(p);
  }

  expect(p, T_RBRACE);
  return mk1(p, A_BLOCK, head);
}

static struct anode*
parse_print_args(struct awk_parser* p, struct anode** redir_target, long* redir_kind) {
  struct anode *head = NULL, *tail = NULL;

  p->no_gt = 1;

  if(!at_stmt_end(p) && cur(p) != T_GT && cur(p) != T_APPEND && cur(p) != T_PIPE) {
    for(;;) {
      struct anode* e = parse_expr(p);

      /* "print (a,b)": a single parenthesized list stands for the
         whole argument list, not one argument holding a group node */
      if(head == NULL && tail == NULL && e && e->op == A_GROUP && cur(p) != T_COMMA) {
        head = e->a;
        break;
      }

      head = list_append(head, &tail, e);

      if(cur(p) == T_COMMA) {
        advance(p);
        skip_newlines(p);
        continue;
      }

      break;
    }
  }

  p->no_gt = 0;

  *redir_kind = REDIR_NONE;
  *redir_target = NULL;

  if(cur(p) == T_GT) {
    advance(p);
    *redir_kind = REDIR_TRUNC;
    *redir_target = parse_ternary(p);
  } else if(cur(p) == T_APPEND) {
    advance(p);
    *redir_kind = REDIR_APPEND;
    *redir_target = parse_ternary(p);
  } else if(cur(p) == T_PIPE) {
    advance(p);
    *redir_kind = REDIR_PIPE;
    *redir_target = parse_ternary(p);
  }

  return head;
}

static struct anode*
parse_stmt(struct awk_parser* p) {
  int t = cur(p);

  switch(t) {
    case T_LBRACE: return parse_block(p);

    case T_IF: {
      struct anode *cond, *thenb, *elseb = NULL;

      advance(p);
      expect(p, T_LPAREN);
      cond = parse_expr(p);
      expect(p, T_RPAREN);
      skip_newlines(p);
      thenb = parse_stmt(p);
      skip_terms(p);

      if(cur(p) == T_ELSE) {
        advance(p);
        skip_newlines(p);
        elseb = parse_stmt(p);
      }

      return mk3(p, A_IF, cond, thenb, elseb);
    }

    case T_WHILE: {
      struct anode *cond, *body;

      advance(p);
      expect(p, T_LPAREN);
      cond = parse_expr(p);
      expect(p, T_RPAREN);
      skip_newlines(p);
      body = parse_stmt(p);
      return mk2(p, A_WHILE, cond, body);
    }

    case T_DO: {
      struct anode *body, *cond, *n;

      advance(p);
      skip_newlines(p);
      body = parse_stmt(p);
      skip_terms(p);
      expect(p, T_WHILE);
      expect(p, T_LPAREN);
      cond = parse_expr(p);
      expect(p, T_RPAREN);
      n = mk2(p, A_DOWHILE, body, cond);
      return n;
    }

    case T_FOR: {
      struct awk_lexer saved;

      advance(p);
      expect(p, T_LPAREN);

      if(cur(p) == T_NAME) {
        saved = p->lx;

        {
          const char* s = p->lx.sval;
          size_t slen = p->lx.slen;

          advance(p);

          if(cur(p) == T_IN) {
            advance(p);

            if(cur(p) == T_NAME) {
              const char* as = p->lx.sval;
              size_t alen = p->lx.slen;

              advance(p);

              if(cur(p) == T_RPAREN) {
                struct anode *var, *arr, *body;

                advance(p);
                skip_newlines(p);
                var = mk_var(p, s, slen);
                arr = mk_var(p, as, alen);
                body = parse_stmt(p);
                return mk3(p, A_FORIN, var, arr, body);
              }
            }
          }
        }

        p->lx = saved; /* not a for-in: rewind and parse classic for */
      }

      {
        struct anode *init = NULL, *cond = NULL, *post = NULL, *body;

        if(cur(p) != T_SEMI)
          init = parse_stmt(p);

        expect(p, T_SEMI);
        skip_newlines(p);

        if(cur(p) != T_SEMI)
          cond = parse_expr(p);

        expect(p, T_SEMI);
        skip_newlines(p);

        if(cur(p) != T_RPAREN)
          post = parse_stmt(p);

        expect(p, T_RPAREN);
        skip_newlines(p);
        body = parse_stmt(p);

        {
          struct anode* n = new_node(p, A_FOR);

          if(n) {
            n->a = init;
            n->b = cond;
            n->c = post;
            n->d = body;
          }

          return n;
        }
      }
    }

    case T_BREAK: advance(p); return new_node(p, A_BREAK);
    case T_CONTINUE: advance(p); return new_node(p, A_CONTINUE);
    case T_NEXT: advance(p); return new_node(p, A_NEXT);
    case T_NEXTFILE: advance(p); return new_node(p, A_NEXTFILE);

    case T_EXIT: {
      advance(p);
      return mk1(p, A_EXIT, at_stmt_end(p) ? NULL : parse_expr(p));
    }

    case T_RETURN: {
      advance(p);
      return mk1(p, A_RETURN, at_stmt_end(p) ? NULL : parse_expr(p));
    }

    case T_DELETE: {
      struct anode *arr, *sub = NULL;
      const char* s;
      size_t n;

      advance(p);

      if(cur(p) != T_NAME) {
        if(!p->err) {
          p->err = AWK_ESYNTAX;
          p->errline = p->lx.line;
        }

        return NULL;
      }

      s = p->lx.sval;
      n = p->lx.slen;
      advance(p);
      arr = mk_var(p, s, n);

      if(cur(p) == T_LBRACKET)
        sub = parse_subscripts(p);

      return mk2(p, A_DELETE, arr, sub);
    }

    case T_PRINT:
    case T_PRINTF: {
      struct anode *args, *target, *n;
      long kind;
      int isprintf = (t == T_PRINTF);

      advance(p);
      args = parse_print_args(p, &target, &kind);
      n = new_node(p, isprintf ? A_PRINTF : A_PRINT);

      if(n) {
        n->idx = kind;
        n->a = args;
        n->b = target;
      }

      return n;
    }

    case T_SEMI: return NULL; /* empty statement */

    default: return mk1(p, A_EXPRSTMT, parse_expr(p));
  }
}

/* ---- program-level parsing and the post-parse fixups ------------------ */

static void
walk_calls(struct awk_parser* p, struct anode* n) {
  if(!n)
    return;

  if(n->op == A_CALL) {
    struct awk_func* fn = awk_func_find(p->prog, n->u.str, str_len(n->u.str));

    if(!fn) {
      p->err = AWK_ESYNTAX;
      p->errline = p->lx.line;
    } else {
      n->idx = (long)(fn - p->prog->funcs);
    }
  }

  walk_calls(p, n->a);
  walk_calls(p, n->b);
  walk_calls(p, n->c);
  walk_calls(p, n->d);
  walk_calls(p, n->next);
}

static int
walk_uses_input(struct anode* n) {
  if(!n)
    return 0;

  if(n->op == A_GETLINE && n->idx == 0)
    return 1;

  return walk_uses_input(n->a) || walk_uses_input(n->b) || walk_uses_input(n->c) ||
         walk_uses_input(n->d) || walk_uses_input(n->next);
}

static const char* const special_names[NSPECIAL] = {"NF",
                                                    "NR",
                                                    "FNR",
                                                    "FS",
                                                    "OFS",
                                                    "ORS",
                                                    "RS",
                                                    "SUBSEP",
                                                    "CONVFMT",
                                                    "OFMT",
                                                    "RSTART",
                                                    "RLENGTH",
                                                    "FILENAME",
                                                    "ARGC",
                                                    "ARGV",
                                                    "ENVIRON"};

int
awk_parse_program(struct awk_parser* p) {
  size_t i;

  p->globalcap = 64;
  p->globals = alloc(p->globalcap * sizeof(char*));

  for(i = 0; i < NSPECIAL; i++)
    p->globals[p->nglobals++] = (char*)special_names[i];

  skip_terms(p);

  while(cur(p) != T_EOF && !p->err) {
    if(cur(p) == T_BEGIN) {
      struct anode* b;

      advance(p);
      skip_newlines(p);
      b = parse_block(p);
      p->begin = list_append(p->begin, &p->beginlast, b);
    } else if(cur(p) == T_END) {
      struct anode* b;

      advance(p);
      skip_newlines(p);
      b = parse_block(p);
      p->end = list_append(p->end, &p->endlast, b);
    } else if(cur(p) == T_FUNCTION) {
      char* name;
      char** params = NULL;
      size_t nparams = 0, paramcap = 0;
      struct anode* body;

      advance(p);

      if(cur(p) != T_NAME && cur(p) != T_FUNC_NAME) {
        p->err = AWK_ESYNTAX;
        p->errline = p->lx.line;
        break;
      }

      name = intern(p, p->lx.sval, p->lx.slen);
      advance(p);
      expect(p, T_LPAREN);

      if(cur(p) != T_RPAREN) {
        for(;;) {
          if(cur(p) != T_NAME) {
            p->err = AWK_ESYNTAX;
            p->errline = p->lx.line;
            break;
          }

          if(nparams == paramcap) {
            paramcap = paramcap ? paramcap * 2 : 8;
            params = alloc_re(params, paramcap * sizeof(char*));
          }

          params[nparams++] = intern(p, p->lx.sval, p->lx.slen);
          advance(p);

          if(cur(p) == T_COMMA) {
            advance(p);
            skip_newlines(p);
            continue;
          }

          break;
        }
      }

      expect(p, T_RPAREN);
      skip_newlines(p);

      p->params = params;
      p->nparams = nparams;
      body = parse_block(p);
      p->params = NULL;
      p->nparams = 0;

      /* params[] was grown with alloc_re() (malloc) during parsing;
         move it into the arena so it lives (and dies) with the
         program instead of leaking, and free the scratch copy */
      if(nparams) {
        char** aparams = arena_allocn(p->a, sizeof(char*), nparams, __alignof__(char*));

        if(aparams)
          byte_copy(aparams, nparams * sizeof(char*), params);

        alloc_free(params);
        params = aparams;
      }

      if(p->nfuncs == p->funccap) {
        p->funccap = p->funccap ? p->funccap * 2 : 8;
        p->funcs = alloc_re(p->funcs, p->funccap * sizeof(struct awk_func));
      }

      p->funcs[p->nfuncs].name = name;
      p->funcs[p->nfuncs].params = params;
      p->funcs[p->nfuncs].nparams = nparams;
      p->funcs[p->nfuncs].body = body;
      p->nfuncs++;
    } else {
      struct anode *pat1 = NULL, *pat2 = NULL, *action = NULL;
      int kind = 0;

      if(cur(p) != T_LBRACE) {
        pat1 = parse_expr(p);
        kind = 1;

        if(cur(p) == T_COMMA) {
          advance(p);
          skip_newlines(p);
          pat2 = parse_expr(p);
          kind = 2;
        }
      }

      if(cur(p) == T_LBRACE)
        action = parse_block(p);

      if(p->nrules == p->rulecap) {
        p->rulecap = p->rulecap ? p->rulecap * 2 : 16;
        p->rules = alloc_re(p->rules, p->rulecap * sizeof(struct awk_rule));
      }

      p->rules[p->nrules].kind = kind;
      p->rules[p->nrules].pat1 = pat1;
      p->rules[p->nrules].pat2 = pat2;
      p->rules[p->nrules].action = action;
      p->nrules++;
    }

    skip_terms(p);
  }

  /* materialize prog->funcs/rules before the call-fixup pass, which
     resolves A_CALL by looking functions up in prog->funcs */
  p->prog->funcs =
      arena_allocn(p->a, sizeof(struct awk_func), p->nfuncs, __alignof__(struct awk_func));

  if(p->nfuncs)
    byte_copy(p->prog->funcs, p->nfuncs * sizeof(struct awk_func), p->funcs);

  p->prog->nfuncs = p->nfuncs;

  if(!p->err) {
    walk_calls(p, p->begin);
    walk_calls(p, p->end);

    for(i = 0; i < p->nrules; i++) {
      walk_calls(p, p->rules[i].pat1);
      walk_calls(p, p->rules[i].pat2);
      walk_calls(p, p->rules[i].action);
    }

    for(i = 0; i < p->nfuncs; i++)
      walk_calls(p, p->prog->funcs[i].body);
  }

  p->prog->begin = p->begin;
  p->prog->end = p->end;
  p->prog->rules =
      arena_allocn(p->a, sizeof(struct awk_rule), p->nrules, __alignof__(struct awk_rule));

  if(p->nrules)
    byte_copy(p->prog->rules, p->nrules * sizeof(struct awk_rule), p->rules);

  p->prog->nrules = p->nrules;

  p->prog->globalnames = arena_allocn(p->a, sizeof(char*), p->nglobals, __alignof__(char*));
  byte_copy(p->prog->globalnames, p->nglobals * sizeof(char*), p->globals);
  p->prog->nglobals = p->nglobals;

  p->prog->regexes = arena_allocn(p->a, sizeof(struct dfa*), p->nregexes, __alignof__(struct dfa*));

  if(p->nregexes)
    byte_copy(p->prog->regexes, p->nregexes * sizeof(struct dfa*), p->regexes);

  p->prog->nregexes = p->nregexes;

  p->prog->uses_main_input =
      !p->err && ((p->nrules > 0) || walk_uses_input(p->begin) || walk_uses_input(p->end));

  for(i = 0; i < p->nfuncs && !p->prog->uses_main_input; i++)
    p->prog->uses_main_input = walk_uses_input(p->prog->funcs[i].body);

  alloc_free(p->globals);
  alloc_free(p->rules);
  alloc_free(p->funcs);
  alloc_free(p->regexes);
  return p->err ? p->err : AWK_OK;
}
