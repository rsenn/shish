DIR=$(dirname "${0}")
. "$DIR/common.sh"

## source_peekn() (src/source/source_peekn.c) always prefetched one byte past
## whatever position it was asked to peek, to tell a real "\<newline>"
## continuation apart from a lone backslash -- even when the peeked character
## wasn't a backslash at all. Peeking an interactively-typed line's own
## trailing newline (nothing typed after it yet) therefore always blocked for
## one more byte of input it didn't need, showing PS2 and needing several
## Ctrl-D presses to get back out. Drives an interactive shish through a pty,
## so it needs python3 and Linux (see tests/term-complete.sh).

SELF=$(readlink "/proc/$$/exe" 2>/dev/null)
PY=$(command -v python3)

if [ -z "$SELF" ] || [ ! -x "$SELF" ] || [ -z "$PY" ]; then
  echo "needs /proc/self/exe and python3, skipping" 1>&2
  summary
fi

TESTDIR=$(mktemp -d)
cd "$TESTDIR" || exit 1

cat > driver.py <<'PYEOF'
import os, pty, sys, time, select, re
shish = sys.argv[1]
pid, fd = pty.fork()
if pid == 0:
    os.environ['TERM'] = 'xterm'
    os.execv(shish, [shish, '-i'])

def drain(t):
    out = b''
    end = time.time() + t
    while time.time() < end:
        r, _, _ = select.select([fd], [], [], 0.1)
        if not r:
            continue
        try:
            d = os.read(fd, 4096)
        except OSError:
            break
        if not d:
            break
        out += d
    return out

drain(0.5)
os.write(fd, b'echo hi\n')
after_cmd = drain(0.5)
os.write(fd, b'\x04')
after_eof = drain(0.5)
try:
    wpid, status = os.waitpid(pid, os.WNOHANG)
except ChildProcessError:
    wpid = pid

def clean(b):
    return re.sub(r'\x1b\[[?0-9;]*[A-Za-z]', '', b.decode('utf8', 'replace')).replace('\r', '')

print("AFTER_CMD:" + clean(after_cmd))
print("AFTER_EOF:" + clean(after_eof))
print("EXITED:" + ("yes" if wpid == pid else "no"))
PYEOF

OUT=$("$PY" driver.py "$SELF" 2>&1)

## "hi" on a line of its own is the command's actual output, not the
## "echo hi" typed/echoed back by the line editor (which also contains "hi").
assert_match "$OUT" "*
hi
*" "echo hi runs and prints its output right away, on its own line"
assert_nomatch "$OUT" "*AFTER_CMD:*
> *" "no PS2 continuation prompt appears before a plain command's own output"
assert_match "$OUT" "*EXITED:yes*" "a single Ctrl-D at the next prompt exits the shell"

cd / && rm -rf "$TESTDIR"

summary
