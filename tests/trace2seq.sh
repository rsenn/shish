DIR=$(dirname "${0}")
. "$DIR/common.sh"

## tools/trace2seq: turns a SHISH_TRACE log into a process tree and event
## counts. Needs a DEBUG_OUTPUT build of shish, so it skips on any other.

SELF=$(readlink "/proc/$$/exe" 2>/dev/null)
TOOL="$DIR/../tools/trace2seq"
LOG=$(mktemp)

SHISH_TRACE=exec,sh SHISH_TRACE_FILE="$LOG" "$SELF" -c 'echo hi | /bin/cat; /bin/echo a >/dev/null' 2>/dev/null

if [ -z "$SELF" ] || [ ! -s "$LOG" ]; then
  echo "shish has no trace support (DEBUG_OUTPUT), skipping" 1>&2
  rm -f "$LOG"
  summary
fi

X=$("$TOOL" -c exec.program.execve "$LOG")
assert_equal 2 "$X" "two programs are exec'd (cat and /bin/echo)"

X=$("$TOOL" "$LOG" | grep -c 'exec /bin/cat')
assert_equal 1 "$X" "the tree shows cat as an exec'd child"

X=$("$TOOL" "$LOG" | grep -c '^pid ')
assert_equal 1 "$X" "exactly one root process"

X=$("$TOOL" "$LOG" | grep -c '^  pid .*exit=0')
assert_equal 3 "$X" "the builtin stage, cat and /bin/echo are children that exit 0"

X=$("$TOOL" -s "$LOG" | grep -c -- '->>')
assert_equal 3 "$X" "-s draws one fork arrow per child process"

X=$("$TOOL" -s "$LOG" | head -1)
assert_equal sequenceDiagram "$X" "-s starts a Mermaid sequenceDiagram"

X=$("$TOOL" -c no.such.event "$LOG")
assert_equal 0 "$X" "an event that never happened counts 0"

rm -f "$LOG"
summary
