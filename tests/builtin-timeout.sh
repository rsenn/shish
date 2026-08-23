DIR=$(dirname "${0}")
. "$DIR/common.sh"

## Testing timeout builtin

## a command finishing within DURATION passes its own exit status through
timeout 2 true
assert_equal "0" "$?" "a command that finishes in time reports its own success"

timeout 2 false
assert_equal "1" "$?" "a command that finishes in time reports its own failure"

## a duration of 0 disables the timeout entirely
timeout 0 sh -c 'exit 5'
assert_equal "5" "$?" "a duration of 0 disables the timeout"

## a timed-out command is reported as 124
timeout 0.2 sleep 5
assert_equal "124" "$?" "a command still running past DURATION is reported as 124"

## -s selects the signal sent on timeout; a KILLed command reports 137
timeout -s KILL 0.2 sleep 5
assert_equal "137" "$?" "-s KILL reports 137 when the command is killed"

## -k escalates to KILL if the command outlives the initial signal
timeout -k 0.3 0.1 sh -c 'trap "" TERM; sleep 5'
assert_equal "137" "$?" "-k escalates to KILL when the command ignores the first signal"

## -v reports the signal sent on timeout, to stderr
timeout -v 0.2 sleep 5 2>errlog >/dev/null
assert_match "$(cat errlog)" "*TERM*" "-v reports the signal name it sent"
rm -f errlog

## a nonexistent command is reported as not found
timeout 1 /no/such/command >/dev/null 2>&1
assert_equal "127" "$?" "a command that can't be found on PATH is reported as 127"

## a missing operand is a usage error
timeout >/dev/null 2>&1
assert_equal "125" "$?" "timeout with no operands is a usage error"

## an invalid duration is a usage error
timeout bogus true >/dev/null 2>&1
assert_equal "125" "$?" "an invalid duration is a usage error"

summary
