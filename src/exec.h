#ifndef EXEC_H
#define EXEC_H

#include "../lib/uint32.h"
#include <stdlib.h>

enum hash_id { H_PROGRAM = 0, H_EXEC, H_SBUILTIN, H_BUILTIN, H_FUNCTION };

union command {
  char* path;
  union node* fn;
  struct builtin_cmd* builtin;
  void* ptr;
};

struct exechash {
  struct exechash* next;
  uint32 hash;
  unsigned int hits;
  char* name; /* name of builtin, function or command */
  enum hash_id id;
  union command cmd;
};

#define EXEC_HASHBITS 5
#define EXEC_HASHSIZE (1 << EXEC_HASHBITS)
#define EXEC_HASHMASK (EXEC_HASHSIZE - 1)

extern struct exechash* exec_hashtbl[EXEC_HASHSIZE];

char* exec_check(char* path);
char* exec_path(char* name);
int exec_command(enum hash_id id, union command* cmd, int argc, char** argv, int exec, union node* redir);
int exec_error(void);
int exec_program(char* path, char** argv, int exec, union node* redir);
uint32 exec_hashstr(const char* s);
struct exechash* exec_create(char* name, uint32 hash);
struct exechash* exec_search(char* name, uint32* hashptr);
union command exec_hash(char* name, enum hash_id* idptr);

#endif /* EXEC_H */
