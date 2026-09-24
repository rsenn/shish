#include "../builtin.h"
#include "../../lib/alloc.h"
#include "../../lib/shell.h"
#include "../../lib/str.h"
#include "../../lib/byte.h"
#include "../var.h"
#include "../vartab.h"
#include "../parse.h"
#include "../fdtable.h"

#ifndef offsetof
#define offsetof(type, field) ((size_t)&((type*)0)->field)
#endif

static int
alias_valid(const char* v) {
  size_t i;

  for(i = 0; v[i] && v[i] != '='; i++)
    if(!parse_isname(v[i], i) && !(i > 0 && v[i] == '-'))
      return 0;

  return 1;
}

static struct alias*
alias_new(const char* str) {
  size_t len = str_len(str);
  struct alias* a;

  if((a = alloc(offsetof(struct alias, def) + len + 1))) {
    a->namelen = str_chr(str, '=');
    a->codelen = len - (a->namelen + 1);
    a->next = 0;
    byte_copy(a->def, len + 1, str);
  }

  return a;
}

static void
alias_remove(struct alias** aptr) {
  struct alias* a = *aptr;
  *aptr = a->next;
  alloc_free(a);
}

static inline void
alias_insert(struct alias** aptr, struct alias* a) {
  a->next = *aptr;
  *aptr = a;
}

/* print one alias, safe to feed back to the shell even if VALUE itself
 * contains a single quote ("'" -> "'\''"). "-p" writes the full
 * 'alias NAME=VALUE' command; printing a specific name (or every alias
 * with no operand at all) writes bare 'NAME=VALUE', since that form is
 * what round-trips through "eval alias $(alias name)" and through
 * "alias -- $(alias)" split on newlines -- a leading "alias " word
 * there would be read back as an operand of its own.
 * ----------------------------------------------------------------------- */
static void
alias_print(struct alias* a, int prefix) {
  size_t codelen;
  const char* code = alias_code(a, &codelen);

  if(prefix)
    buffer_puts(fd_out->w, "alias ");

  buffer_put(fd_out->w, a->def, a->namelen);
  buffer_puts(fd_out->w, "='");

  while(codelen) {
    size_t i = byte_chr(code, codelen, '\'');

    buffer_put(fd_out->w, code, i);

    if(i < codelen) {
      buffer_puts(fd_out->w, "'\\''");
      i++;
    }

    code += i;
    codelen -= i;
  }

  buffer_putc(fd_out->w, '\'');
  buffer_putnlflush(fd_out->w);
}

static struct alias**
alias_search(const char* str) {
  size_t len = str_chr(str, '=');
  struct alias** aptr;

  for(aptr = &parse_aliases; *aptr; aptr = &(*aptr)->next)
    if((*aptr)->namelen == len && byte_equal((*aptr)->def, len, str))
      break;

  return aptr;
}

/* alias built-in
 *
 * ----------------------------------------------------------------------- */
const char help_alias[] = "    Define or print shell aliases.\n"
                          "\n"
                          "    -p              print every alias as 'alias NAME=VALUE'\n"
                          "    name=value      define or redefine an alias\n"
                          "    name            print that one alias's definition\n";

const char help_unalias[] = "    Remove shell aliases.\n"
                            "\n"
                            "    -a              remove every defined alias\n"
                            "    name            remove the named alias\n";

static void
alias_print_all(int prefix) {
  struct alias* alias;

  for(alias = parse_aliases; alias; alias = alias->next)
    alias_print(alias, prefix);
}

int
builtin_alias(int argc, char* argv[]) {
  int c, print = 0, ret = 0;
  char** argp;

  while((c = shell_getopt(argc, argv, "p")) > 0) {
    switch(c) {
      case 'p': print = 1; break;
      default: builtin_invopt(argv); return 1;
    }
  }

  argp = &argv[shell_optind];

  /* "-p", or no operands at all: print every alias */
  if(print || *argp == NULL) {
    alias_print_all(print);
    return 0;
  }

  /* add, or print, each named alias */
  for(; *argp; argp++) {
    struct alias **aptr, *alias;

    if(!alias_valid(*argp)) {
      builtin_errmsg(argv, *argp, "not a valid alias name");
      ret = 1;
      continue;
    }

    /* a bare name (no "=") prints that one alias's definition */
    if(!(*argp)[str_chr(*argp, '=')]) {
      if(!(alias = *alias_search(*argp))) {
        builtin_errmsg(argv, *argp, "no such alias");
        ret = 1;
      } else {
        alias_print(alias, 0);
      }

      continue;
    }

    if(*(aptr = alias_search(*argp)))
      alias_remove(aptr);

    if((alias = alias_new(*argp)))
      alias_insert(aptr, alias);
  }

  return ret;
}

/* unalias built-in
 * ----------------------------------------------------------------------- */
int
builtin_unalias(int argc, char* argv[]) {
  int c, ret = 0, all = 0;
  char** argp;

  while((c = shell_getopt(argc, argv, "a")) > 0) {
    switch(c) {
      case 'a': all = 1; break;
      default: builtin_invopt(argv); return 1;
    }
  }

  if(all) {
    while(parse_aliases)
      alias_remove(&parse_aliases);

    return 0;
  }

  for(argp = &argv[shell_optind]; *argp; argp++) {
    struct alias** aptr = alias_search(*argp);

    if(!*aptr) {
      builtin_errmsg(argv, *argp, "no such alias");
      ret = 1;
      continue;
    }

    alias_remove(aptr);
  }

  return ret;
}
