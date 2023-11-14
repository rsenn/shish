#ifndef FDTABLE_H
#define FDTABLE_H

#include "fd.h"

#define FDTABLE_SIZE FD_MAX

#define fdtable_ok(e) ((e) >= -1 && (e) < FDTABLE_S IZE)
#define efdtable_ok(e) ((e) >= 0 && (e) < FDTABLE_SIZE)

#define FDTABLE_LAZY 0
#define FDTABLE_MOVE 1
#define FDTABLE_FORCE 2
#define FDTABLE_NOCLOSE 4
#define FDTABLE_CLOSE 8

#define FDTABLE_FD (FDTABLE_MOVE | FDTABLE_FORCE)

/* return values */
#define FDTABLE_PENDING -3
#define FDTABLE_ERROR -2
#define FDTABLE_DONE -1

struct fdstack;

/* fdtable_top:    highest occupied vfd + 1
 * fdtable_bottom: lowest occupied vfd  */
extern int fdtable_top, fdtable_bottom;
extern struct fd** const fdtable;
extern struct fd** fdtable_pos;

/* current standard fds */
#define fd_src fdtable[STDSRC_FILENO]
#define fd_in fdtable[STDIN_FILENO]
#define fd_out fdtable[STDOUT_FILENO]
#define fd_err fdtable[STDERR_FILENO]

#define fdtable_foreach(i) \
  for(i = fdtable_bottom; i < fdtable_top; i++) \
    if(fdtable[i])
#define fdtable_foreach_p(i, p) \
  for(i = fdtable_bottom; i < fdtable_top; i++) \
    if(((p) = fdtable[i]))

int fdtable_check(int e);
int fdtable_close(int e, int flags);
int fdtable_dup(struct fd*, int flags);
int fdtable_exec(void);
int fdtable_gap(int e, int force);
int fdtable_here(struct fd*, int flags);
int fdtable_lazy(int e, int flags);
int fdtable_open(struct fd*, int force);
int fdtable_resolve(struct fd*, int force);
int fdtable_wish(int e, int flags);
struct fd* fdtable_newfd(int n, struct fdstack* st, int mode);
#ifdef BUFFER_H
void fdtable_dump(buffer* b);
#endif
void fdtable_link(struct fd*);
void fdtable_track(int n, int flags);
void fdtable_unexpected(int e, int u, int flags);
void fdtable_unlink(struct fd*);
void fdtable_up(void);

#endif /* FDTABLE_H */
