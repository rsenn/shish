DIR=$(dirname "${0}")
. "$DIR/common.sh"

## Testing trap builtin (rewritten to use lib/sig's sig_push()/sig_pop()
## instead of raw signal()/SIG_DFL -- see fixes/76)

## a trap on a real signal actually runs when that signal arrives
OUTFILE=$(mktemp)
(
  trap 'echo caught-term' TERM
  kill -TERM $$
  sleep 0.2
  echo after-term
) > "$OUTFILE"
assert_equal "caught-term
after-term" "$(cat "$OUTFILE")" "a trap on TERM runs when TERM is delivered, and execution continues afterward"
rm -f "$OUTFILE"

## re-trapping the same signal replaces the old handler, doesn't stack
## both (dedup fix -- the old code leaked a "traps" list node and a
## sig_push() stack frame on every retrap of the same signal)
OUTFILE=$(mktemp)
(
  trap 'echo first' USR1
  trap 'echo second' USR1
  kill -USR1 $$
  sleep 0.2
) > "$OUTFILE"
assert_equal "second" "$(cat "$OUTFILE")" "retrapping the same signal replaces the handler -- only the newest one fires, not both"
rm -f "$OUTFILE"

## retrapping the same signal many more times than sig_push()'s
## per-signal stack depth (SIGSTACKSIZE, 16) must not exhaust it --
## confirms trap_install() actually pops the previous registration
## before pushing a new one
OUTFILE=$(mktemp)
(
  i=0
  while [ "$i" -lt 30 ]; do
    trap "echo iter-$i" USR2
    i=$((i + 1))
  done
  kill -USR2 $$
  sleep 0.2
) > "$OUTFILE"
assert_equal "iter-29" "$(cat "$OUTFILE")" "retrapping a signal 30 times (past sig_push()'s 16-deep stack) still works, and only the last trap fires"
rm -f "$OUTFILE"

## "trap - SIG" resets to the default disposition and removes it from
## "trap -p"'s listing
OUTFILE=$(mktemp)
(
  trap 'echo should-not-print' TERM
  trap - TERM
  trap -p TERM
) > "$OUTFILE"
assert_equal "" "$(cat "$OUTFILE")" "trap - SIG resets to default and trap -p SIG then prints nothing for it"
rm -f "$OUTFILE"

## trap -p with no operand lists every currently-installed trap
OUTFILE=$(mktemp)
(
  trap 'echo t1' TERM
  trap 'echo t2' INT
  trap -p
) > "$OUTFILE"
X=$(grep -c "trap 'echo t1' TERM\|trap 'echo t2' INT" "$OUTFILE")
assert_equal "2" "$X" "trap -p with no operand lists every installed trap"
rm -f "$OUTFILE"

## an EXIT trap can be installed and shows up in trap -p's listing.
## checked at top level, not inside a subshell -- "trap -p"'s own
## output is also subject to the misdirection bug above once a
## subshell has an EXIT trap registered, for the same underlying
## reason. Cleaned up immediately via "trap - EXIT" so this doesn't
## leave a dangling top-level EXIT trap for the rest of this file.
X=$(trap 'echo should-show-in-listing' EXIT; trap -p EXIT)
trap - EXIT
assert_equal "trap 'echo should-show-in-listing' EXIT" "$X" "an EXIT trap can be installed and shows up in trap -p's listing"

## fixes/80 (subshell-exit-trap-output-misdirected): a subshell's own
## EXIT trap must run when the subshell itself finishes (whether by
## falling off the end or via an explicit "exit"), using the
## subshell's own fd context, not the top-level shell's -- and it must
## not still be installed (leaked) afterward.
X=$( ( trap 'echo trap-ran' EXIT; : ) )
assert_equal "trap-ran" "$X" "a subshell's own EXIT trap fires -- into the subshell's own capture -- when its body falls off the end"

X=$( ( trap 'echo trap-ran' EXIT; exit 0 ) )
assert_equal "trap-ran" "$X" "a subshell's own EXIT trap fires exactly once (not twice) when the subshell exits via an explicit 'exit'"

OUTFILE80=$(mktemp)
( trap 'echo inner-trap' EXIT )
trap -p EXIT > "$OUTFILE80"
assert_equal "" "$(cat "$OUTFILE80")" "an EXIT trap set inside a subshell must not leak into the parent shell's trap list"
rm -f "$OUTFILE80"

## the parent's own EXIT trap must be unaffected by an unrelated one
## set (and already fired) inside a subshell
OUTFILE80B=$(mktemp)
(
  trap 'echo outer-trap' EXIT
  ( trap 'echo inner-trap' EXIT; : )
  echo after-subshell
) > "$OUTFILE80B"
assert_equal "inner-trap
after-subshell
outer-trap" "$(cat "$OUTFILE80B")" "an outer EXIT trap still fires at the right time, unaffected by a same-named trap set and already run inside a nested subshell"
rm -f "$OUTFILE80B"

summary
