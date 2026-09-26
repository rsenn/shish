#ifndef BRE_H
#define BRE_H

#include <stddef.h>
#include "stralloc.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BRE_MAXGROUPS 9

struct bre_ctx {
  const char* pat_start;
  const char* open_ptr[BRE_MAXGROUPS];
  const char* close_ptr[BRE_MAXGROUPS];
  int nopen;
  const char* gstart[BRE_MAXGROUPS];
  const char* gend[BRE_MAXGROUPS];
};

enum vm_opcode {
  OP_HALT = 0,
  OP_MATCH,
  OP_NOT,
  OP_BRAF,
  OP_BRAT,
  OP_ADDR_RANGE,
  OP_PRINT,
  OP_DELETE,
  OP_SUBST
};

struct vm_instruction {
  unsigned char op;
  int arg_int;
  const char* str_ptr;
  const char* str_aux;
};

struct vm_bytecode {
  struct vm_instruction* code;
  size_t count;
  size_t capacity;
};

struct filter_context {
  int reg_bool;
  unsigned long line_no;
  int in_range;
  stralloc pattern_space;
  stralloc hold_space;
  stralloc output;
  struct bre_ctx bre;
};

void bre_prepass(struct bre_ctx* ctx, const char* pat);
void bre_compile(struct bre_ctx* ctx, const char* pat);
int bre_group_by_open(struct bre_ctx* ctx, const char* pat);
int bre_group_by_close(struct bre_ctx* ctx, const char* pat);
int bre_class_match(const char* name, size_t namelen, int c);
size_t bre_atom_len(const char* pat);
int bre_bracket_matches(const char* pat, size_t alen, int c);
int bre_atom_matches(const char* pat, size_t alen, int c);
const char*
bre_match_star(struct bre_ctx* ctx, const char* atom, size_t alen, const char* rest, const char* s);
const char* bre_match_rec(struct bre_ctx* ctx, const char* pat, const char* s);
int bre_match_exec(struct bre_ctx* ctx, const char* pat, const char* str);
void bre_subst_expand(const char* repl, const struct bre_ctx* ctx, stralloc* out);
int filter_vm_execute(const struct vm_bytecode* prog, struct filter_context* ctx);

#ifdef __cplusplus
}
#endif

#endif