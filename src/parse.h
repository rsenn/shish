#ifndef PARSE_H
#define PARSE_H

#include "../lib/stralloc.h"
#include <stdlib.h>

#include "tree.h"

/* character classification table
 *  * ----------------------------------------------------------------------- */
#define CHAR_BITS (sizeof(unsigned char) * 8)
#define CHAR_RANGE (1 << CHAR_BITS)

/* character class constants */
#define C_UNDEF 0x00    /* undefined character class */
#define C_SPACE 0x01    /* a whitespace character */
#define C_DIGIT 0x02    /* a numerical digit */
#define C_UPPER 0x04    /* an uppercase char */
#define C_LOWER 0x08    /* a lowercase char */
#define C_NAME 0x10     /* an alphanumeric or underscore */
#define C_SPCL 0x20     /* special parameter char */
#define C_CTRL 0x40     /* control operator char */
#define C_ESC 0x80      /* characters to escape before expansion */
#define C_DESC 0x100    /* escapable characters in double quotation mode */
#define C_QUOT 0x200    /* quotes */
#define C_HEX 0x400     /* hex digit */
#define C_OCTAL 0x800   /* octal digit */
#define C_BINARY 0x1000 /* binary digit */

/* character class table */
extern const unsigned short parse_chartable[CHAR_RANGE];

/* character class macros
 * ----------------------------------------------------------------------- */

/* matches [ \t\n] */
#define parse_isspace(c) (parse_chartable[(int)(unsigned char)c] & (C_SPACE))
/* matches [0-9] */
#define parse_isdigit(c) (parse_chartable[(int)(unsigned char)c] & (C_DIGIT))
/* matches [0-9A-Fa-f] */
#define parse_isxdigit(c) (parse_chartable[(int)(unsigned char)c] & (C_HEX))
/* matches [0-7] */
#define parse_isodigit(c) (parse_chartable[(int)(unsigned char)c] & (C_OCTAL))
/* matches [A-Z] */
#define parse_isupper(c) (parse_chartable[(int)(unsigned char)c] & (C_UPPER))
/* matches [a-z] */
#define parse_islower(c) (parse_chartable[(int)(unsigned char)c] & (C_LOWER))
/* matches [a-zA-Z] */
#define parse_isalpha(c)                                                       \
  (parse_chartable[(int)(unsigned char)c] & (C_LOWER | C_UPPER))
/* matches [a-zA-Z0-9] */
#define parse_isalnum(c)                                                       \
  (parse_chartable[(int)(unsigned char)c] & (C_LOWER | C_UPPER | C_DIGIT))
/* matches [@#?!$0-*] (special parameters) */
#define parse_isspcl(c) (parse_chartable[(int)(unsigned char)c] & (C_SPCL))

/* matches [;&|()] (control operator) */
#define parse_isctrl(c) (parse_chartable[(int)(unsigned char)c] & (C_CTRL))

/* matches [*?[]\] (glob expansion escape stuff) */
#define parse_isesc(c) (parse_chartable[(int)(unsigned char)c] & (C_ESC))

/* matches [$`"] (double quote escape stuff) */
#define parse_isdesc(c) (parse_chartable[(int)(unsigned char)c] & (C_DESC))

/* is either alpha, digit or underscore */
#define parse_isname(c)                                                        \
  (parse_chartable[(int)(unsigned char)c] & (C_LOWER | C_UPPER | C_NAME))

/* is either alpha, digit or underscore or special parameter */
#define parse_isparam(c)                                                       \
  (parse_chartable[(int)(unsigned char)c] &                                    \
   (C_LOWER | C_UPPER | C_DIGIT | C_NAME | C_SPCL))

/* token structure:
 * ----------------------------------------------------------------------- *
 * the end of list flag, t_eol, can be set to
 * the following values:
 *
 * 0 - token doesn't end a list, but may start one
 * 1 - token ends a list
 * 2 - token ends a list and may also start one
 */
struct token {
  int eol;          /* eof of list flag */
  const char* name; /* token name */
};

/* tokens */
enum tok_id {
  TI_EOF = 0, /* end of file */

  /* control operator tokens */

  TI_NL,    /* newline */
  TI_SEMI,  /* semicolon */
  TI_ECASE, /* end of case (double-semicolon) */
  TI_BGND,  /* background operator (&) */
  TI_AND,   /* boolean AND (&&) */
  TI_PIPE,  /* pipe operator (|) */
  TI_OR,    /* boolean OR (||) */
  TI_LP,    /* left parenthesis ( */
  TI_RP,    /* right parenthesis ) */
  TI_BQ,    /* backquote ` */

  /* word tokens */
  TI_NAME,   /* word token is a valid name */
  TI_WORD,   /* word (to be expanded) */
  TI_ASSIGN, /* word +token is a valid assignment in the form
                name=[word] */
  TI_REDIR,  /* redirection */

  /* keyword tokens, must be sorted alphanumerically
     for the keyword recognition to work */
  TI_NOT, /* BANG ! */
  TI_CASE,
  TI_DO,
  TI_DONE,
  TI_ELIF,
  TI_ELSE,
  TI_ESAC,
  TI_FI,
  TI_FOR,
  TI_IF,
  TI_IN,
  TI_THEN,
  TI_UNTIL,
  TI_WHILE,
  TI_BEGIN, /* left brace { */
  TI_END,   /* right brace } */
};

enum tok_flag {
  T_EOF = (1 << TI_EOF),
  T_NL = (1 << TI_NL),
  T_SEMI = (1 << TI_SEMI),
  T_ECASE = (1 << TI_ECASE),
  T_BGND = (1 << TI_BGND),
  T_AND = (1 << TI_AND),
  T_PIPE = (1 << TI_PIPE),
  T_OR = (1 << TI_OR),
  T_LP = (1 << TI_LP),
  T_RP = (1 << TI_RP),
  T_BQ = (1 << TI_BQ),
  T_REDIR = (1 << TI_REDIR),
  T_NAME = (1 << TI_NAME),
  T_WORD = (1 << TI_WORD),
  T_ASSIGN = (1 << TI_ASSIGN),
  T_NOT = (1 << TI_NOT),
  T_CASE = (1 << TI_CASE),
  T_DO = (1 << TI_DO),
  T_DONE = (1 << TI_DONE),
  T_ELIF = (1 << TI_ELIF),
  T_ELSE = (1 << TI_ELSE),
  T_ESAC = (1 << TI_ESAC),
  T_FI = (1 << TI_FI),
  T_FOR = (1 << TI_FOR),
  T_IF = (1 << TI_IF),
  T_IN = (1 << TI_IN),
  T_THEN = (1 << TI_THEN),
  T_UNTIL = (1 << TI_UNTIL),
  T_WHILE = (1 << TI_WHILE),
  T_BEGIN = (1 << TI_BEGIN),
  T_END = (1 << TI_END),
};

/* quoting
 * ----------------------------------------------------------------------- */
enum quoting { Q_UNQUOTED = 0, Q_DQUOTED = 1, Q_SQUOTED = 2, Q_BQUOTED = 3 };

/* parser context
 * ----------------------------------------------------------------------- */
struct parser {
  int flags;
  int pushback;
  enum quoting quot;
  enum tok_flag tok;
  stralloc sa;
  union node* node;
  union node* tree;
};

/* parser flags
 * ----------------------------------------------------------------------- */
#define P_DEFAULT 0x0000
#define P_IACTIVE 0x0001
#define P_NOKEYWD 0x0002  /* do not recognize keyowrds */
#define P_NOASSIGN 0x0004 /* don't parse assignments */
#define P_NOREDIR 0x0008  /* don't parse redirections */
#define P_SKIPNL 0x0040   /* skip newlines */
#define P_SUBSTW 0x0100   /* word is inside a var, so its terminated with } */
#define P_BQUOTE 0x0800   /* bquoted mode, delimit words on unesc'd bquotes */
#define P_NOSUBST 0x1000  /* do not create substitution nodes */
#define P_HERE 0x2000     /* parse here-doc */
#define P_ARITH 0x4000    /* parse arithmetic expression */

extern unsigned int parse_lineno;
extern struct token parse_tokens[];

const char* parse_tokname(enum tok_flag tok, int multi);
enum tok_flag parse_gettok(struct parser* p, int tempflags);

int parse_arith(struct parser* p);
int parse_bquoted(struct parser* p);
int parse_dquoted(struct parser* p);
enum tok_flag parse_expect(struct parser* p,
                           int tempflags,
                           enum tok_flag toks,
                           union node* nfree);
int parse_here(struct parser* p, stralloc* delim, int nosubst);
int parse_keyword(struct parser* p);
int parse_param(struct parser* p);
enum tok_flag parse_simpletok(struct parser* p);
enum tok_flag parse_skipspace(struct parser* p);
int parse_squoted(struct parser* p);
int parse_subst(struct parser* p);
int parse_unquoted(struct parser* p);
int parse_validname(stralloc* sa);
int parse_word(struct parser* p);

union node* parse_and_or(struct parser* p);
union node* parse_case(struct parser* p);
union node* parse_command(struct parser* p, int tempflags);
union node* parse_compound_list(struct parser* p);
union node* parse_for(struct parser* p);
union node* parse_function(struct parser* p);
union node* parse_getarg(struct parser* p);
union node* parse_grouping(struct parser* p, int tempflags);
union node* parse_if(struct parser* p);
union node* parse_list(struct parser* p);
union node* parse_loop(struct parser* p);
union node* parse_pipeline(struct parser* p);
union node* parse_simple_command(struct parser* p);

union node* parse_arith_binary(struct parser* p, int precedence);
union node* parse_arith_expr(struct parser* p);
union node* parse_arith_paren(struct parser* p);
union node* parse_arith_unary(struct parser* p);
union node* parse_arith_value(struct parser* p);

void* parse_error(struct parser* p, enum tok_flag toks);
void parse_init(struct parser* p, int flags);
void parse_string(struct parser* p, int flags);

#ifdef DEBUG_ALLOC
void parse_newnodedebug(const char* file,
                        unsigned int line,
                        struct parser* p,
                        enum nod_id nod);
#define parse_newnode(p, id) parse_newnodedebug(__FILE__, __LINE__, (p), (id))
#else
void parse_newnode(struct parser* p, enum nod_id nod);
#endif

#endif /* PARSE_H */
