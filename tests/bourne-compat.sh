DIR=$(dirname "${0}")
. "$DIR/common.sh"

## autoconf's generated `configure` scripts (M4sh) run a "be more
## Bourne compatible" preamble, then gate on an "as_required" snippet
## (both quoted verbatim below from gettext-0.22.5's
## gettext-tools/configure) before deciding whether the shell running
## it is good enough -- if `eval "$as_required"` fails, configure
## assumes the current shell isn't POSIX-compatible enough and goes
## hunting for another one (sh, bash, ksh, sh5) to CONFIG_SHELL-reexec
## into instead. Running gettext-tools/configure under shish used to
## always fail this and silently re-exec bash. Each piece is checked
## separately below so a failure points at the specific construct
## responsible, plus one end-to-end run of the exact concatenated
## script matching what configure itself evaluates.

## the "be more Bourne compatible" preamble -- a no-op outside zsh
AS_BOURNE_COMPATIBLE='if test ${ZSH_VERSION+y} && (emulate sh) >/dev/null 2>&1
then :
  emulate sh
  NULLCMD=:
  # Pre-4.2 versions of Zsh do word splitting on ${1+"$@"}, which
  # is contrary to our usage.  Disable this feature.
  alias -g '"'"'${1+"$@"}'"'"'='"'"'"$@"'"'"'
  setopt NO_GLOB_SUBST
else case e in #(
  e) case `(set -o) 2>/dev/null` in #(
  *posix*) :
    set -o posix ;; #(
  *) :
     ;;
esac ;;
esac
fi
'

## the actual gate: every one of these must succeed for
## `as_have_required` to stay "yes"
AS_REQUIRED='as_fn_return () { (exit $1); }
as_fn_success () { as_fn_return 0; }
as_fn_failure () { as_fn_return 1; }
as_fn_ret_success () { return 0; }
as_fn_ret_failure () { return 1; }

exitcode=0
as_fn_success || { exitcode=1; echo as_fn_success failed.; }
as_fn_failure && { exitcode=1; echo as_fn_failure succeeded.; }
as_fn_ret_success || { exitcode=1; echo as_fn_ret_success failed.; }
as_fn_ret_failure && { exitcode=1; echo as_fn_ret_failure succeeded.; }
if ( set x; as_fn_ret_success y && test x = "$1" )
then :

else case e in #(
  e) exitcode=1; echo positional parameters were not saved. ;;
esac
fi
test x$exitcode = x0 || exit 1
blah=$(echo $(echo blah))
test x"$blah" = xblah || exit 1
test -x / || exit 1'

## exercise each mechanism as_required depends on individually

RESULT=$(as_fn_return () { (exit $1); }
as_fn_success () { as_fn_return 0; }
as_fn_success && echo ok || echo fail)
assert_equal "ok" "$RESULT" "a function calling (exit N) in a subshell, then checked with &&, must report success for N=0"

RESULT=$(as_fn_return () { (exit $1); }
as_fn_failure () { as_fn_return 1; }
as_fn_failure && echo fail || echo ok)
assert_equal "ok" "$RESULT" "a function calling (exit N) in a subshell, then checked with ||, must report failure for N=1"

RESULT=$(as_fn_ret_success () { return 0; }
as_fn_ret_success && echo ok || echo fail)
assert_equal "ok" "$RESULT" "a function using a plain 'return 0' must report success"

RESULT=$(as_fn_ret_failure () { return 1; }
as_fn_ret_failure && echo fail || echo ok)
assert_equal "ok" "$RESULT" "a function using a plain 'return 1' must report failure"

RESULT=$(as_fn_ret_success () { return 0; }
if ( set x; as_fn_ret_success y && test x = "$1" ); then echo ok; else echo fail; fi)
assert_equal "ok" "$RESULT" "'set x' inside a subshell run from an if-condition must make \$1 visible to a following && test in that same subshell"

RESULT=$(blah=$(echo $(echo blah)); echo "$blah")
assert_equal "blah" "$RESULT" "nested \$(...) command substitution must not eat or duplicate any output"

if test -x /; then
  RESULT=ok
else
  RESULT=fail
fi
assert_equal "ok" "$RESULT" "'test -x /' must report the root directory as executable (searchable)"

## the exact concatenated script configure evaluates when probing a
## candidate shell (real configure does `sh -c "$as_bourne_compatible""$as_required"`;
## since this test file is itself already run through the shish under
## test, `eval` in a subshell is the equivalent in-process check)
OUTPUT=$( (eval "$AS_BOURNE_COMPATIBLE$AS_REQUIRED") 2>&1 )
STATUS=$?
assert_equal "0" "$STATUS" "the concatenated as_bourne_compatible+as_required script must exit 0"
assert_equal "" "$OUTPUT" "the concatenated as_bourne_compatible+as_required script must print no failure diagnostics"

summary
