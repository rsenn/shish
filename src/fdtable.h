#ifndef FDTABLE_H
#define FDTABLE_H

#ifdef FD_SETSIZE
#define FDTABLE_SIZE FD_SETSIZE
#else
#define FDTABLE_SIZE 1024
#endif

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

struct fd;
struct fdstack;

extern int fdtable_top; /* highest occupied vfd + 1 */
extern int fdtable_bottom; /* lowest occupied vfd */
extern struct fd** const fdtable;
extern struct fd** fdtable_pos;

#define fdtable_foreach(i)                                                         \
  for(i = fdtable_bottom; i < fdtable_top; i++)                                         \
    if(fdtable[i])
#define fdtable_foreach_p(i, p)                                                    \
  for(i = fdtable_bottom; i < fdtable_top; i++)                                         \
    if(((p) = fdtable[i]))

int fdtable_check(int e);
int fdtable_close(int e, int flags);
int fdtable_dup(struct fd* fd, int flags);
int fdtable_exec(void);
int fdtable_gap(int e, int force);
int fdtable_here(struct fd* fd, int flags);
int fdtable_lazy(int e, int flags);
int fdtable_open(struct fd* fd, int force);
int fdtable_resolve(struct fd* fd, int force);
int fdtable_wish(int e, int flags);
struct fd* fdtable_newfd(int n, struct fdstack* st, int mode);
#ifdef BUFFER_H
void fdtable_dump(buffer* b);
#endif
void fdtable_link(struct fd* fd);
void fdtable_track(int n, int flags);
void fdtable_unexpected(int e, int u, int flags);
void fdtable_unlink(struct fd* fd);
void fdtable_up(void);

#endif /* FDTABLE_H */
