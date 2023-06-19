#ifndef VAR_H
#define VAR_H

#include "../lib/stralloc.h"
#include "../lib/uint64.h"
#include <stdlib.h>

#define VAR_HASH uint64
#define VAR_BITS (sizeof(VAR_HASH) * 8)
#define VAR_MASK (VAR_BITS - 1)
#define VAR_HALF (VAR_BITS / 2)
#define VAR_CHAR (6)
#define VAR_CHARS (VAR_BITS / VAR_CHAR)

#define V_DEFAULT 0x00
#define V_FREE 0x01

#define V_FREESTR 0x02 /* always free the supplied string */
#define V_ZEROSA 0x04  /* allowed to free it, but maybe only zeroing */

#define V_EXPORT 0x08
#define V_LOCAL 0x10
#define V_UNSET 0x20
#define V_INIT 0x40 /* do only set when unset */
#define V_READONLY 0x80

#define VAR_FLAG_NAMES \
  ((const char* const[]){"FREE", \
                         "FREESTR", \
                         "ZEROSA", \
                         "EXPORT", \
                         "LOCAL", \
                         "UNSET", \
                         "INIT", \
                         "READONLY"})

struct search;

struct var {
  struct var* bnext; /* for the hash bucket list in a vartab struct */
  struct var** blink;
  struct var* gnext;  /* for the sorted list in the global vartab */
  struct var** glink; /* position within the global sorted list */
  union {
    stralloc sa;
    struct {
      char* s;
      size_t total;
      size_t a;
    };
  };
  size_t len;    /* name length */
  size_t offset; /* offset to value start */
  int flags;
  VAR_HASH lexhash;
  VAR_HASH rndhash;
  struct var* parent;
  struct var* child;
  struct vartab* table;
};

extern struct var* var_list;
extern size_t var_exported;

#include "vartab.h"

VAR_HASH var_hsearch(struct search* context);
char** var_export(char** dest);

const char* var_get(const char* v, size_t* offset);
const char*
var_setv(const char* name, const char* value, size_t vlen, int flags);
const char* var_setvint(const char* v, int i, int flags);
const char* var_setvsa(const char* name, stralloc* sa, int flags);
const char* var_value(const char* v, size_t* plen);
const char* var_vdefault(const char* v, const char* def, size_t* lenp);

int var_chflg(char* v, int flags, int set);
int var_valid(const char* v);

struct var* var_copys(const char* s, int flags);
struct var* var_create(const char* v, int flags);
struct var* var_import(const char* v, int flags, struct var* var);
struct var* var_init(const char* v, struct var* var, struct search* context);
struct var* var_search(const char* v, struct search* context);
struct var* var_set(char* v, int flags);
struct var* var_setsa(stralloc* sa, int flags);

size_t var_bsearch(struct search* context);
size_t var_count(int flags);
size_t var_lexhash(const char* v, VAR_HASH* h);
size_t var_rndhash(const char* v, VAR_HASH* h);
size_t var_vlen(const char* v);

void var_cleanup(struct var* var);
void var_dump(struct var* var);
void var_print(struct var* var, int flags);
int var_unset(char* v);

#endif /* VAR_H */
