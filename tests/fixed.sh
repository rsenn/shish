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

## fixes/32: job_new() sets job->nproc to a pipeline's fixed member
## count up front, but job_fork() also (wrongly) treated it as a
## running "how many registered so far" counter -- incrementing it
## past the array job_new() actually allocated, and always writing to
## procs[0] instead of the slot for the process actually being
## registered. Confirmed with an ASan build: any 2+-stage pipeline
## heap-buffer-overflowed job->procs[]; a plain (non-ASan) build
## tolerates the overwrite/OOB read silently, so this can't be turned
## into an assertion that would actually fail on the old code the way
## the rest of this file's checks can -- these are still worth having
## as basic pipeline/job-tracking smoke tests, but the real
## verification for this one was the ASan run itself.
X=$(echo a | grep a)
assert_equal "a" "$X" "a 2-stage pipeline must still produce the right output"
X=$(echo a | tr a-z A-Z | rev)
assert_equal "A" "$X" "a 3-stage pipeline must still produce the right output"

I=0
while [ "$I" -lt 5 ]; do
  echo "x$I" | cat >/dev/null
  I=$(( $I + 1 ))
done
echo done >/dev/null
assert_equal "5" "$I" "several 2-stage pipelines in a row must not corrupt job tracking badly enough to break the loop"

## fixes/33: when the current ("%+") job finished and got cleaned up,
## job_free()'s job_delete() just set the "current job" pointer to
## NULL instead of promoting the previous job to current the way bash
## promotes "%-" to "%+" -- so the "+" marker just disappeared from
## "jobs" output instead of moving to the remaining job.
JOBSFILE=$(mktemp)
sleep 1 &
sleep 0.2 &
sleep 0.4
jobs >/dev/null
jobs >"$JOBSFILE"
grep -q "^\[1\]+" "$JOBSFILE"
assert_equal "0" "$?" "the remaining job must be promoted to current (\"+\") once the previous current job is cleaned up"
rm -f "$JOBSFILE"

## fixes/34: builtin_wait() ignored the exit status it waited for and
## always returned 0; "$!" always expanded empty. Both had a deeper
## root cause: sh_forked() never actually updated sh_pid to the
## forked child's own real pid (a commented-out line, doing nothing),
## so a backgrounded child's own setpgid() call used the wrong pid
## for both "who" and "which group", meaning it never actually ended
## up in the process group its parent expected -- job_wait()'s
## wait_pid(-j->pgrp, ...) then reliably failed to find it. Also fixed
## along the way: job_find()'s bare-pid lookup (used by e.g. "wait
## $!") broke out of its inner scan loop on a match but never told the
## outer one, so it kept walking past every real match.
X=$(sh -c 'exit 42' 2>/dev/null &
wait
echo $?)
assert_equal "42" "$X" "wait with no operands must report the exit status of the backgrounded job it waited for"

true &
BGPID=$!
assert_greater "$BGPID" "0" "\"\$!\" must expand to the backgrounded command's pid, not stay empty"

## fixes/35: job_wait()'s "[N]+ Done ..." job-completion banner (only
## meant for the interactive/job-control case, "set -m") printed for
## *any* job it waited on, not just ones that were actually
## backgrounded -- including the fully-internal job eval_pipeline()
## creates just to fork the last member of a pipeline sitting inside a
## command substitution ("$(cmd | cmd)"), which has no ->command
## string at all, hence "[1]+  Done   (null)" leaking onto a normal,
## non-backgrounded command substitution's stderr. Added job->bgnd,
## set only where a job is genuinely backgrounded, and gated the
## banner on it (both here and in the async job_clean() path used
## between prompts) -- and while there, made both banner sites erase
## the current line and move to column 1 first, matching what
## sh_onsig() already does before anything else it prints, so a
## legitimate banner doesn't land mid-prompt.
STDERRFILE=$(mktemp)
(
  set -m
  X=$(echo hi | sed 1q)
  echo "[$X]"
) 2>"$STDERRFILE"
grep -q "null" "$STDERRFILE"
assert_equal "1" "$?" "a command substitution's internal pipeline must not print a job-completion banner"
rm -f "$STDERRFILE"

## fixes/36: sh_onsig()'s SIGCHLD handler erased and redrew the
## current prompt line on *every* SIGCHLD it caught, unconditional on
## whether the shell was actually sitting idle in term_read() waiting
## on interactive keystrokes. A foreground command substitution's
## pipeline ("$(cmd | cmd)") forks its own children, each exiting
## while the shell is mid-eval, not at the prompt -- so each of their
## SIGCHLDs triggered a spurious erase+redraw, splattering
## "\033[2K\033[0G<prompt>" onto the terminal before the substitution's
## own output. Fixed by adding term_reading (src/term.h,
## src/term/term_read.c), set only while term_read()'s read loop is
## actually blocked waiting for more input, and gating both of
## sh_onsig()'s erase/redraw blocks on it.
##
## This only manifests with a real interactive tty (term_output is
## unset otherwise, so sh_onsig() never reaches the affected code at
## all in a script/ctest context) -- not reproducible here. Verified
## instead with a pty-simulated session (python's `pty` module): before
## the fix, `x=$(echo hi | sed 1q); echo [$x]` produced two
## "\033[2K\033[0G<prompt>\033[0D" sequences before "[hi]"; after the
## fix, none. A genuinely backgrounded job ("sleep 0.3 &" under
## "set -m") still gets its erase+redraw and "[N]+ Done ..." banner
## once the shell is back at the prompt, so real job-control notifications
## are unaffected.

summary
