#ifndef VARTAB_H
#define VARTAB_H

#include "../lib/uint64.h"

#define VARTAB_BUCKETS 64

struct vartab;

#include "var.h"

struct search {
  struct var** pos;
  struct var** closest;
  const char* name;
  VAR_HASH lexhash;
  VAR_HASH rndhash;
  VAR_HASH hdist;
  VAR_HASH bdist;
  size_t bucket;
  size_t len; /* length of wanted var */
  int global; /* go through global sorted list */
  int exact;  /* want an exact match only */
};

struct vartab {
  struct var* table[VARTAB_BUCKETS];
  struct var** pos;
  struct vartab* parent;
  unsigned int level : 31;
  unsigned int function : 1;
};

extern struct vartab vartab_root;
extern struct vartab* varstack;

size_t vartab_hash(const char* v, struct search* context);
struct var* vartab_search(struct vartab* vartab,
                          const char* v,
                          struct search* context);

void vartab_add(struct vartab* vartab, struct var* var, struct search* context);

void vartab_push(struct vartab* vartab, int function);
void vartab_pop(struct vartab* vartab);
void vartab_cleanup(struct vartab* vartab);

char** vartab_export(void);
void vartab_print(int flags);
void vartab_dump(struct vartab* vartab, int argc, char* argv[]);

#endif /* VARTAB_H */
