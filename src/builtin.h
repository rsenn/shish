#ifndef BUILTIN_H
#define BUILTIN_H

#include <stdlib.h>

#define B_DEFAULT 0x00
#define B_SPECIAL 0x01
#define B_EXEC 0x02

typedef int(builtin_func)(int argc, char** argv);

struct builtin_cmd {
  const char* name;
  builtin_func* fn;
  int flags;
  const char* args;
};

extern struct builtin_cmd builtin_table[];

struct builtin_cmd* builtin_search(const char* name, int flags);

int builtin_errmsgn(char** argv, const char* s, unsigned int n, char* msg);
int builtin_errmsg(char** argv, char* s, char* msg);
int builtin_error(char** argv, char* s);
int builtin_invopt(char** argv);

/* builtin prototypes
 * ----------------------------------------------------------------------- */
int builtin_source(int argc, char** argv);
int builtin_true(int argc, char** argv);
int builtin_basename(int argc, char** argv);
int builtin_break(int argc, char** argv);
int builtin_cd(int argc, char** argv);
int builtin_dirname(int argc, char** argv);
int builtin_dump(int argc, char** argv);
int builtin_echo(int argc, char** argv);
int builtin_eval(int argc, char** argv);
int builtin_exec(int argc, char** argv);
int builtin_exit(int argc, char** argv);
int builtin_export(int argc, char** argv);
int builtin_expr(int argc, char** argv);
int builtin_false(int argc, char** argv);
int builtin_fdtable(int argc, char** argv);
int builtin_hash(int argc, char** argv);
int builtin_help(int argc, char** argv);
int builtin_history(int argc, char** argv);
int builtin_hostname(int argc, char** argv);
int builtin_mkdir(int argc, char** argv);
int builtin_ln(int argc, char** argv);
int builtin_pwd(int argc, char** argv);
int builtin_set(int argc, char** argv);
int builtin_shift(int argc, char** argv);
int builtin_test(int argc, char** argv);
int builtin_type(int argc, char** argv);
int builtin_unset(int argc, char** argv);

#else
#warning "builtin.h included twice"
#endif /* BUILTIN_H */
