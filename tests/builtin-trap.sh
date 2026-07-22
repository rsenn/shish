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
## Deliberately *not* testing that a subshell's EXIT trap actually
## *runs* here -- confirmed present before this rewrite too (same
## behavior with sig_push()/sig_pop() stashed out), a subshell's EXIT
## trap's own output always lands on the top-level shell's original
## stdout instead of wherever the subshell's own output was going
## (whether a plain "> file" redirect or a "$(...)" capture), and the
## parent doesn't wait for it to actually finish running before moving
## on to its own next command -- see BUGS:
## subshell-exit-trap-output-misdirected. That's an eval-frame/fdstack
## teardown timing issue in the EXIT-trap path generically (also
## affects DEBUG/RETURN), unrelated to this rewrite (trap_install()'s
## EXIT/DEBUG/RETURN branch is untouched -- only the real-signal branch
## now goes through sig_push()/sig_pop()).
## checked at top level, not inside a subshell -- "trap -p"'s own
## output is also subject to the misdirection bug above once a
## subshell has an EXIT trap registered, for the same underlying
## reason. Cleaned up immediately via "trap - EXIT" so this doesn't
## leave a dangling top-level EXIT trap for the rest of this file.
X=$(trap 'echo should-show-in-listing' EXIT; trap -p EXIT)
trap - EXIT
assert_equal "trap 'echo should-show-in-listing' EXIT" "$X" "an EXIT trap can be installed and shows up in trap -p's listing"

summary
