DIR=$(dirname "${0}")
. "$DIR/common.sh"

## Regression tests for entries marked "FIXED:" in BUGS
## (fixes/19-subshell-pipeline-fork-exit.patch,
##  fixes/20-path-directory-shadowing.patch,
##  fixes/21-exec-failure-status-messages.patch)
##
## Exit status is checked via a plain command followed by "$?" rather
## than via "$(cmd)" -- $? after a command-substitution assignment
## does not currently reflect the substituted command's real exit
## status in shish (a separate, pre-existing issue outside the scope
## of these three fixes).

## fixes/19: a pipeline inside a subshell used to leak its builtin
## members as extra copies of the interpreter -- "(true | true)"
## made everything after the subshell run 3 times instead of once.
COUNTFILE=$(mktemp)
(true | true)
echo once >>"$COUNTFILE"
COUNT=$(wc -l <"$COUNTFILE")
assert_equal "1" "$COUNT" "subshell-pipeline must not duplicate following commands"
rm -f "$COUNTFILE"

## fixes/20: PATH search (and exec_hash()'s direct-path branch)
## matched a directory of the same name as a command, shadowing the
## real executable later on PATH; POSIX requires an executable
## regular file.
TESTDIR=$(mktemp -d)
mkdir "$TESTDIR/shadow"
mkdir "$TESTDIR/real"
cat >"$TESTDIR/real/mycmd" <<'EOF'
#!/bin/sh
echo real-command-ran
EOF
chmod +x "$TESTDIR/real/mycmd"

OUTFILE=$(mktemp)
OLDPATH="$PATH"
PATH="$TESTDIR/shadow:$TESTDIR/real:$OLDPATH"
mycmd >"$OUTFILE"
PATH="$OLDPATH"
assert_equal "real-command-ran" "$(cat "$OUTFILE")" "a directory on PATH must not shadow a real executable further down PATH"
rm -f "$OUTFILE"

## direct-path form ("dir/name" where "dir/name" is itself a
## directory) must also be rejected rather than treated as
## executable.
(cd "$TESTDIR" && ./shadow >/dev/null 2>/dev/null)
assert_equal "126" "$?" "invoking a directory via a direct path must fail, not succeed"

rm -rf "$TESTDIR"

## fixes/21: exec failure statuses. Parent-side lookup failures used
## to always exit 1 instead of the POSIX-mandated 127 (not found) /
## 126 (found but not executable), and a post-fork execve() failure
## on something that passed every pre-check (e.g. ENOEXEC) used to
## SEGV the shell instead of reporting an error.

## command not found on PATH -> 127
OLDPATH="$PATH"
PATH="/nonexistent-dir-for-test"
this-command-does-not-exist >/dev/null 2>/dev/null
STATUS=$?
PATH="$OLDPATH"
assert_equal "127" "$STATUS" "command not found on PATH must exit 127"

## found but not executable (a plain, non-executable regular file) -> 126
TESTDIR=$(mktemp -d)
echo "not a script" >"$TESTDIR/notexec"
chmod -x "$TESTDIR/notexec"
OLDPATH="$PATH"
PATH="$TESTDIR"
notexec >/dev/null 2>/dev/null
STATUS=$?
PATH="$OLDPATH"
assert_equal "126" "$STATUS" "command found but not executable must exit 126"
rm -rf "$TESTDIR"

## execve() failing post-fork on something that passed every
## pre-check (not a valid executable, ENOEXEC) must not crash the
## shell and must report a status rather than silently "succeeding".
TESTDIR=$(mktemp -d)
printf '\000\001\002garbage-not-a-valid-executable' >"$TESTDIR/badexec"
chmod +x "$TESTDIR/badexec"
"$TESTDIR/badexec" >/dev/null 2>/dev/null
STATUS=$?
assert_equal "126" "$STATUS" "an unexecutable file (ENOEXEC) must exit 126, not crash or exit 0"
rm -rf "$TESTDIR"

## fixes/22: the "command not found" error message is printed before
## eval_simple_command.c ever reaches exec_command() -- the only place
## that normally resolves a command's still-pending (open()/dup2()
## deferred) redirections -- so without the fix the message escaped to
## the shell's original stderr and the redirection target was never
## even created.
## ("IFS= read -r" rather than "$(cat file)" -- quoted command
## substitution currently doesn't suppress field splitting in shish,
## a separate, pre-existing bug logged in BUGS)
MSGFILE=$(mktemp)
OLDPATH="$PATH"
PATH="/nonexistent-dir-for-test"
this-command-does-not-exist >/dev/null 2>"$MSGFILE"
PATH="$OLDPATH"
IFS= read -r MSGLINE <"$MSGFILE"
assert_match "$MSGLINE" "*No such file or directory*" "command-not-found message must go through its own redirection, not the shell's original stderr"
rm -f "$MSGFILE"

## fixes/23: "read -d X" left the terminating delimiter X in the
## captured value instead of stripping it (unlike the default newline
## delimiter, which was always stripped) -- builtin_read.c's trim
## step was hardcoded to "\r\n" regardless of what delimiter was
## actually in effect.
TMPFILE=$(mktemp)
printf 'first;second;third' >"$TMPFILE"
read -r -d ';' FIRST <"$TMPFILE"
assert_equal "first" "$FIRST" "read -d must not leave the delimiter in the captured value"
rm -f "$TMPFILE"

## fixes/24: arithmetic expansion containing a single-character
## variable name failed to parse ("echo $((i+1))"): parse_arith_value
## peeked one character past the variable name to decide whether to
## treat it as a substitution, which for a single-character name landed
## on whatever followed the expression (an operator/space/paren) rather
## than another character of the name itself, and got rejected.
i=0
i=$((i + 1))
assert_equal "1" "$i" "arithmetic expansion must accept a single-character bare variable name"

## fixes/25: a backgrounded command sharing its input line with more
## commands ("cmd & more...") used to either crash the shell (a
## backgrounded simple command, e.g. a builtin, wasn't forked at all)
## or fail to parse (a backgrounded compound command's "&" was
## consumed wrong by parse_command.c, corrupting lookahead for
## whatever followed) or, once forking, run its later sibling(s) in
## eval_cmdlist()'s own list loop without going through the same
## fork-and-return-immediately path eval_tree() already had, still
## duplicating it.
OUTFILE=$(mktemp)
true & echo after >"$OUTFILE"
COUNT=$(wc -l <"$OUTFILE")
assert_equal "1" "$COUNT" "backgrounding a simple builtin followed by more on the same line must not crash or duplicate"
rm -f "$OUTFILE"

OUTFILE=$(mktemp)
{ true; } & echo after >"$OUTFILE"
COUNT=$(wc -l <"$OUTFILE")
assert_equal "1" "$COUNT" "backgrounding a compound command followed by more on the same line must not fail to parse or duplicate"
rm -f "$OUTFILE"

## fixes/29: marking a variable for export before it was ever assigned
## a value crashed on a later "export"/"export -p": var_init() never
## stored the variable's name anywhere (var->sa.s stayed NULL), which
## var_print() dereferenced unconditionally to print it. The same
## NULL also silently truncated every OTHER exported variable after
## it in a child process's environment (var_export() put the NULL
## straight into the middle of the envp array, which execve() reads
## as the end of the whole array).
OUTFILE=$(mktemp)
export UNSETEXPORTVAR
export -p >"$OUTFILE" 2>/dev/null
STATUS=$?
assert_equal "0" "$STATUS" "export -p after exporting an unset variable must not crash"
grep -q "export UNSETEXPORTVAR$" "$OUTFILE"
assert_equal "0" "$?" "export -p must list a declared-but-unassigned variable with no ='value'"
rm -f "$OUTFILE"

OUTFILE=$(mktemp)
export UNSETEXPORTVAR2
env >"$OUTFILE"
grep -q "^UNSETEXPORTVAR2" "$OUTFILE"
assert_equal "1" "$?" "an exported-but-unassigned variable must not itself appear in a child's environment"
rm -f "$OUTFILE"

COUNTFILE=$(mktemp)
export UNSETEXPORTVAR3
export FIXEDEXPORTVAR=x
env >"$COUNTFILE"
grep -q "^FIXEDEXPORTVAR=x$" "$COUNTFILE"
assert_equal "0" "$?" "an exported-but-unassigned variable must not truncate the rest of a child's environment"
rm -f "$COUNTFILE"

## fixes/30: assigning to a readonly variable in a simple command that
## also carries a redirection crashed. eval_simple_command.c bails out
## (the readonly check failing) before ever reaching the loop that
## sets up each redirection's ->nredir.fd, but its cleanup path
## unconditionally fd_pop()s every parsed redirection regardless of
## whether it actually got that far, and fd_close() dereferenced the
## still-NULL fd.
readonly READONLYVAR=original 2>/dev/null
READONLYVAR=changed 2>/dev/null
STATUS=$?
assert_equal "1" "$STATUS" "assigning to a readonly variable via a redirected command must report failure, not crash"
assert_equal "original" "$READONLYVAR" "a rejected readonly assignment must not change the variable's value"

## fixes/31: command substitution of a pipeline ("$(cmd | cmd)")
## always expanded to empty. Root cause: fd_subst() only sets up an
## in-process memory sink, but a pipeline always forks real children
## (even for builtins), and nothing wired a real pipe from the last
## forked member back into that sink. eval_pipeline.c now reuses the
## same fdstack_npipes()/fdstack_pipe()/fdstack_data() machinery
## exec_program.c already relied on for the (pipeline-free) command
## substitution case, restricted to the pipeline's last member and to
## the closest fdstack level (so a substitution nested inside another
## one, or a here-doc feeding some other member, isn't hijacked).
X=$(echo hi | sed 1q)
assert_equal "hi" "$X" "command substitution of a 2-stage pipeline must capture its output"

X=$(printf "a\nb\nc\n" | grep b)
assert_equal "b" "$X" "command substitution of a pipeline must still work when input is multi-line"

X=$(echo one | tr a-z A-Z | rev)
assert_equal "ENO" "$X" "command substitution of a 3-stage pipeline must capture the last stage's output"

X=$(echo $(echo a | cat) b)
assert_equal "a b" "$X" "a command substitution with its own pipeline, nested inside another command substitution, must not steal the outer one's output"

X=$(echo hi)
assert_equal "hi" "$X" "command substitution of a plain (non-piped) simple command must still work"

summary
