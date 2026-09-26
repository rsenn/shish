DIR=$(dirname "${0}")
. "$DIR/common.sh"

## vi editing mode (src/term/term_vimode.c): ESC enters command mode.
## Drives an interactive shish through a pty, so it needs python3 and Linux.

SELF=$(readlink "/proc/$$/exe" 2>/dev/null)
PY=$(command -v python3)

if [ -z "$SELF" ] || [ ! -x "$SELF" ] || [ -z "$PY" ]; then
  echo "needs /proc/self/exe and python3, skipping" 1>&2
  summary
fi

TESTDIR=$(mktemp -d)
cd "$TESTDIR" || exit 1

cat > driver.py <<'PYEOF'
import os, pty, sys, select
pid, fd = pty.fork()
if pid == 0:
    os.environ['HOME'] = os.getcwd()
    os.execv(sys.argv[1], ['shish', '-i'])
def rd(t):
    o = b''
    while select.select([fd], [], [], t)[0]:
        try: d = os.read(fd, 4096)
        except OSError: break
        if not d: break
        o += d
    return o
rd(1)
os.write(fd, sys.argv[2].encode().decode('unicode_escape').encode('latin1') + b'\n')
out = rd(0.8).decode(errors='replace').replace('\r', '').split('\n')
print([l for l in out if l and '$' not in l][-1])
os.write(fd, b'exit\n')
PYEOF

vi() { "$PY" driver.py "$SELF" "$1" 2>/dev/null; }

assert_equal "one three" "$(vi 'echo one two three\x1bbbdw')" "bb dw: delete the word under the cursor"
assert_equal "aa bb XX" "$(vi 'echo aa bb cc\x1bbcwXX\x1b')" "cw changes a word and ESC leaves insert mode"
assert_equal "abc" "$(vi 'echo abc\x1bxp')" "x then p puts the deleted character back"
assert_equal "ok" "$(vi 'echo foo bar\x1bddiecho ok')" "dd clears the line"
assert_equal "hello!" "$(vi 'echo hello\x1bA!')" "A appends at the end"
assert_equal "x c" "$(vi 'echo x a.b c\x1b0wwdW')" "dW deletes a whole blank-delimited word"
assert_equal "x .b c" "$(vi 'echo x a.b c\x1b0wwdw')" "dw stops at punctuation"
assert_equal "x a.b" "$(vi 'echo x a.b c\x1b0WWWdW')" "W moves over a.b in one step"
assert_equal "y c" "$(vi 'echo a.b c\x1bBcWy\x1b')" "B and cW change a big word"

cd /
rm -rf "$TESTDIR"
summary
