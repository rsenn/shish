/* $Id: fdstack.h,v 1.8 2005/04/08 09:34:09 smoli Exp $
 * ----------------------------------------------------------------------- */
#ifndef FDSTACK_H
#define FDSTACK_H

struct fd;

struct fdstack {
  struct fd* list; /* is sorted by efd */
  struct fdstack* parent;
  unsigned int level; /* stack level -> root is 0 */
};

#include "fd.h"

extern struct fdstack fdstack_root;
extern struct fdstack* fdstack;

int fdstack_data(void);
int fdstack_pipe(unsigned int n, struct fd*);
struct fd* fdstack_search(struct fdstack* st, int n);
unsigned int fdstack_npipes(int mode);
unsigned int fdstack_unref(struct fd* olddup);
void fdstack_dump(buffer* b);
void fdstack_flatten(void);
void fdstack_link(struct fdstack* st, struct fd* fd);
void fdstack_pop(struct fdstack* st);
void fdstack_push(struct fdstack* st);
void fdstack_unlink(struct fd* fd);
void fdstack_update(struct fd* dup);

/* allocate space for pipe fds */
#define FDSTACK_ALLOC_SIZE(n) ((D_SIZE + FD_BUFSIZE / 2) * (n))
#define fdstack_alloc(n) alloca(FDSTACK_ALLOC_SIZE(n))

#endif /* FDSTACK_H */
