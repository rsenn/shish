#ifndef VARTAB_H
#define VARTAB_H

#include "uint64.h"

#define VARTAB_BUCKETS (61LL)

struct vartab;

#include "var.h"

struct search {
  struct var   **pos;
  struct var   **closest;
  const char    *name;
  VAR_HASH       lexhash;
  VAR_HASH       rndhash;
  VAR_HASH       hdist;
  VAR_HASH       bdist;
  unsigned long  bucket;
  unsigned long  len;    /* length of wanted var */
  int            global; /* go through global sorted list */
  int            exact;  /* want an exact match only */
};

struct vartab {
  struct var    *table[VARTAB_BUCKETS];
  struct var   **pos;
  struct vartab *parent;
  unsigned int   level;
};

extern struct vartab  vartab_root;
extern struct vartab *varstack;

unsigned long vartab_hash(struct vartab *vartab, const char *v, struct search *context);
struct var *vartab_search(struct vartab *vartab, const char *v, struct search *context);

void vartab_add(struct vartab *vartab, struct var *var, struct search *context);

void vartab_push(struct vartab *vartab);
void vartab_pop(struct vartab *vartab);
void vartab_cleanup(struct vartab *vartab);
  

char **vartab_export(void);
void vartab_print(int flags);
void vartab_dump(struct vartab *vartab);

#endif /* VARTAB_H */
