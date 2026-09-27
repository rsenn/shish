#!/bin/sh
## set -o pipefail: a pipeline fails with its rightmost non-zero member
## status, for forked members and for members chained in-process (a builtin
## filter chain, TODO.md Goal 13).

. "$(dirname "$0")/common.sh"

SHISH_SELF=$(readlink "/proc/$$/exe" 2>/dev/null)
run() { "$SHISH_SELF" -c "$1" 2>/dev/null; }

assert_equal "0" "$(run 'false | true; echo $?')" "pipefail is off by default"
assert_equal "1" "$(run 'set -o pipefail; false | true; echo $?')" "a failing first member fails the pipeline"
assert_equal "1" "$(run 'set -o pipefail; true | false | true; echo $?')" "a failing middle member fails the pipeline"
assert_equal "0" "$(run 'set -o pipefail; true | true; echo $?')" "an all-zero pipeline still succeeds"
assert_equal "3" "$(run 'set -o pipefail; sh -c "exit 3" | sh -c "exit 0"; echo $?')" "the member status is reported, not just 1"
assert_equal "5" "$(run 'set -o pipefail; sh -c "exit 3" | sh -c "exit 5"; echo $?')" "the rightmost non-zero status wins"
assert_equal "0" "$(run 'set -o pipefail; false | true; set +o pipefail; false | true; echo $?')" "set +o pipefail turns it off again"
assert_equal "1" "$(run 'set -o pipefail; echo hi | grep zzz | cat >/dev/null; echo $?')" "a chained grep with no match fails the pipeline"
assert_equal "0" "$(run 'set -o pipefail; echo hi | grep hi | cat >/dev/null; echo $?')" "a chain of succeeding filters succeeds"
assert_equal "1" "$(run 'set -o pipefail; cat /nonexistent_file_ | cat >/dev/null; echo $?')" "a chained cat that cannot open its file fails the pipeline"
assert_equal "1" "$(run 'set -o pipefail; cat p_ >/dev/null 2>&1; cat /nonexistent_file_ | cat | sort >/dev/null; echo $?')" "a chain feeding an external command reports its members' failure"
assert_equal "1" "$(run 'set -o pipefail; echo hi | cat | grep zzz | sort >/dev/null; echo $?')" "a pumped chain reports a grep that matched nothing"
assert_equal "0" "$(run 'set -o pipefail; echo hi | cat | grep hi | sort >/dev/null; echo $?')" "a pumped chain of succeeding filters succeeds"
assert_equal "1" "$(run 'set -o pipefail; false | true & wait $!; echo $?')" "a backgrounded pipeline reports its failing member through wait"
assert_equal "4" "$(run 'set -o pipefail; sh -c "sleep 0.2; exit 4" | true & wait $!; echo $?')" "a slow failing member is still reported"
assert_equal "0" "$(run 'false | true & wait $!; echo $?')" "a backgrounded pipeline keeps the last member status without pipefail"
assert_equal "pipefail        on" "$(run 'set -o pipefail; set -o' | grep pipefail)" "set -o lists pipefail"

summary
