#include "../builtin.h"
#include "../fd.h"
#include "../fdtable.h"
#include "../sh.h"
#include "../tree.h"
#include "../../lib/shell.h"
#include "../../lib/str.h"
#include "../vartab.h"

extern union node* functions;

/* set arguments of flags
 * ----------------------------------------------------------------------- */
const char help_set[] =
    "    Set shell options and/or positional parameters.\n"
    "\n"
    "    -a              export every variable assignment\n"
    "    -e              exit if a simple command fails\n"
    "    -f              disable pathname expansion (globbing)\n"
    "    -h              remember command locations as they're looked up\n"
    "    -m              enable job control (monitor mode)\n"
    "    -n              read commands but don't execute them\n"
    "    -u              treat unset variables as an error on expansion\n"
    "    -x              print each command before running it\n"
    "    -B              enable brace expansion\n"
    "    -C              don't let '>' clobber an existing file\n"
    "    -H              enable history expansion ('!')\n"
    "    -o name         same as the matching letter option above, by name\n"
    "    -o              (no name) print every option's current state\n"
    "    +option         turn the option off instead of on\n"
    "    arg             new positional parameters ($1, $2, ...)\n"
    "\n"
    "    With no options or arguments, print every variable and function.\n";

/* every letter option also reachable via "-o name"/"+o name" -- kept
 * as a name->letter table rather than one bitfield pointer per name
 * because struct shopt's members are 1-bit bitfields, which C simply
 * doesn't allow taking the address of. Declared alphabetically so
 * "set -o"'s listing comes out sorted for free. "unset" is spelled
 * "nounset" here on purpose: that's the POSIX/bash "-o" name for it
 * (matching "-u"), even though the C struct field is just "unset". */
static const struct {
  const char* name;
  char letter;
} set_longopts[] = {
    {"allexport", 'a'},  {"braceexpand", 'B'}, {"errexit", 'e'},  {"hashall", 'h'},
    {"histexpand", 'H'}, {"monitor", 'm'},     {"noclobber", 'C'}, {"noexec", 'n'},
    {"noglob", 'f'},     {"nounset", 'u'},     {"xtrace", 'x'},
};

#define SET_LONGOPTS_N (sizeof(set_longopts) / sizeof(set_longopts[0]))

static int
set_apply(struct shopt* opts, int letter, int on) {
  switch(letter) {
    case 'a': opts->allexport = on; return 1;
    case 'e': opts->errexit = on; return 1;
    case 'f': opts->noglob = on; return 1;
    case 'h': opts->hashall = on; return 1;
    case 'm': opts->monitor = on; return 1;
    case 'n': opts->noexec = on; return 1;
    case 'u': opts->unset = on; return 1;
    case 'x': opts->xtrace = on; return 1;
    case 'B': opts->braceexpand = on; return 1;
    case 'C': opts->noclobber = on; return 1;
    case 'H': opts->histexpand = on; return 1;
    default: return 0;
  }
}

static int
set_get(const struct shopt* opts, int letter) {
  switch(letter) {
    case 'a': return opts->allexport;
    case 'e': return opts->errexit;
    case 'f': return opts->noglob;
    case 'h': return opts->hashall;
    case 'm': return opts->monitor;
    case 'n': return opts->noexec;
    case 'u': return opts->unset;
    case 'x': return opts->xtrace;
    case 'B': return opts->braceexpand;
    case 'C': return opts->noclobber;
    case 'H': return opts->histexpand;
    default: return 0;
  }
}

/* bare "-o": human-readable "name    on/off" per line (POSIX leaves
 * the exact format unspecified). Bare "+o": one "set -o name"/
 * "set +o name" line per option, meant to be reusable as input to
 * restore the same settings later -- POSIX does specify that much. */
static void
set_print_all(const struct shopt* opts, int reusable) {
  size_t i;

  for(i = 0; i < SET_LONGOPTS_N; i++) {
    int on = set_get(opts, set_longopts[i].letter);

    if(reusable) {
      buffer_puts(fd_out->w, "set ");
      buffer_puts(fd_out->w, on ? "-o " : "+o ");
      buffer_puts(fd_out->w, set_longopts[i].name);
    } else {
      size_t len = str_len(set_longopts[i].name);

      buffer_puts(fd_out->w, set_longopts[i].name);

      while(len++ < 16)
        buffer_putc(fd_out->w, ' ');

      buffer_puts(fd_out->w, on ? "on" : "off");
    }

    buffer_putnlflush(fd_out->w);
  }
}

int
builtin_set(int argc, char* argv[]) {
  int c;
  struct shopt opts = sh->opts;
  struct optstate opt = {"+-", 0, 0, 0, 0, 0};

  /* check options */
  while((c = shell_getopt_r(&opt, argc, argv, "+aefhmnouxBCH")) > 0) {
    int on = opt.prefix == '-';

    if(c == 'o') {
      char* name;

      /* shell_getopt_r() only advances past the *whole* current argv
         element for an option that takes an argument (optstring's
         ":") -- "o" has none (it's parsed like any other boolean
         flag, since its own argument is read by hand here, not by
         shell_getopt_r), so opt.ind is still pointing at "-o"/"+o"
         itself at this point, not the word after it. */
      opt.ind++;
      opt.ofs = 0;
      name = argv[opt.ind];

      /* "-o"/"+o" always claims the very next word as its option
         name if there is one at all -- POSIX doesn't leave room for
         it to instead be read as a new positional parameter, even if
         it turns out not to match any known name (that's just an
         error, same as bash's "set: bogus: invalid option name"). */
      if(!name) {
        set_print_all(&opts, !on);
      } else {
        size_t i;
        int letter = 0;

        for(i = 0; i < SET_LONGOPTS_N; i++) {
          if(str_equal(set_longopts[i].name, name)) {
            letter = set_longopts[i].letter;
            break;
          }
        }

        if(!letter) {
          builtin_errmsg(argv, name, "invalid option name");
          return 1;
        }

        set_apply(&opts, letter, on);
        opt.ind++;
      }

      continue;
    }

    if(!set_apply(&opts, c, on)) {
      builtin_invopt(argv);
      return 1;
    }
  }

  /* did shell_getopt_r() just consume an explicit "--"? It advances
     past the "--" element itself before returning -1 for it, so by
     now opt.ind points one past it -- checking the *previous* argv
     element is the only way left to tell. Needed for two separate
     things below: an explicit "--" is what makes "set --" (no
     operands at all) still mean "yes, replace $@, with nothing"
     rather than "no operands were given, leave $@ alone", and it's
     also what stops a literal "-" appearing right after it (e.g.
     "set -- - -- baz") from being mistaken for the bare-"-" form
     below, which only applies when "-" is genuinely the very first
     operand reached. */
  int saw_dashdash = opt.ind > 0 && argv[opt.ind - 1] && str_equal(argv[opt.ind - 1], "--");

  /* POSIX: a bare "-" (unlike "--") ends option processing the same
     way, but is also specific to "set" -- turning off -x (and -v,
     once implemented) -- so it can't be handled once and for all
     inside the shared shell_getopt_r() the way "--" is: other
     builtins built on the same routine (e.g. "cat -") need a lone
     "-" left alone as a literal operand, not silently consumed as an
     end-of-options marker. */
  if(!saw_dashdash && argv[opt.ind] && str_equal(argv[opt.ind], "-")) {
    opts.xtrace = 0;
    opt.ind++;
  }

  sh->opts = opts;

  if(argv[opt.ind] || saw_dashdash) {
    /* &argv[opt.ind] is a valid pointer to a NULL entry when there
       are zero operands (e.g. "set --" alone) -- sh_setargs()
       correctly treats that as "replace $@ with zero arguments",
       distinct from passing it a NULL pointer outright (which means
       "leave $@ as it is", used elsewhere for unshifting). */
    sh_setargs(&argv[opt.ind], 1);
  }
  /* print only for a truly bare "set" (argc == 1, just argv[0]) --
     "set --" is handled above regardless of operand count, and
     "set -e" (a real option, no operands, no "--") must silently
     leave $@ untouched and print nothing either. */
  else if(argc <= 1) {
    union node* n;

    vartab_print(V_DEFAULT);

    for(n = functions; n; n = n->next) {
      tree_print(n, fd_out->w);
      buffer_putnlflush(fd_out->w);
    }
  }

  return 0;
}
