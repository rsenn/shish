DIR=$(dirname "${0}")
. "$DIR/common.sh"

## Tab-completion of the first word of a command: reserved words that
## begin a construct, builtins and defined functions (src/term/term_complete.c).
## Drives an interactive shish through a pty, so it needs python3 and Linux.

SELF=$(readlink "/proc/$$/exe" 2>/dev/null)
PY=$(command -v python3)

if [ -z "$SELF" ] || [ ! -x "$SELF" ] || [ -z "$PY" ]; then
  echo "needs /proc/self/exe and python3, skipping" 1>&2
  summary
fi

TESTDIR=$(mktemp -d)
cd "$TESTDIR" || exit 1
: > zzfile
: > cf_alpha.txt
: > cf_beta
: > .cf_hidden
: > cfx1
: > cfx2
mkdir cf_dir
: > cf_dir/inner_file
ln -s cf_dir cf_link

cat > driver.py <<'PYEOF'
import os, pty, sys, time, select, re, fcntl, termios, struct
shish = sys.argv[1]
chunks = [c.encode().decode('unicode_escape').encode('latin1') for c in sys.argv[2:]]
pid, fd = pty.fork()
if pid == 0:
    os.environ['TERM'] = 'xterm'
    os.execv(shish, [shish, '-i'])
fcntl.ioctl(fd, termios.TIOCSWINSZ, struct.pack('HHHH', 24, 80, 0, 0))
out = b''
def drain(t):
    global out
    end = time.time() + t
    while time.time() < end:
        r, _, _ = select.select([fd], [], [], 0.1)
        if r:
            try: d = os.read(fd, 4096)
            except OSError: return
            if not d: return
            out += d
drain(1.0)
for c in chunks:
    os.write(fd, c); drain(0.4)
os.write(fd, b'exit\n'); drain(0.4)
txt = re.sub(r'\x1b\[[?0-9;]*[A-Za-z]', '', out.decode('utf8', 'replace')).replace('\r', '')
print(txt)
PYEOF

## complete <chunk>...   feed each chunk to the editor, print the screen
complete() {
  "$PY" driver.py "$SELF" "$@" 2>&1
}

OUT=$(complete 'ech' '\t' 'hi\n')
assert_match "$OUT" "*echo hi*" "a unique builtin prefix completes at the first word (ech<TAB> -> echo )"

OUT=$(complete 'cas' '\t' '\x03')
assert_match "$OUT" "*case *" "a construct keyword completes (cas<TAB> -> case )"

OUT=$(complete 'unt' '\t' '\x03')
assert_match "$OUT" "*until *" "unt<TAB> -> until"

OUT=$(complete 'myfunc() { echo called; }\n' 'myfu' '\t' '\n')
assert_match "$OUT" "*myfunc *called*" "a defined function name completes and runs"

OUT=$(complete 'echo a; ech' '\t' 'b\n')
assert_match "$OUT" "*echo a; echo b*" "the word after ';' is a command position"

OUT=$(complete 'if ech' '\t' 'c; then echo t; fi\n')
assert_match "$OUT" "*if echo c; then*" "the word after 'if' is a command position"

OUT=$(complete 'echo ech' '\t' '\x03')
assert_nomatch "$OUT" "*echo echo*" "the second word does not complete to a builtin"

OUT=$(complete '\t' '\t' '\x03')
assert_nomatch "$OUT" "*while*" "a bare TAB does not list keywords and builtins"
assert_match "$OUT" "*zzfile*" "a bare TAB still lists the files"

## file names: a unique file gets a space, a directory (or a link to one) a slash
OUT=$(complete 'echo cf_al' '\t' '\n')
assert_match "$OUT" "*echo cf_alpha.txt *cf_alpha.txt*" "a unique file name completes with a trailing space"

OUT=$(complete 'echo cf_di' '\t' 'x\n')
assert_match "$OUT" "*echo cf_dir/x*" "a unique directory completes with a trailing slash"

OUT=$(complete 'echo cf_li' '\t' 'x\n')
assert_match "$OUT" "*echo cf_link/x*" "a symlink to a directory completes with a slash too"

OUT=$(complete 'echo cf_dir/in' '\t' '\n')
assert_match "$OUT" "*echo cf_dir/inner_file *" "completion looks inside the directory typed so far"

OUT=$(complete 'echo cfx' '\t' '\x03')
assert_match "$OUT" "*cfx1*cfx2*" "several candidates are listed"
assert_nomatch "$OUT" "*cfx1 *echo cfx1*" "and nothing is inserted beyond what they share"

OUT=$(complete 'echo ' '\t' '\x03')
assert_nomatch "$OUT" "*.cf_hidden*" "dotfiles are not offered for a bare TAB"
assert_match "$OUT" "*cf_beta*" "ordinary files are"

OUT=$(complete 'echo .cf_' '\t' '\n')
assert_match "$OUT" "*echo .cf_hidden *" "dotfiles are offered once a leading dot is typed"

OUT=$(complete 'echo .' '\t' '\x03')
assert_match "$OUT" "*.cf_hidden*" "a bare dot lists the dotfiles"
assert_match "$OUT" "* .. *" "and .. (which readdir would give)"

cd / && rm -rf "$TESTDIR"

summary
