#ifndef AST_H
#define AST_H

#include "json.h"
#include "../lib/uint64.h"

union node;
struct location;

/* parse tree -> JSON (see doc/ast-serialization.md); plain text, no colour
 * ----------------------------------------------------------------------- */
struct ast {
  struct json j;
  unsigned loc : 1;         /* "loc": "file:line:col" (default on) */
  unsigned range : 1;       /* "range": [start,end] byte offsets (default off) */
  unsigned no_position : 1; /* neither of the two, whatever loc and range say */
};

void ast_init(struct ast* a, buffer* b, int indent);
void ast_node(struct ast* a, union node* node);
void ast_list(struct ast* a, union node* list);
void ast_tree(struct ast* a, union node* list);   /* a whole script: ast_list() + newline, flushed */
void ast_ndjson(struct ast* a, union node* list); /* one compact object + newline per top-level node */

/* "loc" and/or "range", for a token len bytes long at *loc */
void ast_pos(struct ast* a, const struct location* loc, size_t len);

extern const char* const ast_names[]; /* node id -> JSON "kind" */
extern const unsigned ast_names_count;

/* "k": [ ...nodes ] -- an empty list stays an empty array */
static inline void
ast_kids(struct ast* a, const char* k, union node* list) {
  json_key(&a->j, k);
  ast_list(a, list);
}

/* "k": { node }, left out when node is NULL */
static inline void
ast_kid(struct ast* a, const char* k, union node* node) {
  if(node) {
    json_key(&a->j, k);
    ast_node(a, node);
  }
}

static inline void
ast_bgnd(struct ast* a, unsigned bit) {
  json_kuint(&a->j, "bgnd", bit);
}

/* "rdir": [ ... ], left out when there are no redirections */
static inline void
ast_rdir(struct ast* a, union node* list) {
  if(list)
    ast_kids(a, "rdir", list);
}

#endif /* AST_H */
