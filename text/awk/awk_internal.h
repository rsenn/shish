/* internal to text/awk (not installed, not included by callers of
 * text/awk.h): node tree, cell/value representation, lexer/parser and
 * interpreter state shared by the lexer/parser/interpreter/builtins/
 * field/IO files.
 * ----------------------------------------------------------------------- */
#ifndef TEXT_AWK_INTERNAL_H
#define TEXT_AWK_INTERNAL_H

#include "../awk.h"
#include "../dfa.h"
#include "../../lib/arena.h"
#include "../../lib/hashmap.h"
#include "../../lib/stralloc.h"
#include "../../lib/buffer.h"

/* ---- node tree ---------------------------------------------------- */

enum {
  /* primaries / operators */
  A_NUM,      /* u.num */
  A_STR,      /* u.str (arena) */
  A_REGEX,    /* u.re; also usable as a bare expression ($0 ~ /re/) */
  A_VAR,      /* idx: slot index; flags & F_LOCAL selects the frame */
  A_INDEX,    /* a = array var (A_VAR); b = subscript list (->next chain) */
  A_FIELD,    /* a = $ operand expr ($0, $1, $NF, $(expr)...) */
  A_ASSIGNOP, /* idx: 0 = '=', else one of ADDOP_* below; a = lvalue, b = rhs */
  A_INCDEC,   /* idx: bit0 = pre, bit1 = decrement; a = lvalue */
  A_NOT,
  A_UMINUS,
  A_UPLUS,
  A_POW,
  A_MUL,
  A_DIV,
  A_MOD,
  A_ADD,
  A_SUB,
  A_CONCAT,
  A_CMP,   /* idx: CMP_* below */
  A_MATCH, /* idx: 0 = '~', 1 = '!~'; a = subject, b = regex-valued expr */
  A_IN,    /* a = subscript list (->next chain); b = array var (A_VAR) */
  A_AND,
  A_OR,
  A_COND,       /* a = cond, b = then, c = else */
  A_CALL,       /* u.str = name (until the fixup pass sets idx); a = args */
  A_CALLBUILTIN,/* idx = BI_*; a = args (->next chain) */
  A_GETLINE,    /* idx: GL_* bits; a = var lvalue or NULL; b = file/cmd expr */
  A_GROUP,      /* '(i, j) in a': subscript list under a, like A_IN's a */

  /* statements */
  A_BLOCK, /* a = first stmt of a ->next chain */
  A_IF,    /* a = cond, b = then, c = else (nullable) */
  A_WHILE, /* a = cond, b = body */
  A_DOWHILE, /* a = body, b = cond */
  A_FOR,     /* a = init stmt, b = cond expr, c = post stmt, d = body (any may be NULL except d) */
  A_FORIN,   /* a = var (A_VAR, simple), b = array var, c = body */
  A_BREAK,
  A_CONTINUE,
  A_NEXT,
  A_NEXTFILE,
  A_EXIT,   /* a = expr or NULL */
  A_RETURN, /* a = expr or NULL */
  A_DELETE, /* a = array var; b = subscript list (->next) or NULL = whole array */
  A_PRINT,  /* idx: 0 none, '>' , 'A' (>>), '|'; a = args (->next); b = redir target expr */
  A_PRINTF, /* same shape as A_PRINT; a's first is the format */
  A_EXPRSTMT /* a = expr */
};

/* A_ASSIGNOP idx (0 is plain '=') */
enum { ADDOP_ADD = 1, ADDOP_SUB, ADDOP_MUL, ADDOP_DIV, ADDOP_MOD, ADDOP_POW };

/* A_CMP idx */
enum { CMP_LT, CMP_LE, CMP_GT, CMP_GE, CMP_EQ, CMP_NE };

/* A_GETLINE idx bits */
#define GL_VAR 0x01  /* has an lvalue target (a) instead of $0/NF/NR/FNR */
#define GL_FILE 0x02 /* "< file" */
#define GL_CMD 0x04  /* "cmd |" */

/* A_PRINT/A_PRINTF idx: redirection kind, stored as the operator byte
 * itself so 0 doubles as "no redirection" */
enum { REDIR_NONE = 0, REDIR_TRUNC = '>', REDIR_APPEND = 'A', REDIR_PIPE = '|' };

/* built-in function ids (A_CALLBUILTIN.idx) */
enum {
  BI_LENGTH,
  BI_SUBSTR,
  BI_INDEX,
  BI_SPLIT,
  BI_SUB,
  BI_GSUB,
  BI_MATCH,
  BI_SPRINTF,
  BI_SIN,
  BI_COS,
  BI_ATAN2,
  BI_EXP,
  BI_LOG,
  BI_SQRT,
  BI_INT,
  BI_RAND,
  BI_SRAND,
  BI_TOLOWER,
  BI_TOUPPER,
  BI_SYSTEM,
  BI_CLOSE,
  BI_FFLUSH
};

/* anode.flags */
#define F_LOCAL 0x01 /* A_VAR: idx indexes the current call frame, not globals */

struct anode {
  short op;
  short flags;
  long idx;
  union {
    double num;
    char* str;
    struct dfa* re;
  } u;
  struct anode *a, *b, *c, *d;
  struct anode* next; /* sibling chain: statements, argument/subscript lists */
};

/* ---- special (fixed-index) globals --------------------------------- */

enum {
  SP_NF,
  SP_NR,
  SP_FNR,
  SP_FS,
  SP_OFS,
  SP_ORS,
  SP_RS,
  SP_SUBSEP,
  SP_CONVFMT,
  SP_OFMT,
  SP_RSTART,
  SP_RLENGTH,
  SP_FILENAME,
  SP_ARGC,
  SP_ARGV,
  SP_ENVIRON,
  NSPECIAL
};

/* ---- program (the compiled, immutable result of awk_compile()) ---- */

struct awk_rule {
  int kind; /* 0 = always ($0-less BEGIN/END aren't rules), 1 = expr, 2 = range */
  struct anode *pat1, *pat2; /* pat2 set only for a range pattern */
  struct anode* action;      /* NULL = the default "{ print }" */
};

struct awk_func {
  char* name;
  char** params;
  size_t nparams;
  struct anode* body;
};

struct awk_prog {
  arena a;
  struct anode* begin;  /* ->next chain of BEGIN block bodies (each an A_BLOCK) */
  struct anode* end;    /* ->next chain of END block bodies */
  struct awk_rule* rules;
  size_t nrules;
  struct awk_func* funcs;
  size_t nfuncs;
  char** globalnames; /* NSPECIAL fixed + user globals, in index order */
  size_t nglobals;
  int uses_main_input; /* false if every pattern/action and BEGIN/END never
                          touches $0/NF/fields/getline: skip the read loop */

  /* every /re/ literal compiled during parsing: struct dfa's own
     internals are alloc()'d (heap), not arena, so awk_free() must
     dfa_free() each of these explicitly before releasing the arena. */
  struct dfa** regexes;
  size_t nregexes;
};

/* function lookup by name, used by the parser's fixup pass and by
 * BI_* dispatch's "is this name shadowed by a function" checks. */
struct awk_func* awk_func_find(struct awk_prog* prog, const char* name, size_t len);

/* ---- lexer ---------------------------------------------------------- */

enum {
  T_EOF = 0,
  T_NEWLINE,
  T_NUMBER,
  T_STRING,
  T_ERE,
  T_NAME,
  T_FUNC_NAME,
  T_BUILTIN,
  T_GETLINE,
  /* keywords */
  T_BEGIN,
  T_END,
  T_FUNCTION,
  T_IF,
  T_ELSE,
  T_WHILE,
  T_FOR,
  T_DO,
  T_BREAK,
  T_CONTINUE,
  T_NEXT,
  T_NEXTFILE,
  T_EXIT,
  T_RETURN,
  T_DELETE,
  T_IN,
  T_PRINT,
  T_PRINTF,
  /* punctuation/operators */
  T_LBRACE,
  T_RBRACE,
  T_LPAREN,
  T_RPAREN,
  T_LBRACKET,
  T_RBRACKET,
  T_SEMI,
  T_COMMA,
  T_DOLLAR,
  T_ASSIGN,
  T_ADD_ASSIGN,
  T_SUB_ASSIGN,
  T_MUL_ASSIGN,
  T_DIV_ASSIGN,
  T_MOD_ASSIGN,
  T_POW_ASSIGN,
  T_OROR,
  T_ANDAND,
  T_NOMATCH,
  T_EQ,
  T_LE,
  T_GE,
  T_NE,
  T_INCR,
  T_DECR,
  T_APPEND, /* >> */
  T_LT,
  T_GT,
  T_PIPE,
  T_TILDE,
  T_NOT,
  T_PLUS,
  T_MINUS,
  T_STAR,
  T_SLASH,
  T_PERCENT,
  T_CARET,
  T_QUESTION,
  T_COLON
};

struct awk_lexer {
  const char* p;
  const char* end;
  unsigned long line;
  int prev; /* class of the previous significant token, for '/' disambiguation
               and for allowing a newline after certain tokens */
  int paren_depth;
  /* one token of lookahead */
  int tok;
  double numval;
  const char* sval; /* points into source or a scratch buffer; sval/slen valid
                        until the next awk_lex_next() call */
  size_t slen;
  int have_lookahead;
  int err; /* AWK_E* once a lexer-level error (bad string/regex) is hit */
};

void awk_lex_init(struct awk_lexer* lx, const char* src, size_t len);
int awk_lex_next(struct awk_lexer* lx); /* returns/consumes the current token */
int awk_lex_peek(struct awk_lexer* lx); /* returns without consuming */
/* re-lexes the current '/'-starting token as an ERE instead of division;
   only valid to call immediately after a T_SLASH/T_DIV_ASSIGN peek */
int awk_lex_force_ere(struct awk_lexer* lx);

/* shared escape-sequence pass (strings and, once slash-quoting is
   undone, dynamic regexes): decodes \\ \" \/ \a \b \f \n \r \t \v \ddd
   into *out (arena-owned). */
char* awk_unescape(arena* a, const char* s, size_t n, size_t* outlen);

/* ---- parser ---------------------------------------------------------- */

struct awk_parser {
  struct awk_lexer lx;
  arena* a;
  struct awk_prog* prog;
  int err;
  unsigned long errline;
  int no_gt; /* inside print/printf's argument list: a bare '>' ends the
                list (redirection) instead of parsing as "greater than" */

  /* global name table (grows during parsing) */
  char** globals;
  size_t nglobals, globalcap;

  /* current function's parameters, or NULL at top level */
  char** params;
  size_t nparams;

  /* rules/funcs, grown with plain realloc (parse time only; freed into
     the arena at the end via awk_parser_finish) */
  struct awk_rule* rules;
  size_t nrules, rulecap;
  struct awk_func* funcs;
  size_t nfuncs, funccap;
  struct anode *begin, *beginlast, *end, *endlast;

  struct dfa** regexes;
  size_t nregexes, regexcap;
};

int awk_parse_program(struct awk_parser* p);

/* ---- values ---------------------------------------------------------- */

enum { CELL_UNINIT = 0, CELL_NUM, CELL_STR, CELL_STRNUM, CELL_ARRAY };

typedef struct awk_cell {
  unsigned char type;
  double num;
  char* str; /* heap (alloc()), NUL-terminated; owned by this cell */
  hashmap* arr;
} awk_cell;

void awk_cell_free(awk_cell* c);       /* frees str/arr, does not free c itself */
void awk_cell_set_num(awk_cell* c, double n);
void awk_cell_set_str(awk_cell* c, const char* s, size_t n, int strnum);
void awk_cell_assign(awk_cell* dst, const awk_cell* src); /* copy by value (never ARRAY) */

struct awk_state; /* forward */
double awk_tonum(struct awk_state* st, awk_cell* c);
const char* awk_tostr(struct awk_state* st, awk_cell* c, int output); /* output: use OFMT, else CONVFMT */
int awk_tobool(struct awk_state* st, awk_cell* c);
int awk_looks_numeric(const char* s, size_t n, double* out);
int awk_is_numeric_cell(const awk_cell* c); /* NUM, or STRNUM that looks numeric, or UNINIT */
int awk_cmp(struct awk_state* st, awk_cell* a, awk_cell* b); /* <0,0,>0 */

/* ---- fields/records ---------------------------------------------------- */

struct awk_rec {
  stralloc line;
  awk_cell* f; /* f[0]..f[cap-1]; f[0] mirrors $0 (rebuilt lazily) */
  size_t nf, cap;
  int split_done;
  int dirty; /* $0 needs rebuilding from f[1..nf] with OFS */
};

void awk_rec_init(struct awk_rec* r);
void awk_rec_free(struct awk_rec* r);
void awk_rec_setline(struct awk_state* st, const char* s, size_t n); /* new $0, resplit lazily */
void awk_rec_ensure_split(struct awk_state* st);
awk_cell* awk_rec_field(struct awk_state* st, long n); /* n==0 rebuilds $0 first if dirty */
void awk_rec_setnf(struct awk_state* st, long nf);     /* NF = n: grow/shrink f[], mark dirty */

/* $n=... for n>=1: grows NF/fills any gap with "" as needed, marks $0
   dirty, and returns the field's cell for the caller to overwrite.
   ($0=... goes through awk_rec_setline() instead -- a different
   value: a full record to resplit, not one field to join back in.) */
awk_cell* awk_rec_field_for_write(struct awk_state* st, long n);

/* a field/split() piece: [s, s+len) points *into the input `s`
   awk_split() was given* -- valid only as long as that buffer is
   (callers copy out immediately, e.g. into a cell or a hashmap entry). */
struct awk_span {
  const char* s;
  size_t len;
};

/* awk_split: splits s[0..n) per fs[0..fslen) (POSIX field-splitting
   rules, reused by both $0's own split and the split() builtin):
   fslen==0 splits into individual bytes (GNU extension), fslen==1 and
   fs[0]==' ' is the default (runs of blanks, trimmed), fslen==1
   otherwise splits on each literal occurrence of that byte (empty
   fields allowed), fslen>1 treats fs as an ERE via dfa_search.
   *out and *outcap grow (alloc_re) as needed; returns the field count. */
size_t awk_split(struct awk_state* st, const char* s, size_t n, const char* fs, size_t fslen,
                  struct awk_span** out, size_t* outcap);

/* the ERE-splitting half of awk_split(), exposed separately for
   split()'s regex-*literal* third argument, which hands over an
   already-compiled struct dfa instead of a string to compile. */
size_t awk_split_re(struct awk_state* st, const char* s, size_t n, struct dfa* re,
                     struct awk_span** out, size_t* outcap);

/* ---- call frames -------------------------------------------------------- */

#define AWK_MAXDEPTH 1000

struct awk_frame {
  awk_cell** slot;   /* [i]: aliases a caller cell, or &local[i] */
  awk_cell* local;   /* backing storage for by-value/fresh params */
  size_t n;
};

/* ---- open I/O targets (redirections, getline<file, cmd|getline) ------- */

struct awk_stream {
  char* name;
  int is_write; /* 0 = input (getline<file / cmd|getline), 1 = output */
  int is_cmd;   /* the shell-command variants */
  void* h;      /* io->open_read()/open_write()/run_shell() handle */
  struct awk_state* st; /* back-pointer, so the buffer's read callback can reach io->read */
  buffer rb;             /* input streams read through a buffer of our own on top of io->read */
  char rbuf[4096];
  int eof;
};

/* ---- interpreter state -------------------------------------------------- */

/* exec() control-flow signal */
enum { CF_NORMAL = 0, CF_BREAK, CF_CONTINUE, CF_NEXT, CF_NEXTFILE, CF_EXIT, CF_RETURN };

/* st->unwind: a stop-everything signal that reaches statement-level
   exec() from inside expression evaluation (a runtime error, or
   `exit` executed by a user function called from within an
   expression) where there is no control-flow return channel.
   Checked at loop/block boundaries and treated exactly like CF_EXIT,
   except UNWIND_ERROR additionally skips END (awk_run.c). */
enum { UNWIND_NONE = 0, UNWIND_EXIT, UNWIND_ERROR };

struct awk_state {
  struct awk_prog* prog;
  const struct awk_io* io;
  arena tmp; /* statement-lifetime temporaries, reset after each statement */

  awk_cell* globals; /* prog->nglobals entries */
  struct awk_rec rec;

  struct awk_frame* frame; /* current call frame, or NULL at top level */
  size_t depth;

  awk_cell retval; /* CF_RETURN's value */
  int exit_status;
  int unwind; /* UNWIND_* */

  /* range-pattern "currently inside" flags, one per rule */
  unsigned char* range_active;

  /* open redirections/getline streams, by target-string identity */
  struct awk_stream* streams;
  size_t nstreams, streamcap;

  /* main-input driving state */
  long cur_argi;    /* next ARGV index to consider (1-based) */
  struct awk_stream cur; /* the file operand currently being read, reusing
                            struct awk_stream's buffered reader */
  int cur_open;     /* is `cur` live right now? */
  int any_input_used;

  unsigned long seed; /* rand()/srand() LCG state */
  unsigned long last_seed;
};

/* ---- shared helpers used across files ---------------------------------- */

/* formats n per fmt (CONVFMT/OFMT, or "%d"-equivalent for an integer
   value) into st->tmp (the statement arena) and returns it; valid
   until the next statement boundary resets st->tmp. */
const char* awk_num2str(struct awk_state* st, double n, const char* fmt);
awk_cell* awk_global(struct awk_state* st, long idx);
hashmap* awk_array_of(struct awk_state* st, awk_cell* c); /* turns UNINIT into ARRAY lazily */

/* lvalue: a resolved, assignable location. A_FIELD gets special
   handling (kind LV_FIELD) instead of a raw cell pointer because
   assigning $0 or $n>0 has side effects (resplit / NF growth /
   $0-rebuild) beyond "store a value". */
enum { LV_CELL, LV_FIELD };
struct awk_lvalue {
  int kind;
  awk_cell* cell; /* LV_CELL */
  long fieldn;    /* LV_FIELD */
};
struct awk_lvalue awk_lvalue(struct awk_state* st, struct anode* n);
awk_cell* awk_lvalue_get(struct awk_state* st, struct awk_lvalue* lv);
void awk_lvalue_set_num(struct awk_state* st, struct awk_lvalue* lv, double n);
void awk_lvalue_set_str(struct awk_state* st, struct awk_lvalue* lv, const char* s, size_t len, int strnum);

awk_cell awk_eval(struct awk_state* st, struct anode* n);
int awk_exec(struct awk_state* st, struct anode* n);
int awk_call_user(struct awk_state* st, struct awk_func* fn, struct anode* args, awk_cell* out);
awk_cell awk_call_builtin(struct awk_state* st, struct anode* n);

int awk_getrecord(struct awk_state* st); /* advances to the next $0 from ARGV/stdin; 0 = EOF, <0 = error */
int awk_getline_from(struct awk_state* st, struct anode* n, awk_cell** setvar); /* implements A_GETLINE */

/* NAME=VALUE, from -v or a file-operand position (POSIX processes
   both identically, including value escape processing); returns 1 if
   `s` had that shape (whether or not NAME turned out to be a global
   the program actually references -- either way it is consumed, not
   treated as a filename), 0 if `s` is an ordinary operand. */
int awk_apply_assignment(struct awk_state* st, const char* s);

void awk_runtime_error(struct awk_state* st, const char* msg);
void awk_output(struct awk_state* st, const char* s, size_t n);
void awk_output_err(struct awk_state* st, const char* s, size_t n);
struct awk_stream* awk_stream_for_write(struct awk_state* st, const char* name, int append, int is_cmd);
struct awk_stream* awk_stream_for_read(struct awk_state* st, const char* name, int is_cmd);
int awk_stream_close(struct awk_state* st, const char* name); /* 1 = was open, 0 = not found */
void awk_streams_close_all(struct awk_state* st);
void awk_streams_flush_all(struct awk_state* st);

int awk_sprintf(struct awk_state* st, stralloc* out, const char* fmt, size_t fmtlen, struct anode* args);

#endif
