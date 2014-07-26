#ifndef REDIR_H
#define REDIR_H

#include "buffer.h"
#include "stralloc.h"

#include "fd.h"

/* redirection types
 * ----------------------------------------------------------------------- */

/* bit 0: direction of the redirection, either R_IN or R_OUT */
#define R_IN      0x01
#define R_OUT     0x02

/* bits 1-3: type of redirection, R_OPEN, R_DUP or R_HERE.
   R_OPEN and R_DUP can be combined (fd<>file redirections */
#define R_ACT     0x1c

#define R_OPEN    0x04
#define R_DUP     0x08
#define R_HERE    0x10

/* bit 5: stripped here-document */
#define R_STRIP   0x20

/* bit 6: append mode for output redirections */
#define R_APPEND  0x40

/* bit 7: clobbering for output redirections */
#define R_CLOBBER 0x80

#define R_NOW    0x100 /* immediately open */

extern struct nredir *redir_list;
extern char           redir_tempname[];

struct parser;

int redir_dup(struct nredir *nredir, stralloc *sa);
int redir_eval(struct nredir *nredir, struct fd *fd, int rfl);
int redir_here(struct nredir *nredir, stralloc *sa);
int redir_open(struct nredir *nredir, stralloc *sa);
int redir_parse(struct parser *p, int rf, int fd);
void redir_addhere(struct nredir *nredir);
void redir_source(void);

#endif /* REDIR_H */
