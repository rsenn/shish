DIR=$(dirname "${0}")
. "$DIR/common.sh"

## Testing kill builtin
##
## Every "still alive?" probe below is its own separate "if kill -0
## ...; then ... else ... fi" rather than chaining a redirect right
## after the invocation -- a pre-existing, unrelated parser bug (see
## BUGS: kill-arg-redirect-parse) drops a "N>word" redirect that
## immediately follows a "-N"-shaped argument plus a quoted/expanded
## word, so "kill -0 \"\$pid\" 2>/dev/null" doesn't actually redirect
## fd 2 -- it hands "2>/dev/null" to kill as a literal (and bogus)
## operand instead.

sleep 5 &
pid=$!
if kill -0 "$pid"; then X=alive; else X=dead; fi
assert_equal "alive" "$X" "kill -0 on a live pid reports success"

kill -TERM "$pid"
sleep 0.3
if kill -0 "$pid"; then X=alive; else X=dead; fi
assert_equal "dead" "$X" "kill -TERM actually terminates the process, confirmed via a following kill -0"

sleep 5 &
pid2=$!
kill -9 "$pid2"
sleep 0.3
if kill -0 "$pid2"; then X=alive; else X=dead; fi
assert_equal "dead" "$X" "kill -9 (numeric signal) terminates the process"

sleep 5 &
pid3=$!
kill -KILL "$pid3"
sleep 0.3
if kill -0 "$pid3"; then X=alive; else X=dead; fi
assert_equal "dead" "$X" "kill -KILL (bare name) terminates the process"

sleep 5 &
pid4=$!
kill -SIGKILL "$pid4"
sleep 0.3
if kill -0 "$pid4"; then X=alive; else X=dead; fi
assert_equal "dead" "$X" "kill -SIGKILL (SIG-prefixed name, case-insensitive) terminates the process"

sleep 5 &
jobpid=$!
kill %1
sleep 0.3
if kill -0 "$jobpid"; then X=alive; else X=dead; fi
assert_equal "dead" "$X" "kill %job-spec resolves the job and terminates it"

( kill -FOOBAR 1 )
assert_equal "1" "$?" "an unrecognized signal name fails with a non-zero exit status"

( kill )
assert_equal "1" "$?" "kill with no operands fails with a non-zero exit status"

( kill -0 999999 )
assert_equal "1" "$?" "kill -0 on a pid that doesn't exist fails with a non-zero exit status"

( kill -0 $$ )
assert_equal "0" "$?" "kill -0 on the shell's own pid succeeds"

summary
