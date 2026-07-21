#ifndef EXEC_H
#define EXEC_H

#include "../lib/uint32.h"
#include <stdlib.h>

enum hash { H_PROGRAM = 0, H_EXEC = 1, H_SBUILTIN = 2, H_BUILTIN = 4, H_FUNCTION = 8 };

enum execflag { X_EXEC = 1, X_NOWAIT = 2 };

struct command {
  enum hash id;
  union {
    char* path;
    union node* fn;
    struct builtin_cmd* builtin;
    void* ptr;
  };
};

struct exechash {
  struct exechash* next;
  uint32 hash;
  unsigned int hits;
  char* name; /* name of builtin, function or command */
  struct command cmd;
  int mask;
};

#define EXEC_HASHBITS 5
#define EXEC_HASHSIZE (1 << EXEC_HASHBITS)
#define EXEC_HASHMASK (EXEC_HASHSIZE - 1)

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#define EXIT_ERROR 2
#define EXIT_NOEXEC 126
#define EXIT_NOTFOUND 127
#define EXIT_SYNERROR (256 + EXIT_ERROR)
#define EXIT_EXPERROR EXIT_ERROR
#define EXIT_ASSGNERR EXIT_ERROR
#define EXIT_REDIRERR EXIT_ERROR

extern struct exechash* exec_hashtbl[EXEC_HASHSIZE];

/* errno from the access()/stat() call that made exec_hash()/exec_path()
   give up on a candidate (ENOENT if nothing matching PATH existed at
   all, EACCES/EISDIR if something matched but couldn't be executed).
   The caller can't rely on plain "errno" for this by the time it
   notices cmd.ptr == NULL -- redirections, variable expansion and
   other syscalls run in between and routinely clobber it. */
extern int exec_lasterrno;

char* exec_check(char* path);
char* exec_path(char* name);
int exec_command(struct command* cmd, int argc, char** argv, enum execflag);
int exec_error(void);
int exec_program(char* path, char** argv, enum execflag);
uint32 exec_hashstr(const char* s);
struct exechash* exec_create(char* name, uint32 hash);
struct exechash* exec_lookup(char* name, uint32* hashptr);
struct command exec_hash(char* name, int mask);
struct command exec_search(char* name, int mask);
int exec_type(char* name, int mask, int force_path, int type_name);

union node;

/* Snapshot of the function list (head + per-node next pointers) so a subshell
   can restore the parent's view on exit. functions defined or redefined inside
   the subshell get unlinked from the parent's list on restore; nodes are not
   freed because the exec_hash cache may still reference them. */
struct func_snapshot {
  size_t n;
  union node** nodes;
};

extern int exec_subshell_depth;

void exec_functions_save(struct func_snapshot* snap);
void exec_functions_restore(struct func_snapshot* snap);

#endif /* EXEC_H */
