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
## (run in a subshell: since fixes/196 a variable assignment error ends
## a non-interactive shell, POSIX 2.8.1, so doing this inline would now
## take the rest of this file with it.)
readonly READONLYVAR=original 2>/dev/null
(READONLYVAR=changed 2>/dev/null)
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

## fixes/37: builtin echo used shell_getopt() (shared, strict getopt
## infrastructure meant for real builtins with real options) to parse
## its "options", so any unrecognized word starting with "-" -- which
## for echo isn't an option at all, just an operand that happens to
## start with a dash -- hit shell_getopt()'s default case and errored
## out via builtin_invopt() instead of being printed. Replaced with
## manual parsing matching POSIX/dash/bash: an argument starting with
## "-" only ends option parsing (and is consumed) if every remaining
## character is one of n/e/E; the first word that doesn't qualify ends
## option parsing right there and is itself the first operand.
X=$(echo ---marker---)
assert_equal "---marker---" "$X" "echo must print a dash-led word it doesn't recognize as an option, not error out"

X=$(echo -n hi)
assert_equal "hi" "$X" "echo -n must still be recognized as an option"

## fixes/38: configure.ac's per-builtin AC_DEFINE_UNQUOTED loop was
## generated with m4_foreach(), but a stray "dnl" was placed right
## before the closing "])" meant to end the loop body -- "dnl" eats
## the rest of its physical line (including that "])"), so the
## m4_foreach() call was left unterminated and silently swallowed
## everything up to the next "])" it could find later in the file,
## producing a generated configure script with the raw builtin-name
## list dumped as literal shell text instead of the intended
## per-builtin case/AC_DEFINE_UNQUOTED blocks. A later, botched
## attempt to work around this (m4_echo()/AC_FOREACH(), the latter
## not a real Autoconf macro) made it worse rather than fixing it.
## Rewrote the block as a single well-formed m4_foreach() over
## ALL_BUILTINS, with the per-builtin uppercased macro name built via
## the standard "[BUILTIN_]m4_toupper(BUILTIN)" concatenation idiom
## (bracket-quoting the loop variable, as the previous version did,
## prevents it from being substituted before m4_toupper() runs on it).
##
## This is a configure.ac / autotools-generation bug, not a shish
## runtime behavior -- ctest never invokes autoconf/autoheader, so
## there's no way to exercise it from tests/*.sh. Verified instead by
## running ./autogen.sh && ./configure --enable-builtins=echo &&
## ./config.status config.h and confirming config.h now has
## "#define BUILTIN_ECHO 1" (plus the rest of the default builtin
## set) instead of the generated configure script containing the raw,
## unexpanded builtin-name list as literal text.

## fixes/39: "set" with no arguments and no options is supposed to
## dump every variable (and, per POSIX/bash, every defined function)
## for re-input, but its "were any options given?" check tested
## whether the *resulting* shopt struct was all-zero rather than
## whether shell_getopt_r() actually matched anything -- so on any
## shell where a shopt flag was ever turned on (interactively, "set
## -m" is common), the struct was never all-zero again and the dump
## silently stopped firing for the rest of the session. Also never
## dumped functions at all. Fixed with an explicit got_opt flag, and
## added a functions dump (reusing tree_print(), same as "dump -F")
## after the variable dump.
OUTFILE=$(mktemp)
(
  set -m
  FOO=setnoargsvalue
  myfn() { echo hi; }
  set
) >"$OUTFILE"
grep -q '^FOO="setnoargsvalue"$' "$OUTFILE"
assert_equal "0" "$?" "\"set\" with no args must dump variables even after a shopt flag was turned on"
grep -q '^myfn() {$' "$OUTFILE"
assert_equal "0" "$?" "\"set\" with no args must also dump defined functions"
rm -f "$OUTFILE"

## fixes/40: builtin_chmod only ever accepted a plain octal mode
## ("chmod 755 file") -- any symbolic mode ("chmod a+x file", "chmod
## u+rwx,go-w file") fell through scan_8int() untested (it stops at
## the first non-octal-digit character and silently returns whatever
## it managed to parse before that, so e.g. "u+x" parsed as "0" and
## every file got chmod(path, 0)). Added chmod_symbolic(), a
## [ugoa]*[+-=][rwx]* parser (repeatable, comma-separated) applied on
## top of each file's own current mode via stat(); a mode string only
## takes the numeric path now if it's made up entirely of octal
## digits.
##
## Not exercisable through this default ctest build: chmod is an
## EXTRA_BUILTINS entry, off by default (BUILTIN_CHMOD=0), so "chmod"
## here resolves to the system PATH binary, which already understands
## symbolic mode natively regardless of this fix -- a test running
## against this binary would pass whether or not the fix is present,
## so it wouldn't actually be testing anything. Verified instead by
## building a separate tree with -DBUILTIN_CHMOD=ON and confirming:
## "chmod a+x file" / "chmod go-r file" / "chmod a=rwx file" / "chmod
## u+rwx,go-w file" all produce the expected mode bits (checked via
## "stat -c %a"), "chmod 644 file" (plain octal) still works
## unchanged, and "chmod +q file" (an invalid perm letter) reports
## "chmod: +q: invalid mode" and exits 1 rather than silently
## chmod()ing to 0.

## fixes/41-44: fg/bg were fully implemented (jobs/fg/bg in
## src/builtin/builtin_jobs.c). Along the way:
## - builtin_fg had a stack out-of-bounds write for the common bare
##   "fg" (no operand) case: a "struct job *joblist[argc - 1]" VLA is
##   zero-length there, and "joblist[0] = job_current();" wrote past
##   it regardless. Rewritten to resolve a single job directly (fg
##   only ever moves one job to the foreground) instead of building an
##   array at all.
## - builtin_table.c's dispatch table had "bg" wired to &builtin_fg
##   instead of &builtin_bg -- bg silently ran fg's code instead.
## - job_wait() never asked wait()/waitpid() for WUNTRACED, so a
##   stopped process was indistinguishable from "still running" to it.
##   Added wait_pid_untraced()/wait_nohang_untraced() and wired them
##   into job_wait()'s synchronous wait and sh_onsig()'s async SIGCHLD
##   path (gated on sh->opts.monitor -- script behavior is unchanged).
## - job_done(j) didn't exclude a stopped job (job_running() only
##   checks status == -1), so a job that just stopped looked "done"
##   and got job_free()'d instead of staying around for fg/bg.
## - exec_program.c's separate X_NOWAIT fork path (used for any
##   backgrounded *external* command, e.g. "cmd &") never called
##   setpgid() at all, so job->pgrp recorded the child's pid as if it
##   were a real process group without that ever being true at the OS
##   level -- killpg(job->pgrp, SIGCONT) (fg/bg) and job_wait()'s own
##   wait_pid(-job->pgrp, ...) both silently failed (ESRCH) against a
##   process group that didn't exist. Fixed by adding the same
##   parent+child setpgid() dance job_fork() already does elsewhere.
##
## "kill -STOP"/"kill -CONT" are used here instead of a real Ctrl-Z
## (SIGTSTP from a terminal driver) since these tests don't run under
## a pty -- the resulting process state is identical either way, and
## exercises the exact same job_wait()/job_signal() bookkeeping.
JOBSFILE2=$(mktemp)
(
  set -m
  sleep 0.5 &
  PID=$!
  kill -STOP "$PID"
  sleep 0.2
  jobs >"$JOBSFILE2.stopped"
  bg >/dev/null
  wait "$PID"
  echo "waitstatus=$?" >"$JOBSFILE2.wait"
  jobs >"$JOBSFILE2.after"
)
grep -q "Stopped" "$JOBSFILE2.stopped"
assert_equal "0" "$?" "kill -STOP on a backgrounded job must be detected and reported as Stopped"

IFS= read -r WAITLINE <"$JOBSFILE2.wait"
assert_equal "waitstatus=0" "$WAITLINE" "\"bg\" (not \"fg\" via the mis-wired dispatch table) must actually resume the stopped job so \"wait\" sees it exit cleanly"

[ -s "$JOBSFILE2.after" ]
assert_equal "1" "$?" "a resumed-then-waited-for job must be fully reaped, not left listed by \"jobs\""

rm -f "$JOBSFILE2" "$JOBSFILE2.stopped" "$JOBSFILE2.wait" "$JOBSFILE2.after"

## "fg" (no operand, the common case that used to crash) on a stopped
## job must resume it and block until it actually finishes.
FGFILE=$(mktemp)
(
  set -m
  sleep 0.3 &
  PID=$!
  kill -STOP "$PID"
  sleep 0.1
  fg >/dev/null
  echo "fgstatus=$?" >"$FGFILE.status"
  jobs >"$FGFILE.after"
)
IFS= read -r FGLINE <"$FGFILE.status"
assert_equal "fgstatus=0" "$FGLINE" "\"fg\" on a stopped job must resume it (no stack OOB crash on the bare no-operand form) and wait for it to finish"

[ -s "$FGFILE.after" ]
assert_equal "1" "$?" "a job brought to the foreground and waited out must be fully reaped, not left listed by \"jobs\""

rm -f "$FGFILE" "$FGFILE.status" "$FGFILE.after"

## "bg" on a job that's already running (never stopped) must refuse,
## not silently do nothing (or, pre-fix, silently run fg's code
## instead via the mis-wired dispatch table).
BGERRFILE=$(mktemp)
(
  set -m
  sleep 0.2 &
  bg
  echo "bgstatus=$?"
) >/dev/null 2>"$BGERRFILE"
grep -q "already in background" "$BGERRFILE"
assert_equal "0" "$?" "\"bg\" on a job that was never stopped must report an error, not silently succeed"
rm -f "$BGERRFILE"

## fixes/45: every "[id] ..." job banner (a job starting in the
## background, job_wait()'s/sh_onsig()'s Done/Stopped notices, bg's
## resume line, and "jobs"'s own listing) used to hand-roll its own
## buffer_put*() calls -- job_wait()'s banners padded the status word
## to a different column than job_print()'s (26 vs. 24, though they
## came out the same total width by coincidence), and two of the three
## Done/Stopped sites computed the "is this the current job" marker
## via "(struct job*)job_pointer == j", comparing a struct job** cast
## to struct job* against a struct job* -- meaningless, always false
## in practice, so the "%-" ('-') marker never actually appeared.
## Consolidated into one job_banner(job, out, kind) (src/job/
## job_banner.c) that every site now calls; job_print() (used by the
## "jobs" builtin) is now just job_banner() with the status
## auto-selected from the job's current state.
JOBSFILE3=$(mktemp)
sleep 0.2 &
jobs >"$JOBSFILE3"
grep -q "^\[1\][+ ]  Running" "$JOBSFILE3"
assert_equal "0" "$?" "job_print() must still format a running job's status line correctly after routing through job_banner()"
wait
rm -f "$JOBSFILE3"

## fixes/46: "$?" right after a foreground pipeline on the same line
## kept reporting whatever it was *before* the pipeline ran, instead
## of the pipeline's own (already correctly computed -- eval_pipeline()
## itself returned the right value) exit status. Root cause:
## eval_simple_command() updates sh->exitcode directly, but
## eval_pipeline() only returned its status, relying on eval_tree()'s
## loop to stash it in the per-frame e->exitcode -- which isn't synced
## back to sh->exitcode (what "$?" actually reads) until the *whole*
## line's eval_tree() call returns, too late for a later command on
## the same line ("cmd1 | cmd2; echo $?"). Fixed by having
## eval_pipeline() set sh->exitcode directly too, the same as
## eval_simple_command() already does.
X=$(true | false; echo $?)
assert_equal "1" "$X" "\"\$?\" right after a 2-stage pipeline (builtin | builtin) on the same line must be the last stage's status"

X=$(false | true; echo $?)
assert_equal "0" "$X" "\"\$?\" right after a pipeline must be the LAST stage's status, not the first"

X=$(true | false | false; echo $?)
assert_equal "1" "$X" "\"\$?\" after a 3-stage pipeline must still be the last stage's status"

X=$(false | false | true; echo $?)
assert_equal "0" "$X" "\"\$?\" after a 3-stage pipeline ending in success must be 0"

X=$(true | false & wait; echo $?)
assert_equal "0" "$X" "backgrounding a pipeline (\"cmd1 | cmd2 &\") must itself report success as \"\$?\", independent of what it later exits with"

## fixes/47: exec_hash()'s command-search cache remembered a name's
## resolved path forever, even after PATH was reassigned to something
## that would find (or no longer find) a different file for the same
## name -- POSIX 3.9.1.1 requires a PATH change to invalidate this.
## Fixed by having exec_hash() compare PATH's current value against
## the last one it saw on every call, invalidating every cached entry
## (forcing a fresh exec_search()) the first time it notices PATH has
## changed since.
TESTDIR=$(mktemp -d)
mkdir "$TESTDIR/a" "$TESTDIR/b"
cat >"$TESTDIR/a/pathcachecmd" <<'EOF'
#!/bin/sh
echo from-a
EOF
cat >"$TESTDIR/b/pathcachecmd" <<'EOF'
#!/bin/sh
echo from-b
EOF
chmod +x "$TESTDIR/a/pathcachecmd" "$TESTDIR/b/pathcachecmd"

OLDPATH="$PATH"
OUTFILE=$(mktemp)
PATH="$TESTDIR/a:$OLDPATH"
pathcachecmd >"$OUTFILE"
PATH="$TESTDIR/b:$OLDPATH"
pathcachecmd >>"$OUTFILE"
PATH="$OLDPATH"

IFS= read -r LINE1 <"$OUTFILE"
assert_equal "from-a" "$LINE1" "a command must resolve against PATH as it was at the time it's first run"

{ IFS= read -r _; IFS= read -r LINE2; } <"$OUTFILE"
assert_equal "from-b" "$LINE2" "the same command name run again after PATH changes must re-resolve, not reuse the stale cached path"

rm -rf "$TESTDIR"
rm -f "$OUTFILE"

## fixes/48: a plain foreground external command (no "&") never got a
## struct job at all -- exec_program.c's non-X_NOWAIT path called
## job_wait(NULL, pid, &status) (the pid-only branch, no job tracking
## whatsoever), unlike the X_NOWAIT/backgrounded path just above it in
## the same function. So a foreground command that got Ctrl-Z'd
## (SIGTSTP) could never be resumed via fg/bg -- there was no job in
## job_list to resume, and nothing recorded its stop in the first
## place. Fixed by creating a real struct job for this path too (mask
## job->bgnd = 0 so job_wait() doesn't print a redundant "Done"
## banner), building job->command from argv directly (nothing else
## downstream gets a chance to -- eval_simple_command.c only does that
## for "ncmd->bgnd", by reading back *job_pointer once exec_command()
## returns, which for a foreground job that already ran to completion
## no longer even points at this job, since job_wait() already freed
## it by then), and mirroring job_fork()'s job_pgrp bookkeeping so a
## stopped-then-resumed job correctly hands the terminal back to the
## shell once it's actually done (without this, the terminal would
## have stayed with the exited child's defunct process group forever,
## wedging the shell's own next terminal read behind a SIGTTIN).
##
## The actual "stop it, fg it, it resumes" behavior needs a real
## foreground process group receiving a real SIGTSTP (or "kill -STOP"
## sent to a pid a *script* has no way to learn, since a foreground
## command was never backgrounded and so never got a "$!") -- not
## reproducible here. Verified instead with a pty-simulated session
## (python's `pty` module, same technique as fixes/36): typed "sleep
## 2" as a genuine foreground command, sent SIGSTOP to the resulting
## child (found via /proc), and confirmed "[1]+  Stopped   sleep 2"
## appeared (not "(null)"), "fg" resumed it and it ran to completion,
## and "jobs" afterward was empty (fully reaped, not left dangling).
## Also confirmed several plain foreground external commands in a row
## still work normally and don't leave the terminal wedged (each one
## now takes and hands back real process-group ownership, where
## before this fix nothing did).
X=$(true; echo one; false; echo status=$?)
assert_equal "one
status=1" "$X" "plain foreground external/builtin commands must still work normally now that they get a real struct job"

## fixes/49: quoted "$(cmd)" didn't suppress field splitting --
## expand_command() correctly received X_QUOTED (set from the
## parser's S_DQUOTED/S_SQUOTED/... flag on the N_ARGCMD node, same
## mechanism a quoted plain string or "$var" already used), but then
## explicitly stripped it ("flags & (~(X_QUOTED))") right before
## passing the substituted output to expand_cat() -- the one function
## that actually decides whether to split on IFS. Fixed by passing
## flags through unchanged, same as every other expand_arg.c call
## site (expand_param(), expand_cat() for a literal string part) that
## doesn't second-guess its caller's quoting.
X=$(set -- "$(printf "a b c")"; echo $#)
assert_equal "1" "$X" "a quoted \"\$(cmd)\" producing space-separated output must stay one word"

## IFS is reset explicitly here rather than relied on as inherited --
## an unrelated, separate bug ("IFS= read" leaking its prefix
## assignment past the command, since builtin_read.c is misclassified
## as a POSIX special builtin) can leave it emptied out by an earlier
## "IFS= read ..." elsewhere in this same file.
X=$(unset IFS; set -- $(printf "a b c"); echo $#)
assert_equal "3" "$X" "an UNQUOTED \$(cmd) must still field-split on IFS as before"

X=$(x="$(printf "a b c")"; echo "[$x]")
assert_equal "[a b c]" "$X" "a quoted \"\$(cmd)\" assigned to a variable must keep its internal spaces"

X=$(set -- "`printf "a b c"`"; echo $#)
assert_equal "1" "$X" "a quoted backquoted command substitution must also stay one word, not just \"\$(...)\""

## fixes/50: "read" was classified as a POSIX special builtin
## (B_SPECIAL/H_SBUILTIN), which is wrong -- it isn't one of the
## special builtins POSIX 2.14 actually lists. Consequence:
## "IFS=x read line"'s prefix assignment persisted past the command
## instead of being scoped to it, since eval_simple_command.c only
## pushes a temporary var scope for prefix assignments when
## "cmd.id != H_SBUILTIN". Reclassifying "read" as a regular builtin
## alone made things *worse*, though: the temporary scope
## vartab_push() creates was pushed as function=0, so var_create()'s
## existing walk-past-transient-scope logic (already used to skip a
## *function call's* own scope for a plain, non-"local" assignment)
## didn't recognize this scope as transient too -- "read"'s own
## var_setv() call for its target variable(s) landed in the temp
## scope and got silently discarded along with the prefix assignment
## when it was popped, and the same happened for any function called
## with a prefix assignment that made a plain (non-"local") global
## assignment of its own. Fixed by pushing the temp scope as
## function=1 (so those writes correctly walk past it to the real
## enclosing scope) while forcing the prefix assignment itself to
## land in that exact scope via V_LOCAL (bypassing the same walk, so
## it doesn't escape into whatever outer scope an existing
## same-named variable happens to live in).
X=$(
  echo "before=[$IFS]"
  IFS= read -r x <<EOF
hello
EOF
  echo "after=[$IFS]"
)
assert_equal "before=[$IFS]
after=[$IFS]" "$X" "\"IFS=x read ...\"'s prefix assignment must not persist past the command"

X=$(IFS=: read -r a b <<EOF
x:y
EOF
echo "a=[$a] b=[$b]")
assert_equal "a=[x] b=[y]" "$X" "\"read\"'s own prefix-scoped IFS must still take effect during the read itself"

X=$(
  myfn() { GLOBALVAR=set-by-fn; }
  FOO=bar myfn
  echo "GLOBALVAR=[$GLOBALVAR] FOO=[$FOO]"
)
assert_equal "GLOBALVAR=[set-by-fn] FOO=[]" "$X" "a function called with a prefix assignment must keep its own plain (non-local) global assignment, and the prefix assignment must not leak"

X=$(
  X=outer
  f() { local X=inner; echo "in=$X"; }
  FOO=bar f
  echo "after=$X"
)
assert_equal "in=inner
after=outer" "$X" "\"local\" inside a function called with a prefix assignment must still shadow correctly"

## fixes/51: a here-document whose delimiter is quoted must suppress
## parameter/command/arithmetic expansion in its body (POSIX 2.7.4),
## but redir_source.c only checked the delimiter word's S_ESCAPED flag
## (set for a lone backslash escape, e.g. "<<\EOF") when deciding
## whether to suppress expansion -- missing S_TABLE entirely, the
## flag bits that actually record single/double-quoting (S_SQUOTED/
## S_DQUOTED). It also only ever looked at the top node's flags,
## which for a delimiter mixing quoted and unquoted parts (e.g.
## "E\"O\"F") is an N_ARG wrapper whose own flag field is never set at
## all -- every sub-part needs checking, not just the top node.
X=$(X=hi; cat <<'EOF'
literal $X
EOF
)
assert_equal "literal \$X" "$X" "a single-quoted heredoc delimiter must suppress expansion in the body"

X=$(X=hi; cat <<"EOF"
literal $X
EOF
)
assert_equal "literal \$X" "$X" "a double-quoted heredoc delimiter must suppress expansion in the body"

X=$(X=hi; cat <<\EOF
literal $X
EOF
)
assert_equal "literal \$X" "$X" "a backslash-escaped heredoc delimiter must suppress expansion in the body"

X=$(X=hi; cat <<EOF
literal $X
EOF
)
assert_equal "literal hi" "$X" "an UNQUOTED heredoc delimiter must still expand its body as before"

X=$(X=hi; cat <<E"O"F
literal $X
EOF
)
assert_equal "literal \$X" "$X" "a heredoc delimiter with only PART of it quoted must still suppress expansion"

## fixes/52 (printf-field-width): builtin printf ignored flags/width/
## precision on numeric (and %s/%c) conversions -- the format string
## was printed back completely unprocessed instead of being applied,
## and %X printed lowercase hex digits instead of uppercase.
X=$(printf "%08x\n" 255)
assert_equal "000000ff" "$X" "printf %08x must zero-pad a hex conversion to the given width"

X=$(printf "%X\n" 255)
assert_equal "FF" "$X" "printf %X must print uppercase hex digits"

X=$(printf "%5d|\n" 42)
assert_equal "   42|" "$X" "printf %5d must space-pad a decimal conversion to the given width"

X=$(printf "%-5d|\n" 42)
assert_equal "42   |" "$X" "printf %-5d must left-justify within the given width"

X=$(printf "%+d\n" 42)
assert_equal "+42" "$X" "printf %+d must force a sign on a positive value"

X=$(printf "%.3d\n" 5)
assert_equal "005" "$X" "printf %.3d must zero-extend a decimal conversion to the given precision"

X=$(printf "%-8s|\n" hi)
assert_equal "hi      |" "$X" "printf %-8s must left-justify a string within the given width"

X=$(printf "%05d\n" -5)
assert_equal "-0005" "$X" "printf %05d must keep the sign before zero-padding a negative value"

## fixes/53 (misclassified-special-builtins): "alias", "getopts",
## "local", "umask", and "unalias" were all marked B_SPECIAL in
## builtin_table.c even though none of them are in POSIX 2.14's actual
## special-builtin list -- the same misclassification "read" had
## (fixes/50), with the same symptom: a prefix assignment on the
## command leaked past it instead of being scoped to just that command.
DEFAULT_IFS=$(echo "[$IFS]")

X=$(IFS=: getopts ":a:" opt "-a" "x:y" >/dev/null 2>&1; echo "[$IFS]")
assert_equal "$DEFAULT_IFS" "$X" "a prefix assignment on \"getopts\" must not leak past the command"

X=$(IFS=: alias foo=bar; echo "[$IFS]")
assert_equal "$DEFAULT_IFS" "$X" "a prefix assignment on \"alias\" must not leak past the command"

X=$(alias foo=bar; IFS=: unalias foo; echo "[$IFS]")
assert_equal "$DEFAULT_IFS" "$X" "a prefix assignment on \"unalias\" must not leak past the command"

X=$(IFS=: umask >/dev/null; echo "[$IFS]")
assert_equal "$DEFAULT_IFS" "$X" "a prefix assignment on \"umask\" must not leak past the command"

X=$(f() { IFS=: local x=1; echo "[$IFS]"; }; f)
assert_equal "$DEFAULT_IFS" "$X" "a prefix assignment on \"local\" must not leak past the command"

## fixes/54 (quoted-at-empty-param-split): a quoted "$@" silently
## dropped empty positional parameters instead of expanding them to an
## empty word -- expand_param()'s special-parameter branch only ever
## set its "v" (value) pointer when the built stralloc was non-empty,
## so an empty positional parameter fell through to the same "v is
## NULL" path used for an actually-unset parameter and contributed no
## argument node at all, despite the code's own comment ("special
## parameters are always set"). Found and fixed alongside a second,
## independent bug hit while testing the unquoted case: an unquoted
## "$@"/"$*" with an empty positional parameter in the *middle* of the
## list (not first or last) crashed with a segfault, because the
## S_ARGVS loop advanced its node-chaining pointer with "&n->next" even
## when the previous index had contributed no node at all (n == NULL),
## corrupting the chain for every following index.
X=$(f() { test "$@"; echo $?; }; f -n "")
assert_equal "1" "$X" "a quoted \"\$@\" must expand an empty positional parameter to its own (empty) word"

set -- -n ""
X=""
for a in "$@"; do X="$X[$a]"; done
assert_equal "[-n][]" "$X" "a for-loop over a quoted \"\$@\" must iterate an empty positional parameter as its own (empty) word"

set -- a "" c
X=""
for a in $@; do X="$X[$a]"; done
assert_equal "[a][c]" "$X" "a for-loop over an unquoted \$@ with an empty positional parameter in the middle must not crash, and must still drop the empty word via field splitting"

## fixes/55 (fd-close-noop): ">&-"/"<&-" didn't really close a file
## descriptor -- fd_null() (its only caller, redir_dup()'s "-" branch)
## swapped in a null-sink buffer pair instead, so writes/reads against
## the "closed" fd silently succeeded against nothing instead of
## failing, and an external command exec'd afterward still inherited
## the real, still-open kernel descriptor untouched. Fixed by making
## the closed fd's entry fail FD_ISRD()/FD_ISWR() (so a later fd_dup()
## against it -- what both builtins and external commands' own
## redirections go through -- fails exactly like a real closed
## descriptor would) and by real close()ing the underlying kernel
## descriptor in fdtable_resolve() right before an execve() would
## otherwise hand it to a child process.
## note: fd 9 is used here (rather than a low number like 3) because,
## at the time this was written, a fd number a caller of shish already
## had open (e.g. a launcher's own pipe) hit a separate bug --
## fdstack-scope-chain-mislink, since fixed (fixes/58) -- where a
## nested scope's new struct for that number got linked as a *child*
## of the ancestor scope's already-tracked entry instead of replacing
## it as the visible top. Left on fd 9 rather than reverted back to a
## low number: it costs nothing and keeps this test independent of
## that other fix.
(exec 9>&-
echo hi >&9 2>/dev/null
STATUS=$?
assert_equal "1" "$STATUS" "\"echo >&9\" after \"exec 9>&-\" must fail instead of silently succeeding")

(exec 9>&-
X=$(echo hi >&9 2>/dev/null)
assert_equal "" "$X" "\"echo >&9\" after \"exec 9>&-\" must not produce any output")

(exec 9<&-
read X <&9 2>/dev/null
STATUS=$?
assert_equal "1" "$STATUS" "\"read <&9\" after \"exec 9<&-\" must fail instead of silently succeeding")

TMPFILE=$(mktemp)
(exec 9>"$TMPFILE"
echo before >&9
echo hidden 9>&- >&9 2>/dev/null
echo after >&9)
X=$(cat "$TMPFILE")
assert_equal "before
after" "$X" "a temporary (non-\"exec\") \">&-\" on one command must not close the fd for commands after it"
rm -f "$TMPFILE"

## fixes/56 (dash-c-for-loop-parse-error): a compound command whose
## closing keyword ("done"/"fi"/...) landed exactly at end-of-input --
## with no trailing whitespace/newline after it, e.g. a "-c" argument
## (which unlike a script file has no trailing newline unless the
## caller added one) -- failed to parse with "unexpected token EOF,
## expecting 'done'". parse_unquoted() only recognizes a keyword when
## it can peek a delimiter char *after* the word (see its
## parse_isctrl()/parse_isspace() branch); at true EOF there's no such
## char to peek, so parse_word() fell through to its final
## parse_string() call, which cleared p->sa (the accumulated "done")
## before parse_gettok()'s own fallback keyword check ever ran against
## it -- so the fallback always saw an empty string. Reproduced here
## by sourcing a file with no trailing newline (same underlying
## source_peek()-hits-true-EOF mechanism as "-c", without needing to
## invoke a second shish process to prove it).
TMPFILE=$(mktemp)
printf 'for i in a b c; do echo "[$i]"; done' >"$TMPFILE"
X=$(. "$TMPFILE")
assert_equal "[a]
[b]
[c]" "$X" "a \"for\" loop whose closing \"done\" lands exactly at end-of-input must still parse"
rm -f "$TMPFILE"

TMPFILE=$(mktemp)
printf 'if true; then echo hi; fi' >"$TMPFILE"
X=$(. "$TMPFILE")
assert_equal "hi" "$X" "an \"if\" whose closing \"fi\" lands exactly at end-of-input must still parse"
rm -f "$TMPFILE"

## fixes/57 (last-command-status-not-propagated): a script's/"-c"
## string's own process exit status didn't reflect its last command's
## natural (non-"exit") failure -- only an explicit "exit N" ever
## changed it. Two separate spots hardcoded a 0: sh_main.c called
## "sh_exit(0)" unconditionally after sh_loop() returned (the path a
## script file with a trailing newline after its last command takes),
## and sh_loop.c itself called "sh_exit(p.tok != T_EOF)" when the last
## command wasn't followed by a newline/semicolon (the path "-c 'cmd'"
## takes, since a "-c" argument has no trailing newline unless the
## caller added one) -- both now use sh->exitcode instead. Also fixed:
## "$?" right after a command-substitution-only assignment ("X=$(cmd)"
## with no command name) reflected a stale, always-0 per-command
## eval frame value instead of the substitution's real status.
##
## the sh_loop.c half needs its own subprocess/subshell to observe (it
## calls sh_exit(), which -- since a plain, non-forking "(...)"
## subshell here still sets up a matching longjmp target, see
## eval_subshell.c -- unwinds only the subshell rather than the whole
## test process, which is why this is wrapped in one).
TMPFILE=$(mktemp)
printf 'false' >"$TMPFILE"
(. "$TMPFILE")
STATUS=$?
assert_equal "1" "$STATUS" "sourcing a file whose last command (no trailing newline) fails must propagate that status"
rm -f "$TMPFILE"

X=$(false)
assert_equal "" "$X" "sanity: \$(false) itself still produces no output"
false; X=5; STATUS_AFTER_PLAIN_ASSIGN=$?
assert_equal "0" "$STATUS_AFTER_PLAIN_ASSIGN" "a plain assignment with no command substitution must reset status to 0, not carry over the previous command's"

false
X=$(true)
assert_equal "0" "$?" "\"X=\$(cmd)\" with no command name must reflect the substitution's own (successful) status"

true
X=$(exit 5)
assert_equal "5" "$?" "\"X=\$(cmd)\" with no command name must reflect the substitution's own (failing) status"

false
X=$?
assert_equal "1" "$X" "\"X=\$?\" must still capture the PRECEDING command's status, not be treated as a substitution"

## fixes/58 (fdstack-scope-chain-mislink): when a fd number already
## had an entry tracked by an ANCESTOR fdstack scope and a NESTED
## scope (a subshell here) redirected that same number,
## fdstack_search()'s walk left fdtable_pos pointing at the ancestor
## struct's own "parent" slot instead of at the top-level fdtable[]
## slot, so fdtable_newfd()'s subsequent fdtable_link() linked the new
## struct in *underneath* the ancestor entry rather than replacing it
## as what's visible -- fdtable[n] kept resolving to the ancestor's
## original (still fully open) descriptor, so the nested scope's own
## close/redirect of that fd was invisible to anything that looked the
## fd number up afterward, INCLUDING the ancestor scope once the
## nested one popped (fdtable_unlink() restores *fd->pos = fd->parent,
## which -- since fd->pos pointed at the ancestor's own parent slot,
## not at fdtable[n] -- clobbered the ancestor's own ->parent instead
## of fdtable[n], though that corruption isn't itself exercised here).
TMPFILE=$(mktemp)
exec 4>"$TMPFILE"
echo before >&4
(exec 4>&-
echo hidden >&4 2>/dev/null)
STATUS=$?
echo after >&4
assert_equal "1" "$STATUS" "a nested subshell closing a fd an ancestor scope already had open must actually fail the redirection, not silently succeed"
X=$(cat "$TMPFILE")
assert_equal "before
after" "$X" "the ancestor scope's own fd must keep working after the nested scope's close/pop, with nothing leaked in between"
rm -f "$TMPFILE"

## fixes/59 (nested-backquote): nested backquote command substitution
## (inner backquotes escaped with backslash, per POSIX 2.6.3) didn't
## work -- the inner backquotes were left completely unprocessed,
## becoming literal text instead of a nested substitution.
## parse_bquoted() used to parse a backquoted substitution's body
## directly off the live source stream, the same way "$( )" already
## correctly does -- but unlike "$( )", a backquoted substitution's
## open and close delimiter are the *same* character, so a nested one
## is only distinguishable via backslash-escaping, which a single
## recursive-descent pass over the live source can't resolve on its
## own (by the time a "\`" is seen, there's no way to know yet whether
## it's the start of a nested substitution or the escaped end of the
## current one -- that depends on what comes *after* it). Fixed by
## collecting the whole backquoted body as raw text first (unescaping
## "\`"/"\$"/"\\" per POSIX 2.6.3 along the way), then reparsing that
## text as its own independent script -- exactly the same two-pass
## approach POSIX shells use, and exactly how a "-c" argument or a
## ". "'d file already gets parsed here.
X=$(nested=`echo \`echo inner\``; echo "$nested")
assert_equal "inner" "$X" "a backquote substitution nested inside another (backslash-escaped) must actually run, not be left as literal text"

## fed through a variable rather than used as a direct unquoted
## argument -- a direct unquoted backquote substitution's own output
## doesn't field-split on IFS at all, a separate, pre-existing bug
## (bquote-direct-no-field-split) unrelated to nesting; see BUGS.
X=$(Y=`echo \`echo a; echo b\``; echo $Y)
assert_equal "a b" "$X" "a nested backquote substitution's own multiple commands/words must all run and field-split normally"

X=$(echo `echo $(echo mixed)`)
assert_equal "mixed" "$X" "a \"\$( )\" substitution nested inside a backquoted one (no escaping needed for \"\$( )\") must still work"

X=$(echo $(echo `echo mixed2`))
assert_equal "mixed2" "$X" "a backquoted substitution nested inside a \"\$( )\" one must still work (this direction never needed escaping and worked already)"

## the escaped backslash isn't placed at the very end of the nested
## body on purpose -- an escape sequence with nothing at all after it
## once reparsed hits a separate, pre-existing "escape at end-of-
## input" parser gap (the same one "an unterminated substitution
## silently accepts EOF as its close" falls out of), unrelated to
## nesting.
X=$(V=1; echo `echo \$V and \\a-backslash and \`echo nested\``)
assert_equal "1 and a-backslash and nested" "$X" "\"\\\$\" and \"\\\\\" inside a backquoted substitution must still unescape to \"\$\"/\"\\\" as POSIX 2.6.3 requires, alongside the new \"\\\`\" handling"

X=$(echo `echo plain`)
assert_equal "plain" "$X" "a plain (non-nested) backquote substitution must still work exactly as before"

## fixes/60 (bquote-direct-no-field-split): an unquoted backquote
## command substitution used directly as a command's own argument
## (rather than first assigned to a variable) didn't field-split on
## IFS. parse_bquoted.c set the N_ARGCMD node's flag to
## "S_BQUOTE | p->quot" -- but S_BQUOTE (0x04) shared the low nibble
## expand_arg.c masks off with S_TABLE (0x0f) to detect quoting, so an
## UNQUOTED backquote substitution (p->quot == Q_UNQUOTED == 0) still
## produced a nonzero "flag & S_TABLE", which expand_arg.c mistook for
## "this word part is quoted" and suppressed its splitting. S_BQUOTE
## marks the *syntax* a substitution was written with (for tree_cat()
## alone, see fixes/61 below) and has nothing to do with whether its
## *result* is quoted -- moved it out of the S_TABLE-masked nibble
## into its own, non-overlapping bit.
X=$(echo `echo a; echo b`)
assert_equal "a b" "$X" "an unquoted backquote substitution used directly as an argument must field-split on IFS, like \"\$( )\" and a variable already did"

X=$(echo "`echo a; echo b`")
assert_equal "a
b" "$X" "a QUOTED backquote substitution used directly as an argument must still NOT field-split"

## fixes/61 (tree-cat-nested-backquote-unescaped): tree_cat() (used by
## shformat, and by "set"'s own function-definition dump, exercised
## here without needing to locate/invoke the separate shformat binary)
## reprinted a N_ARGCMD node written with backquotes as a bare
## "`...`" unconditionally, with no awareness of whether it was
## already nested inside another backquoted substitution being
## printed -- so a nested backquote substitution's own backquotes came
## out unescaped, producing text that no longer parses the way the
## original source did (POSIX 2.6.3 requires a backslash before each
## nesting level's backquotes, see fixes/59). Fixed by always
## re-emitting "$( )" for anything nested inside a backquote-printed
## substitution's own body, regardless of how it was originally
## written -- "$( )" doesn't have backquote's escaping problem in the
## first place, so there's nothing to get wrong at any further depth.
X=$(uniquefn987() { echo `echo \`echo inner\``; }
set | grep -A2 "^uniquefn987")
assert_equal 'uniquefn987() {
  echo `echo $(echo inner)`;
}' "$X" "reprinting a nested backquote substitution must re-emit it as \"\$( )\" so the result still parses back to the same thing"

X=$(uniquefn988() { echo `echo plain`; }
set | grep -A2 "^uniquefn988")
assert_equal 'uniquefn988() {
  echo `echo plain`;
}' "$X" "reprinting a plain (non-nested) backquote substitution must still keep its original backquote syntax"

## fixes/62 (subshell-function-body-isolation): a function defined
## with a subshell as its body ("f() ( ... )") didn't actually isolate
## variable assignments into a subshell the way a bare "( ... )"
## subshell does -- exec_command.c's H_FUNCTION case always evaluated
## the body via eval_cmdlist() regardless of whether it had been
## parsed as N_SUBSHELL or N_BRACEGROUP, unlike eval_command.c's own
## dispatch for a standalone grouping (N_SUBSHELL -> eval_subshell(),
## which does the actual isolating; N_BRACEGROUP -> eval_cmdlist(),
## no isolation) -- so a function's own "(...)" body ran exactly like
## "{...}" would, sharing the caller's environment. Fixed by making
## exec_command.c dispatch the same way.
X=$(subiso_f() ( X=inner ); X=outer; subiso_f; echo "$X")
assert_equal "outer" "$X" "a function whose body is \"(...)\" must isolate its own assignments from the caller, exactly like a bare \"(...)\" subshell"

X=$(subiso_f() ( exit 5 ); subiso_f; echo "$?")
assert_equal "5" "$X" "a subshell-bodied function's own exit status must still propagate to its caller"

X=$(subiso_f() ( echo "arg=$1" ); subiso_f hello)
assert_equal "arg=hello" "$X" "a subshell-bodied function must still see its own call arguments"

## fixes/63 (cmdsubst-external-stderr-redirect-lost): an external
## command's "2>&1" inside a command substitution didn't get
## captured -- it leaked straight to the shell's own real stderr, and
## the substitution captured nothing for it. Root causes (three,
## compounding):
##
## 1. fdstack_npipes()/fdstack_pipe() stop walking outward at the
##    first fdstack level carrying an FD_SUBST/FD_HERE target, to
##    avoid also wiring a pipe for an OUTER command substitution's own
##    target (see their own comments). But fd_dup() (redir_dup.c, for
##    "2>&1") copies the FD_TYPE bits of whatever it duplicates --
##    which includes FD_STRALLOC, part of FD_SUBST -- onto its own
##    FD_DUP'd struct, at the *redirected command's own* (inner)
##    fdstack level. That level isn't a nested substitution's own
##    target, just an alias of the outer one, but looked exactly like
##    one to the walk, which then stopped one level too early and
##    never reached the real fd 1 target further out at all.
## 2. fdstack_pipe() nulled the real target's "->r" after wiring its
##    read side to the pipe. Nothing in the normal (no duplicate) path
##    needs it (fdstack_data(), which drains the pipe, reads via
##    "->rb.fd" directly) -- but fdstack_unref(), which hands off
##    pipe ownership to a surviving duplicate when the original is
##    popped, dereferences "->r" as a byte_copy() source, segfaulting
##    on the now-reachable real target once (1) was fixed.
## 3. fdstack_pipe() installs a brand new struct as the live occupant
##    of the target fd's slot (to hold the real pipe write end) but
##    left any duplicate created *before* it ran (like "2>&1",
##    evaluated during the command's own earlier redirect loop) still
##    pointing at the old, now-shadowed struct, which never gets a
##    real descriptor -- dup2()ing an unresolved -1 for the child once
##    (1) and (2) were fixed. Fixed by repointing any such duplicate's
##    ->dup to the new struct.
X=$(ls /nonexistent-dir-for-bug-repro-fixes63 2>&1)
assert_match "$X" "*nonexistent-dir-for-bug-repro-fixes63*" "an external command's \"2>&1\" inside a command substitution must actually be captured, not leak to the shell's real stderr"

X=$(ls /nonexistent-dir-for-bug-repro-fixes63 2>/dev/null)
assert_equal "" "$X" "sanity: without \"2>&1\", an external command's stderr must still NOT be captured"

## fixes/64 (fdstack-push-assertion-cmdsubst-redir): fdstack_push()'s
## own sanity assertion ("st < fdstack || fdstack == &fdstack_root")
## fired and aborted the shell for any redirection at all inside a
## command substitution, under an ASan/Debug build. The check compared
## the new frame's raw stack address against the current top,
## assuming normal downward stack growth so a callee's (deeper) frame
## always sits below its caller's -- not a portable invariant:
## AddressSanitizer's stack-use-after-return detection allocates
## "escaping" locals like this one (its address is stored into the
## global "fdstack" pointer) on a separate fake stack, in no
## particular order relative to each other, so the comparison was a
## false positive. Fixed by replacing it with a meaningful, portable
## check -- "st != fdstack", matching the sibling vartab_push()'s own
## "vartab != varstack" pattern -- instead of the address comparison.
## No assertion added here: assert() is a no-op under the NDEBUG this
## repo's default (non-Debug) CMake build type defines, so nothing in
## a normal "ctest" run exercises fdstack_push()'s assertion either
## way, and the crash itself only reproduced under ASan specifically
## (confirmed via ASAN_OPTIONS=detect_stack_use_after_return=0 making
## it disappear even under ASan). Verified instead by building with
## "-DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_FLAGS=-fsanitize=address,undefined"
## and confirming "shish -c 'x=$(true 2>&1)'" no longer aborts.

## fixes/65 (expand-glob-stack-overflow): a stack-buffer-overflow in
## expand_glob() at src/expand/expand_glob.c:48, the glob(3) call
## itself. expand_glob.c always used lib/glob.h's own, much smaller
## glob_t layout ("#if 0 // defined(HAVE_GLOB_H) || ..." -- the
## intended condition had been disabled, unconditionally falling back
## to the "#else" branch) regardless of platform. But lib/unix/glob.c
## -- the project's own glob()/globfree() implementation, the only
## thing that could make that small layout correct -- has its entire
## body guarded by "#if WINDOWS_NATIVE"; on any non-Windows build it
## compiles to nothing, so HAVE_GLOB there actually means the
## platform's real libc glob() gets linked in instead. glibc's real
## glob_t is considerably larger (several GLOB_ALTDIRFUNC callback
## members lib/glob.h's doesn't have), so libc's glob() wrote past the
## end of the too-small on-stack glob_t on every non-Windows build --
## corrupting the stack on ANY unquoted word containing a glob-special
## character ("*?[]\", see parse_isesc()), including innocuous cases
## like the test builtin's "[" name itself (S_GLOB gets set on it same
## as a real wildcard, glob(3) just reports no match for a literal
## "["). Fixed by using the real <glob.h>/glob_t whenever HAVE_GLOB
## means the system's own glob() is what gets linked, matching what
## the code already did for the glob() *call* just below (also
## "#ifdef HAVE_GLOB"), instead of an unconditional small-struct
## fallback.
##
## The corruption doesn't reliably crash outside of ASan (silently
## overwrites otherwise-unused stack space in a plain build) -- the
## crash itself was verified via ASan specifically:
## "-DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_FLAGS=-fsanitize=address,undefined",
## then "shish -c '[ 1 = 1 ]'" (previously a guaranteed stack-buffer-
## overflow abort every time, since "[" alone is enough to trigger it)
## no longer aborts. Functional coverage below is a normal-build
## regression check that the affected code paths still behave
## correctly, not a crash reproduction.
assert_equal "0" "$( [ 1 = 1 ]; echo $? )" "the \"[\" test builtin (itself a glob-special character, see fixes/65) must still work correctly"
assert_equal "1" "$( [ 1 = 2 ]; echo $? )" "sanity: \"[\" must still correctly report a false comparison"

## fixes/66 (glob-not-triggered-for-plain-arguments): filename
## globbing ("*"/"?"/"[]" in an ordinary, unquoted command argument)
## never actually ran glob(3) at all -- three separate, compounding
## bugs, each masking the next:
##
## 1. expand_cat.c's word-splitting loop unconditionally called
##    expand_escape() on every literal text chunk, which blindly
##    prepends a backslash to *every* occurrence of "\*?[" -- turning
##    a bare, user-intended wildcard into an escaped literal before it
##    ever reached expand_glob()/glob(3), and double-escaping an
##    already backslash-protected literal on top of that. A stale,
##    never-set "X_ESCAPE" flag and a commented-out conditional right
##    next to the active code showed this was supposed to be
##    conditional, never unconditional. Fixed by using a plain copy
##    instead: parse_unquoted.c already leaves a bare wildcard bare
##    and an escaped literal already in glob(3)'s own "\X" syntax, so
##    no further escaping was ever needed here. (expand_escape() and
##    the dead X_ESCAPE flag removed entirely -- nothing else used
##    either.)
## 2. Even once (1) let a bare wildcard survive, parse_unquoted()'s
##    S_GLOB flag -- tracked in a local variable -- was lost whenever
##    the glob pattern was the last thing before true end-of-input
##    (e.g. a "-c" argument with nothing after it, which unlike a
##    script file has no trailing newline): the EOF branch returned
##    without ever persisting it, the same root cause
##    dash-c-for-loop-parse-error (fixes/56) had for keyword
##    recognition, just for a different flag. Fixed by mirroring the
##    delimiter branch exactly: try a keyword match first (so fixes/56
##    keeps working), and only if that fails, flush with the
##    locally-tracked flags instead of parse_word()'s own flags-less
##    fallback flush.
## 3. Once glob(3) actually started running (after (1) and (2)),
##    expand_glob()'s own loop building the result nodes reused the
##    *first* match's node (the one that held the original pattern
##    text) via stralloc_zero(), which only resets the logical length,
##    not the underlying buffer -- so a shorter match left the
##    pattern's own leftover tail bytes physically sitting right after
##    it, and nothing re-terminated the string at the new, shorter
##    length. Anything reading it as a plain C string read straight
##    through into that stale tail (e.g. a pattern matching "a.txt"
##    where the original pattern text happened to also end in "txt"
##    printed "a.txttxt"). Fixed by NUL-terminating each match's node
##    right after filling it in.
TMPDIR_GLOB=$(mktemp -d)
touch "$TMPDIR_GLOB/a.txt" "$TMPDIR_GLOB/b.txt"

X=$(echo "$TMPDIR_GLOB"/*.txt)
assert_equal "$TMPDIR_GLOB/a.txt $TMPDIR_GLOB/b.txt" "$X" "a bare wildcard as a command argument must actually glob-expand, even as the very last thing before end-of-input"

X=$(echo "$TMPDIR_GLOB"/*.txt more)
assert_equal "$TMPDIR_GLOB/a.txt $TMPDIR_GLOB/b.txt more" "$X" "a bare wildcard followed by more words must still glob-expand"

X=$(for f in "$TMPDIR_GLOB"/*.txt; do echo "[$f]"; done)
assert_equal "[$TMPDIR_GLOB/a.txt]
[$TMPDIR_GLOB/b.txt]" "$X" "a wildcard in a \"for\" loop's word list must glob-expand into separate words"

X=$(echo "$TMPDIR_GLOB"/*.nomatch)
assert_equal "$TMPDIR_GLOB/*.nomatch" "$X" "a wildcard with no matches must fall back to the literal pattern text"

X=$(echo "$TMPDIR_GLOB"/\*.txt)
assert_equal "$TMPDIR_GLOB/*.txt" "$X" "a backslash-escaped wildcard must stay a literal, not glob-expand"

X=$(echo "$TMPDIR_GLOB"/[ab].txt)
assert_equal "$TMPDIR_GLOB/a.txt $TMPDIR_GLOB/b.txt" "$X" "a bracket-expression wildcard must glob-expand without leftover garbage appended to the first match"

rm -rf "$TMPDIR_GLOB"

## fixes/67 (fnmatch-p-hang): lib/path/path_fnmatch.c (the fnmatch(3)
## workalike behind "case" pattern matching, and glob(3)'s own
## bracket-expression fallback) spun at 100% CPU forever on a POSIX
## bracket character class ("[:lower:]", "[:digit:]", etc.) -- the
## branch that was supposed to handle "[:" inside a bracket expression
## was an empty placeholder ("/* TODO: implement them */"), so it
## never advanced past the "[:" it had just recognized, and the
## enclosing "while(plen)" loop just kept re-examining the exact same
## two bytes forever. Fixed by actually implementing POSIX 2.13.1's
## character classes (upper/lower/alpha/digit/alnum/punct/graph/
## print/cntrl/blank/space/xdigit, via <ctype.h>), plus "[.x.]"
## (collating symbol) and "[=x=]" (equivalence class), which -- since
## full locale-aware collation isn't implemented -- degrade to
## matching just the single enclosed character in the C/POSIX locale,
## the only behavior POSIX actually requires a conforming application
## be able to rely on. A malformed/unterminated "[:"/"[."/"[=" no
## longer hangs either: it now falls back to treating the "[" as an
## ordinary literal bracket-expression member, same as any other char.
##
## Not fixed here, confirmed separate and pre-existing (not caused or
## masked by the hang -- the same file's other test blocks running
## after this fix's own now-non-hanging one still fail the same way):
## a bracket-expression range whose endpoints are themselves
## "[.x.]"-wrapped ("[[.0.]-[.2.]]") doesn't match as a range, and
## quoted substrings inside a "case" pattern's bracket expression
## (e.g. "[\".]"/'["."]') don't get their quoting handled correctly.
## See BUGS.
X=$(case a in [[:lower:]]) echo lower;; esac)
assert_equal "lower" "$X" "a POSIX bracket character class must actually match, not hang forever"

X=$(case A in [[:lower:]]) echo lower;; *) echo nomatch;; esac)
assert_equal "nomatch" "$X" "a POSIX bracket character class must correctly NOT match a character outside the class"

X=$(case 5 in [[:digit:]]) echo digit;; esac)
assert_equal "digit" "$X" "a different POSIX bracket character class (:digit:) must also match correctly"

X=$(case a in [[.a.]]) echo matched;; esac)
assert_equal "matched" "$X" "a bracket-expression collating symbol ([.x.]) must degrade to matching its single enclosed character"

X=$(case a in [[=a=]]) echo matched;; esac)
assert_equal "matched" "$X" "a bracket-expression equivalence class ([=x=]) must degrade to matching its single enclosed character"

## no explicit timeout wrapper needed here -- this is exactly the
## shape that used to hang forever before this fix; if it regressed,
## this whole test file (and "ctest") would simply hang too, which is
## its own unmistakable signal.
X=$(case a in [[:) echo unreachable;; *) echo fallback;; esac)
assert_equal "fallback" "$X" "an unterminated/malformed bracket class must fall back to a literal '[' member instead of hanging"

## fixes/68 (dollar-pid-changes-across-fork): "$$" is supposed to stay
## the pid of the originally invoked shell for the whole script, no
## matter how many pipeline/subshell/cmdsubst forks happen along the
## way (this repo's own "./configure --enable-maintainer-mode" relies
## on exactly this to build a stable "conf$$subs.awk" filename it
## reuses across several separately-forked pipeline stages). It used
## to read the same global (sh_pid) that job control's sh_forked()
## updates to the real OS pid on every fork, so each forked pipeline
## member reported a different, wrong "$$". Fixed by giving "$$" its
## own global (sh_shpid), set once at startup and never touched again.
X=$(echo $$)
assert_equal "$$" "$X" "\"\$\$\" inside a command substitution must match the top-level shell's pid"

X=$(echo "pipeline: $$" | cat)
assert_equal "pipeline: $$" "$X" "\"\$\$\" inside a pipeline member (its own forked child) must still match the top-level shell's pid"

X=$( ( echo $$ ) )
assert_equal "$$" "$X" "\"\$\$\" inside a subshell must still match the top-level shell's pid"

## fixes/69 (variable-value-double-unescaped): expand_args()/expand_vars()
## always ran the final assembled argument through
## expand_unescape(parse_isesc), a pass meant to undo the doubling
## parse_squoted.c/parse_dquoted.c/parse_unquoted.c add to protect a
## LITERAL source character from being misread as a real glob
## metachar later on. Parameter/command-substitution results were
## never doubled that way -- they're already real bytes -- so running
## the same pass over them stripped a genuine backslash the
## substituted value contained (this repo's own
## "./configure --enable-maintainer-mode" builds a sed script in a
## variable this way and passes it straight to sed, so every
## backslash in that script lost one level of escaping, corrupting
## the script -- "sed: -e expression #1, char N: unterminated `s'
## command"). Fixed with a new X_LITERAL flag, set only on chunks
## that actually came from source text, gating the unescape call.
## Fixing that surfaced a second bug in the same change: expand_unescape()
## also nul-terminates its stralloc as a side effect, so skipping it
## entirely for substituted content (no literal chunk in the word at
## all) left that argument's buffer not nul-terminated -- anything
## reading it as a plain C string (e.g. the "." builtin building a
## path from a command-substitution result) walked off the end into
## unrelated heap memory. Fixed by nul-terminating unconditionally.
## (single-quoted assignment, not a command-substitution RHS: a
## variable assignment's own "name=" prefix is literal source text
## sharing the same argument buffer as its value, so an assignment
## whose RHS is itself a command substitution containing backslashes
## is a separate, still-open, pre-existing issue -- confirmed
## unaffected by this fix either way -- and deliberately not exercised
## here.)
X='a\\b'
myfunc69a() {
  test "$1" = "$2"
}
myfunc69a 'a\\b' "$X"
assert_equal "0" "$?" "a variable holding literal backslashes must survive being re-substituted (quoted) as a command argument"

myfunc69a 'a\\b' $X
assert_equal "0" "$?" "a variable holding literal backslashes must survive being re-substituted (unquoted) as a command argument"

SAVED_OK69=$ASSERTIONS_SUCCEEDED
SAVED_FAIL69=$ASSERTIONS_FAILED
DIR69=$(dirname "$0")
. "$DIR69/common.sh"
ASSERTIONS_SUCCEEDED=$SAVED_OK69
ASSERTIONS_FAILED=$SAVED_FAIL69
UNRELATED69=$(printf '%s' 'sentinel-value-should-not-be-corrupted')
assert_equal "sentinel-value-should-not-be-corrupted" "$UNRELATED69" "sourcing a file via a command-substitution-built path must not corrupt an unrelated later substitution's buffer"

## fixes/70 (assign-cmdsubst-value-loses-escaping): an assignment's own
## "NAME=" text is literal source (parser-doubled for glob protection,
## same as any other literal word) sharing one argument buffer with
## its value, which fixes/69 left running through a single deferred,
## whole-buffer expand_unescape() pass gated on "does this word
## contain any literal chunk at all" -- correct for a *pure* literal
## or *pure* substitution value, but wrong the moment the two mix in
## one buffer (a plain assignment always mixes them, via its own
## "NAME=" prefix): the literal "NAME=" chunk needing the pass and the
## substituted value chunk that must not get it were indistinguishable
## once concatenated. Fixed by having expand_cat()'s non-splitting
## branch (X_NOSPLIT|X_QUOTED -- what every assignment routes through,
## unconditionally, per expand_vars.c's X_NOSPLIT) unescape each
## literal chunk itself, immediately, before it ever touches the
## shared buffer, so a later substitution chunk's real bytes are never
## touched. Marked via a new X_UNESCAPED flag so expand_args() (command
## arguments, which can still mix a quoted chunk that already went
## through this branch with an unquoted chunk that hasn't) knows not
## to run its own deferred pass a second time over already-final bytes.
Y70='a\\b'
Z70=prefix${Y70}suffix
assert_equal 'prefixa\\bsuffix' "$Z70" "an assignment mixing a literal prefix/suffix with a substituted value containing a real backslash must not lose that backslash"

## the fix above must not regress a plain, fully-quoted command
## argument (no assignment, no mixing at all) -- expand_cat()'s
## self-correction and expand_args()'s deferred pass must not both run
## and strip the backslash twice.
myfunc70() {
  test "$1" = "$2"
}
myfunc70 'a\\b' 'a\\b'
assert_equal "0" "$?" "a plain fully-quoted command argument containing a real backslash must still match itself"

## fixes/71 (heredoc-body-loses-escaping): a here-document body's
## N_ARGSTR chunk(s) were indistinguishable, at the expand-time flag
## level, from ordinary quoted/unquoted literal text -- so they got
## the same expand_unescape() pass that undoes the parser's
## glob-protection doubling, even though parse_here.c's underlying
## parse_squoted()/parse_dquoted() calls skip that doubling entirely
## for P_HERE content (a heredoc body is never pathname-expanded, so
## there's nothing to protect). That pass silently collapsed a genuine
## "\\" in the body down to one backslash. Fixed by tagging every
## heredoc-body chunk with a new S_HEREDOC flag at parse time and
## having expand_arg() leave X_LITERAL off for it.
X71=$(cat <<'INNEREOF'
a\\b
INNEREOF
)
assert_equal 'a\\b' "$X71" "a quoted-delimiter here-document body must preserve a real backslash verbatim"

Y71=world
Z71=$(cat <<INNEREOF2
hello ${Y71} a\\b
INNEREOF2
)
assert_equal 'hello world a\b' "$Z71" "an unquoted-delimiter here-document body must still expand parameters and collapse an escaped backslash to one, matching double-quote rules"

## fixes/72 (configure-summary-test-invalid-expression): "test STRING"
## (or "[ STRING ]") with exactly one argument must always just check
## whether STRING is non-null, per POSIX's argument-count table --
## even when STRING starts with "-" and isn't one of the unary
## operator letters test_unary() recognizes (or even when it *is* one,
## like "-f", but has no following operand). test_unary() used to try
## parsing any single "-..."-shaped argument as a real unary operator
## regardless of whether an operand actually followed, so a single
## argument like "-lm" (autoconf's "if test \"$LIBS\"; then ..." with
## LIBS=-lm) fell through to shell_getopt() finding no matching
## option, returning -1, and the whole "test" call reporting "invalid
## expression" instead of true.
LIBS72=-lm
if test "$LIBS72"; then X72=nonempty; else X72=empty; fi
assert_equal "nonempty" "$X72" "test with a single argument that looks like an unrecognized unary operator must just check non-emptiness"

if test -f; then X72=nonempty; else X72=empty; fi
assert_equal "nonempty" "$X72" "test with a single argument that looks like a real unary operator (missing its operand) must also just check non-emptiness"

if test -f "$0"; then X72=exists; else X72=missing; fi
assert_equal "exists" "$X72" "test -f with an actual operand must still perform the real file test, unaffected by the single-argument fix above"

## fixes/73 (fdtable-cycle-detection): BUGS carried an "illustrative
## (unconfirmed)" example of a redirection chain that "looks" cyclic
## (each hop's fd is itself a dup of the one before), worried that the
## fdtable resolver's recursive "follow whoever occupies the slot I
## want" machinery might spin forever on it. Extensive fuzzing (fd
## rotations of various lengths, the classic 3>&1 1>&2 2>&3 3>&-
## stdout/stderr swap, pipelines combined with dup redirections) never
## found a real infinite cycle -- fd_dup() always flattens a fresh
## redirection's target to its ultimate, already-resolved ancestor at
## setup time, and every redirection clause replaces its slot's
## occupant with a brand new struct, so the dependency graph these
## functions walk can't actually cycle through ordinary syntax. These
## two are exactly that "looks cyclic but isn't" shape, confirmed
## working correctly. Replaced the resolver's previous defense (a raw
## recursion-depth counter, capped at FDTABLE_SIZE / FD_MAX -- deep
## enough to risk a real stack overflow before ever being reached, and
## unable to tell a genuine cycle from a merely long chain) with real
## graph-cycle detection: a stack of the fd numbers actively being
## resolved on the current call chain, checked before each recursive
## call.
OUTFILE73=$(mktemp)
(
exec 3<&0
exec 4<&3
exec 3<&4
head -n1 <&3
) < "$0" > "$OUTFILE73"
assert_equal "DIR=\$(dirname \"\${0}\")" "$(head -n1 "$OUTFILE73")" "a multi-hop fd-alias rotation (3<&0, 4<&3, 3<&4) must still read from the original stdin"
rm -f "$OUTFILE73"

OUTFILE73B=$(mktemp)
ERRFILE73B=$(mktemp)
(
exec 3>&1 1>&2 2>&3 3>&-
echo "went to stderr via fd1"
echo "went to stdout via fd2" >&2
) > "$OUTFILE73B" 2> "$ERRFILE73B"
assert_equal "went to stdout via fd2" "$(cat "$OUTFILE73B")" "the classic 3>&1 1>&2 2>&3 3>&- stdout/stderr swap must land the right text on stdout"
assert_equal "went to stderr via fd1" "$(cat "$ERRFILE73B")" "the classic 3>&1 1>&2 2>&3 3>&- stdout/stderr swap must land the right text on stderr"
rm -f "$OUTFILE73B" "$ERRFILE73B"

## fixes/79 (kill-arg-redirect-parse): a bare-digit redirection prefix
## ("2>word") right after a quoted/expanded argument wasn't recognized
## as a redirection at all when the parser's reused scratch stralloc
## buffer (p->sa) still held stale trailing bytes from an earlier,
## longer token ending in a digit (e.g. "-0", exactly the shape of a
## signal number passed to "kill"). scan_uint() reads p->sa.s as a
## plain C string with no length bound, so without a nul terminator at
## the real end of the current token it kept reading into that
## leftover byte, mismatching p->sa.len and causing the whole
## redirection to be silently skipped and treated as a literal
## trailing word instead.
X79=$(echo -0 "x" 2>/dev/null)
assert_equal "-0 x" "$X79" "a bare-digit redirection after a -N-shaped argument and a quoted word must still be parsed as a redirection"

X79B=$(echo -9 "z" 2>/dev/null)
assert_equal "-9 z" "$X79B" "same bare-digit-redirection case with a different -N digit"

X79C=$(echo a "x" 2>/dev/null)
assert_equal "a x" "$X79C" "a plain (non-dash) preceding argument must still parse the following redirection correctly (no regression)"

## fixes/81 (case-pattern-bracket-quote-stripping, root-caused as
## "case matching wrongly applied pathname-globbing's leading-dot
## rule"): eval_case.c passed SH_FNM_PERIOD to path_fnmatch(), so any
## case statement whose scrutinee started with "." failed to match
## "*"/"?"/a bracket expression at all -- including the universal "*"
## fallback, which normally can never fail to match anything.
X81=$(case "." in *) echo matched;; esac)
assert_equal "matched" "$X81" "case's universal * pattern must match a scrutinee value that starts with a literal dot"

X81B=$(case "." in [.]) echo matched;; *) echo no;; esac)
assert_equal "matched" "$X81B" "a bracket expression explicitly containing a literal dot must match a leading-dot scrutinee"

X81C=$(y="."; case "$y" in ["."]) echo matched;; *) echo no;; esac)
assert_equal "matched" "$X81C" "a bracket expression with a quoted dot inside it must match a leading-dot scrutinee (the original BUGS repro)"

## fixes/82 (case-quoted-bracket-not-literal): a case pattern that is a
## bracket expression *entirely* inside quotes (all of "[", ".", "]"
## quoted, not just the char inside the brackets) must become the
## literal 3-character string "[.]" after quote removal, not stay a
## live bracket expression -- so it must NOT match a bare ".", only
## the literal text "[.]" itself.
X82=$(case \. in "[.]") echo should-not-match;; *) echo no-match;; esac)
assert_equal "no-match" "$X82" "a fully-quoted bracket-expression-shaped case pattern is a literal string, not a live bracket expression"

X82B=$(case "[.]" in "[.]") echo literal-match;; *) echo no;; esac)
assert_equal "literal-match" "$X82B" "a fully-quoted bracket-expression-shaped case pattern must still match its own literal text"

## fixes/82 also had to teach path_fnmatch() that a backslash inside
## an *unquoted* bracket expression escapes the next char to always be
## a literal member (never a closing "]", range dash, or class
## opener) -- otherwise the fix above regressed these two, which
## worked before it.
X82C=$(case \] in [\]] ) echo matched;; esac)
assert_equal "matched" "$X82C" "a backslash-escaped ] inside an unquoted bracket expression must still match a literal ] (no regression)"

X82D=$(case \] in ["]"]) echo matched;; esac)
assert_equal "matched" "$X82D" "a bracket expression with just the closing ] quoted inside it must still match a literal ] (no regression)"

## fixes/83 (expand-param-pattern-leading-dot): like the case-statement
## bug fixed in fixes/82, ${var#pattern}/${var##pattern}/${var%pattern}/
## ${var%%pattern} passed SH_FNM_PERIOD to path_fnmatch(), so a
## "?"/"*"/bracket-expression pattern refused to match a leading "."
## in the parameter's own value.
X83=$(X=".abc"; echo "${X#?}")
assert_equal "abc" "$X83" "\${var#pattern} must match a leading dot with a single-char ? pattern"

X83B=$(X=".abc"; echo "${X##?}")
assert_equal "abc" "$X83B" "\${var##pattern} must also match a leading dot with a single-char ? pattern"

X83C=$(X=".abc"; echo "${X#*.}")
assert_equal "abc" "$X83C" "\${var#pattern} with a leading * must still match through a leading dot"

## no-regression: ordinary (non-leading-dot) prefix/suffix removal
X83D=$(X="foo.bar.baz"; echo "${X%.*}")
assert_equal "foo.bar" "$X83D" "\${var%pattern} smallest-suffix removal is unaffected by the fix (no regression)"

X83E=$(X="foo.bar.baz"; echo "${X##*.}")
assert_equal "baz" "$X83E" "\${var##pattern} largest-prefix removal is unaffected by the fix (no regression)"

## fixes/84 (fnmatch-bracket-collating-range): a bracket-expression
## range whose endpoints are "[.x.]"-wrapped collating symbols (not
## bare characters) must fuse into a single range, matching everything
## in between -- lib/path/path_fnmatch.c used to match each
## "[.symbol.]" as its own standalone member and the "-" in between as
## a third, literal "-" member instead.
X84=$(case 1 in [[.0.]-[.2.]]) echo matched;; *) echo no;; esac)
assert_equal "matched" "$X84" "a range between two [.symbol.] collating-symbol endpoints must match a value inside it"

X84B=$(case 0 in [[.0.]-[.2.]]) echo matched;; *) echo no;; esac)
assert_equal "matched" "$X84B" "a [.symbol.]-[.symbol.] range must match its own start endpoint"

X84C=$(case 2 in [[.0.]-[.2.]]) echo matched;; *) echo no;; esac)
assert_equal "matched" "$X84C" "a [.symbol.]-[.symbol.] range must match its own end endpoint"

X84D=$(case 3 in [[.0.]-[.2.]]) echo matched;; *) echo no;; esac)
assert_equal "no" "$X84D" "a [.symbol.]-[.symbol.] range must not match a value outside it"

X84E=$(case 1 in [0-[.2.]]) echo matched;; *) echo no;; esac)
assert_equal "matched" "$X84E" "a range with one plain-char endpoint and one [.symbol.] endpoint must also fuse correctly"

## no-regression: a standalone collating symbol, a plain range, and a
## trailing literal dash must all still work as before
X84F=$(case a in [[.a.]]) echo matched;; *) echo no;; esac)
assert_equal "matched" "$X84F" "a standalone [.symbol.] (no following range) still matches its single character (no regression)"

X84G=$(case 1 in [0-2]) echo matched;; *) echo no;; esac)
assert_equal "matched" "$X84G" "an ordinary bare-character range is unaffected by the fix (no regression)"

X84H=$(case - in [a-]) echo matched;; *) echo no;; esac)
assert_equal "matched" "$X84H" "a trailing dash right before the closing ] is still a literal member, not a range (no regression)"

## fixes/85: `<<-` only stripped a here-doc body line's leading tabs
## from whatever chunk of `p->sa` was still pending when
## parse_squoted/parse_dquoted returned -- but parse_dquoted flushes
## p->sa into a tree node early, as soon as it hits a `$`/`` ` `` (see
## parse_dquoted.c), which happens *before* parse_here.c's post-hoc
## strip ever runs. So a line whose leading tabs were followed later
## by a parameter/command expansion never got stripped, while a line
## with no expansion (or an expansion right at the start) did. Fixed
## by stripping leading tabs straight off the source at the start of
## each body line, before parse_squoted/parse_dquoted ever reads it.
X85=$(V85=hi; cat <<-EOF
	line with ${V85}
	EOF
)
assert_equal "line with hi" "$X85" "<<- must strip leading tabs even on a line whose tabs are followed by an expansion"

## fixes/86: `redir_source()` reuses one `struct parser` across every
## here-doc queued on the same command line (e.g. `cmd <<A <<B`), one
## `parse_here()` call per queued here-doc. `parse_here()`'s loop
## breaks out as soon as a line matches the delimiter, but that break
## skips the `parse_string()` call that is the only thing that ever
## flushes/clears `p->sa` -- so the matched delimiter line's raw text
## was left sitting in `p->sa` and got prepended onto the first line
## the *next* here-doc in the queue read, corrupting its body. Fixed
## by zeroing `p->sa` at the top of `parse_here()`, alongside the
## existing `p->tree`/`p->node` reset for the same reused-parser case.
X86=$(cat <<A <<B
first
A
second
B
)
assert_equal "second" "$X86" "the later of two here-docs redirected to the same fd must fully win, with no leftover from the first"

X86B=$(cat <<A <<B <<C
one
A
two
B
three
C
)
assert_equal "three" "$X86B" "the same holds with three here-docs queued on one command line"

## fixes/87 (sh-onsig-async-unsafe): sh_onsig(), the SIGCHLD handler,
## used to call term_erase()/term_restore()/prompt_show()/buffer_*()
## directly from signal-handler context -- none of that is
## async-signal-safe, a latent race rather than a deterministic
## failure. None of it is reachable non-interactively (it's all gated
## on an actual terminal/job-control session), so this file can't
## exercise the fix directly the way the rest of it does -- verified
## instead by driving an interactive shish through a pty by hand
## (background two jobs, let SIGCHLD fire while term_read() is
## blocked mid-prompt, confirm no hang/corruption and the right
## "Done"/"Stopped" banners) and by running the full ctest suite
## before and after on a from-scratch checkout, confirming an
## identical pass/fail/timeout count either way. The synchronous half
## of the repro this bug's BUGS entry gave still works as a basic
## smoke test:
sleep 1 &
sleep 2 &
wait
assert_equal "0" "$?" "backgrounding two jobs and waiting on both must still succeed"

## fixes/88: builtin_cat()'s read loop only handled
## buffer_get_until() returning 0 (EOF) or >0 (data) -- a negative
## return (a real read(2) failure, e.g. EBADF off an fd that failed to
## resolve) hit neither branch and the loop just called
## buffer_get_until() again forever. "cat <&-" deterministically hands
## builtin_cat() a closed stdin, independent of whatever originally
## produces the invalid fd (fixes/89 below is one real way to get
## there; this test only needs *a* negative read()). No explicit
## timeout wrapper needed -- this is exactly the shape that used to
## hang forever before this fix; if it regressed, this whole test file
## (and "ctest") would simply hang too, which is its own unmistakable
## signal.
X88=$(cat <&- 2>/dev/null; echo "done_$?")
assert_equal "done_1" "$X88" "cat reading from a closed/invalid fd must report an error and stop, not spin forever"

## fixes/89 (redir-fd-chain-resolves-to-invalid-fd): a plain (non-exec)
## command's redirections are only *recorded* as parsed, with the real
## open()/dup2() deferred until the command actually runs
## (eval_simple_command.c) -- so when "9<in0" is followed later in the
## same command by a "<&"-dup chain ("8<&9 7<&8 ... 0<&3"), fd 9 hasn't
## been opened yet by the time those dups snapshot its (still -1)
## effective fd. Two call sites shared this bug, one per way a command
## actually runs:
##  - exec_command()'s builtin path used to only resolve
##    fd_in/fd_out/fd_err themselves via fdtable_open() -- a no-op for
##    one of these that's a dup (FD_DUP mode) rather than itself
##    pending an open (FD_OPEN mode), so fd 9's real open() never
##    happened at all and fd_in stayed permanently unresolved
##    (confirmed via strace: builtin_cat() ends up read()ing fd -1,
##    EBADF -- see fixes/88 for why that used to hang instead of just
##    failing). Fixed by also opening fd_in/out/err's ->dup source
##    (fd_dup() already flattens ->dup straight to that ultimate
##    source, however deep the chain) if it's still pending, before
##    running the builtin.
##  - fdtable_exec() (forked external commands, right before execve())
##    resolves every virtual fd in ascending order, so it hit fd 0
##    (whose dup source, fd 9, comes *later* in that order) before fd 9
##    ever got a chance to open -- resolving fd 0 first just failed
##    outright, and its return value was silently ignored by its only
##    caller (exec_program.c), so the child execve()d anyway with a
##    broken fd 0 (confirmed: no hang here, just silently empty
##    output). Fixed by having fdtable_exec() open every still-pending
##    real file first, in one pass, before resolving anything else --
##    by the time any dup is resolved, its source (however many other
##    dups share it) is already correct.
IN0=$(mktemp)
echo in0content >"$IN0"
X89=$(cat 9<"$IN0" 8<&9 7<&8 6<&7 5<&6 4<&5 3<&4 0<&3)
assert_equal "in0content" "$X89" "an 8-deep <& dup chain off a freshly-opened fd must still read the right data, not fail/hang on an unresolved fd"

X89B=$(/bin/cat 9<"$IN0" 8<&9 7<&8 6<&7 5<&6 4<&5 3<&4 0<&3 2>/dev/null)
assert_equal "in0content" "$X89B" "the same dup chain must also work for a forked external command, not just a builtin"
rm -f "$IN0"

## fixes/90 (redir-pipeline-builtin-stdin-unbuffered): eval_pipeline.c
## builds a fresh "struct fd" to wrap a pipeline member's stdin (the
## previous member's pipe read end), but never gave it a real buffer --
## fd_init() (via fd_push()) leaves ->r with a NULL, zero-length one.
## That's harmless for a forked *external* program (it never reads
## through this struct at all, just inherits the raw pipe fd via
## dup2()), but a *builtin* runs in-process and reads through
## fd_in->r directly -- read(fd, NULL, 0) is well-defined to return 0
## immediately, which buffer_get_until() can't tell apart from real
## EOF. Any builtin that reads stdin as a non-first pipeline member
## ("... | cat", "... | read x", ...) silently produced no output/no
## value at all, every time, regardless of what the pipe actually
## carried. Found by hand ("echo hi | cat" printed nothing) while
## verifying fixes/87 through fixes/89 -- confirmed present on a
## from-scratch checkout with zero local changes, so pre-existing and
## unrelated to any of those three. Fixed by giving the wrapper struct
## a real buffer whenever fd_needbuf() says it still needs one.
X90=$(echo hi | cat)
assert_equal "hi" "$X90" "a builtin reading stdin as a non-first pipeline member must see the actual piped data, not immediate EOF"

X90B=$(printf 'a\nb\nc\n' | cat -n)
assert_equal "$(printf '    1 a\n    2 b\n    3 c')" "$X90B" "the same holds for a pipeline that actually pushes multiple lines through the builtin"

## fixes/92 (eval-lineno-always-1): eval builds its own in-memory
## source via source_buffer(), which used to always reset to line 1
## (source_push()'s default for a genuinely new file) -- so a $LINENO
## reference inside an eval'd string always printed 1, no matter where
## the eval call itself appeared in the surrounding script. Fixed by
## having source_buffer() seed the new source's starting line from
## parse_lineno for any buffer that has a parent source (i.e. every
## caller except a top-level script/-c source, which still starts
## fresh at line 1 via source_push() directly). Sourcing a separate
## temp file (rather than checking $LINENO against this script's own
## line count) keeps the assertion below stable if lines are ever
## added above it.
F92=$(mktemp)
printf 'echo line1\neval "echo \$LINENO"\n' >"$F92"
X92=$(. "$F92")
rm -f "$F92"
assert_equal "$(printf 'line1\n2')" "$X92" "\$LINENO inside an eval'd string must count from the eval call's own line, not always report 1"

## fixes/93 (dquoted-backslash-newline-continuation): a backslash
## immediately followed by a newline is always silently removed by
## source_skip()/source_peekn() before any quoting-mode code ever sees
## the raw bytes (line-continuation, same as outside quotes) -- but
## parse_dquoted.c only recognized a backslash as *itself* removable
## when it was escaping one of $, `, ", \; for any other following
## character (including a newline that had, by the time it asked, already
## vanished) it fell back to keeping the backslash as a literal
## character. So a backslash-newline inside a double-quoted string kept
## the backslash while still losing the newline, corrupting the string
## instead of joining the two lines with nothing in between. Found via
## autoconf's generated `configure` (gettext-tools), whose generated
## as_suggested shell-compatibility probe uses exactly this construct
## and, when it silently mis-evaluated under shish, made configure
## conclude the running shell wasn't good enough and go hunting for
## (and re-exec into) another one.
F93=$(mktemp)
printf 'X="ab\\\ncd"\necho "$X"\n' >"$F93"
X93=$(. "$F93")
rm -f "$F93"
assert_equal "abcd" "$X93" "a backslash-newline inside a double-quoted string must be removed entirely, not leave a stray backslash behind"

## fixes/94 (mmap-read-empty-file-fails): mmap_read() (lib/mmap/mmap_read.c)
## treated a genuinely empty (0-byte) file the same as a real open()
## failure -- lseek() returning 0 hit an early "return 0" (failure)
## before ever reaching the "else map = \"\";" branch below it that was
## clearly meant to handle exactly this case. builtin_cat (and anything
## else going through buffer_mmapread()) then reported whatever errno
## happened to be lying around from some earlier, unrelated syscall
## (e.g. a stale ECHILD from job control reaping a background
## pipeline) as if it were a real error reading the file. Found via
## gettext's generated configure, whose libtool boilerplate routinely
## `cat`s a compiler-warnings file that's legitimately empty when the
## compiler produced no warnings.
F94=$(mktemp)
X94=$(cat "$F94" 2>&1)
STATUS94=$?
rm -f "$F94"
assert_equal "0" "$STATUS94" "cat on a genuinely empty file must succeed, not report a spurious error"
assert_equal "" "$X94" "cat on a genuinely empty file must produce no output and no error message"

## fixes/95 (forked-child-stale-efunction-crash): sh_forked() (the
## per-process cleanup run right after fork(), in src/sh/sh_forked.c)
## flattens the whole struct env chain down to a single sh_root, but
## copied sh->eval verbatim from whatever was active at fork time --
## if that was a shell function call (exec_command.c's H_FUNCTION case
## sets sh->eval = &e with E_FUNCTION), the copy kept E_FUNCTION set on
## the now-parentless sh_root. sh_exit()'s "unwind past every
## E_FUNCTION frame to find the real root" loop then walked sh->parent
## (NULL, since sh_root has no parent) straight into a NULL
## dereference. Any pipeline whose builtin stage is forked from inside
## a function via a command substitution segfaulted as soon as that
## builtin finished -- confirmed while investigating excessive/crashing
## forks running gettext-tools' generated configure under shish.
F95script='f() { x=$(echo hi | cat); echo "$x"; }
f'
F95=$(mktemp)
printf '%s\n' "$F95script" >"$F95"
X95=$(. "$F95" 2>/tmp/fixed95.err)
STDERR95=$(cat /tmp/fixed95.err)
rm -f "$F95" /tmp/fixed95.err
assert_equal "hi" "$X95" "a pipeline's builtin stage, forked from inside a function via a command substitution, must still produce the right output"
assert_equal "" "$STDERR95" "the same construct must not crash the forked builtin's own process on exit"

## fixes/96 (heredoc-eof-no-trailing-newline): a here-document whose
## closing delimiter is the very last thing in the source, with no
## newline after it, failed to parse at all -- parse_dquoted.c/
## parse_squoted.c's per-character read loop only recognized "end of
## this line" on an actual '\n' byte, so hitting end-of-input first
## returned a hard error instead. redir_source.c's error path then
## left the redirection's word as the original, unexpanded delimiter
## node, so the here-doc's "content" silently became the literal
## delimiter text ("EOF") instead of the real body. A `-c` command
## string is the routine way to hit this (unlike a file, it has no
## implicit trailing newline of its own), but a sourced file missing
## its own final newline hits the identical code path.
F96=$(mktemp)
printf 'cat <<EOF\nHEREDOCCONTENT\nEOF' >"$F96"
X96=$(. "$F96")
rm -f "$F96"
assert_equal "HEREDOCCONTENT" "$X96" "a here-doc whose closing delimiter is the last thing in the source, with no trailing newline, must still work"

## fixes/97 (eval-return-frame-skip-leak): eval_return() searched only
## for the nearest E_FUNCTION eval frame, so a "return" inside a
## subshell (E_ROOT, not E_FUNCTION -- eval_subshell.c) longjmped
## straight past the subshell's own boundary into the *enclosing*
## function, instead of just ending the subshell like "exit" would.
## Its fdstack/varstack unwind code (mirroring eval_jump()'s for
## break/continue) was also present but commented out, so every frame
## skipped this way leaked its env/vartab/fdstack state permanently --
## confirmed via RSS growing linearly without bound under a tight loop
## of the minimal case. Fixed by (a) searching for the nearest
## E_FUNCTION *or* E_ROOT frame, matching eval_exit()'s existing
## E_ROOT search for the analogous "exit inside a subshell" case, and
## (b) uncommenting/fixing the unwind to match eval_jump().
X97=$(f() { ( return 5 ); echo "after: $?"; }; f; echo "f returned: $?")
assert_equal "$(printf 'after: 5\nf returned: 0')" "$X97" "return inside a subshell must only end the subshell, not propagate out through the enclosing function"

## fixes/98 (unpaired-bracket-triggers-real-glob-every-time):
## parse_unquoted.c set S_GLOB on a word the instant it saw any
## unquoted character in "* ? [ ] \", with no check for whether those
## characters actually formed a syntactically plausible pattern -- so
## a lone "[" (POSIX test/"[ ]" syntax, the single most common token
## in any real script) sent every word through a real glob(3) call,
## which reads the current directory (getdents64) and stat()s
## candidates before giving up, every single time. Fixed by only
## setting S_GLOB for "[" once a later "]" in the same word actually
## completes a bracket expression.
##
## Correctness is covered above and elsewhere (glob patterns still
## expand correctly); this is specifically a performance regression
## guard, timing-based since the bug produced correct output, just
## catastrophically slower. 50000 iterations of a bare "[ ]" loop
## condition: ~0.02s fixed, ~3.6s with the bug reintroduced (measured
## via `git stash` against the pre-fix tree) -- generous margin below
## to tolerate a slow/loaded CI machine while still failing hard if
## the real glob(3) call comes back.
T98_0=$(date +%s)
i=0
while [ $i -lt 50000 ]; do i=$((i + 1)); done
T98_1=$(date +%s)
ELAPSED98=$((T98_1 - T98_0))
assert_less "$ELAPSED98" "5" "50000 iterations of a bare '[ ]' loop condition must not trigger a real glob(3) call per iteration"

## fixes/99 (eval-jump-frame-skip-leak): eval_jump() (break/continue)
## had the exact same class of bug as eval_return.c's (fixes/97):
## searching only for the nearest E_LOOP eval frame let a subshell
## (E_ROOT) or function call (E_FUNCTION) boundary be skipped right
## over instead of blocking the search, so "break"/"continue" inside
## a subshell or function escaped all the way out to whatever loop
## happened to enclose *that*, instead of erroring like bash does
## ("break: only meaningful in a `for'/`while'/`until' loop") -- and,
## since the longjmp bypassed every skipped frame's own cleanup,
## leaked its env/vartab/fdstack state permanently, same as fixes/97.
## This one is what was actually crashing real, long-running scripts:
## a leaked struct env is stack-allocated, so once its own C stack
## frame is reused by later calls, sh_forked()'s later walk over
## sh->parent reads whatever now occupies that memory and corrupts
## the heap (or segfaults outright) forking the next external command.
X99=$(for i in 1 2 3; do ( break ); echo "iter $i"; done; echo done)
assert_equal "$(printf 'iter 1\niter 2\niter 3\ndone')" "$X99" "break inside a subshell must not escape to a loop outside it"

X99B=$(f() { break; }; for i in 1 2 3; do f; echo "iter $i"; done; echo done)
assert_equal "$(printf 'iter 1\niter 2\niter 3\ndone')" "$X99B" "break inside a function must not escape to a loop the function isn't lexically inside"

X99C=$(for i in 1 2 3; do for j in a b c; do if [ $j = b ]; then break 2; fi; echo "i=$i j=$j"; done; done; echo after)
assert_equal "$(printf 'i=1 j=a\nafter')" "$X99C" "break N must still cross ordinary nested loops with no function/subshell boundary in between"

## fixes/100 (umask-not-restored-after-subshell): a subshell's own
## "umask NNN" correctly updated sh->umask for the subshell's own
## struct env (so "$(umask)" read back inside, and after popping back
## out, both reported the right values), but nothing ever called the
## real umask() syscall to restore the *process-wide* mask when the
## subshell exited -- only builtin_umask.c ever calls umask(), and
## only when it itself runs. Every file/directory actually created
## after such a subshell kept silently getting the subshell's more
## restrictive mode for the rest of the process's life. autoconf/
## gnulib's "(umask 077 && mkdir ...)" private-tmpdir idiom hits this
## constantly; found while investigating a real "conftest.c: Permission
## denied" / "C compiler cannot create executables" failure running
## gettext-tools' configure.
F100=$(mktemp -d)
cd "$F100"
touch control.txt
(umask 077)
touch after.txt
CONTROL100=$(ls -la control.txt | cut -c1-10)
AFTER100=$(ls -la after.txt | cut -c1-10)
cd - >/dev/null
rm -rf "$F100"
assert_equal "$CONTROL100" "$AFTER100" "a subshell's umask change must not leak into files created after the subshell exits, regardless of this machine's ambient umask"

## fixes/101 (eval-exit-frame-skip-leak-and-bogus-early-return):
## eval_exit() ("exit" inside a function/subshell) had two bugs:
##
## 1. Like eval_return.c/eval_jump.c (fixes/97, fixes/99), its
##    fdstack/varstack/source unwind was commented out, and unlike
##    either of those, "exit" specifically walks *past* any number of
##    E_FUNCTION frames on its way to the nearest E_ROOT frame (a
##    subshell or the top-level script) -- so it also has to pop the
##    struct env exec_command.c's H_FUNCTION case pushed for each
##    skipped function call (sh_push(&inst) at H_FUNCTION), or that
##    env dangles the moment its stack slot is reused. Confirmed via a
##    real crash: looping "(f() { exit 5; }; f)" completed all
##    iterations but then segfaulted in sh_exit()'s final
##    "while(s->eval...) s = s->parent" walk, with corrupted register
##    state proving sh->eval was garbage by then.
##
## 2. A separate, pre-existing "if(e == sh->parent->eval) return"
##    early-out (meant to stop the search from crossing into a forked
##    child's stale, inherited eval chain -- already redundant, since
##    sh_forked() separately clears ev->jump on every inherited frame)
##    fired incorrectly any time "exit" was called two or more function
##    calls deep in the *same* process: exec_command.c's H_FUNCTION
##    case always does "sh->eval = &e" (its own local frame) on entry,
##    so sh->parent->eval is simply the immediately-enclosing function
##    call's own eval frame -- exactly the first frame the search
##    walks onto -- making "exit" silently return without ever
##    reaching the subshell/root, i.e. a silent no-op.
X101=$(g() { exit 7; }; f() { g; echo unreachable; }; (f); echo "after: $?")
assert_equal "after: 7" "$X101" "exit inside a function called from another function inside a subshell must terminate the subshell, not silently no-op"

i=0
while [ $i -lt 2000 ]; do
  (f() { exit 5; }; f) >/dev/null
  i=$((i + 1))
done
echo ok >/dev/null
assert_equal "0" "$?" "2000 iterations of exit-inside-a-function-inside-a-subshell must not crash or hang"

## fixes/102 (eval-function-redefinition-corrupts-ast-on-reexecution):
## eval_function() (running a "name() { ... }" definition) stole
## (moved, then NULLed) the name/body pointers straight out of the
## defining AST node and into a freshly allocated "functions" list
## entry. That only works if the node is evaluated exactly once before
## its enclosing top-level statement is freed (sh_loop.c frees each
## top-level statement right after running it) -- it breaks the moment
## the very same node is evaluated again, e.g. a function defined
## inside a loop body, or inside a shish "(...)" subshell (which runs
## in-process, not forked, so exec_functions_save/restore deliberately
## discards subshell-installed definitions every time the subshell
## scope ends, meaning the definition must be reinstallable on every
## visit). A second visit found name/body already NULLed from the
## first and dereferenced the NULL name -> segfault. Fixed by giving
## every installed definition its own independent copy (a new
## tree_copy() helper) instead of stealing the AST's own pointers, so
## the defining node can be evaluated any number of times.
X102=$(i=0; while [ $i -lt 5 ]; do f() { echo "call $i"; }; i=$((i + 1)); done; f)
assert_equal "call 5" "$X102" "a function defined inside a loop must not crash on the loop's second+ visit"

X102B=$(i=0; while [ $i -lt 500 ]; do (g() { :; }; g); i=$((i + 1)); done; echo "$i")
assert_equal "500" "$X102B" "a function defined and called inside an in-process subshell, repeated many times, must not crash"

## fixes/103 (external-commands-fragmented-into-orphan-process-groups):
## exec_program.c/job_fork.c unconditionally setpgid()'d every external
## command (single or pipeline member, foreground or background) into
## its own separate process group, regardless of whether job control
## was actually active (interactive, "set -m") -- real bash never does
## this for a non-interactive script (confirmed directly: every child,
## piped or backgrounded, stays in bash's own pgid). Since nothing then
## ever moves the *terminal's* actual foreground process group to
## match (that only happens for a genuinely interactive session), a
## non-interactive shish's children ended up in a process group the
## controlling terminal never designated as foreground -- so pressing
## Ctrl-C at the terminal only ever delivered SIGINT to shish's own
## process group, never to whatever external command/pipeline was
## currently running. shish itself would die, but the command it had
## just started (e.g. "gcc" mid-compile during a real ./configure run)
## was left running, orphaned, completely unaffected -- looking like
## "Ctrl-C doesn't work", or needing many presses to eventually land at
## a moment nothing was running. Confirmed via a real repro: send
## SIGINT to a non-interactive shish mid-"sleep 5", or mid-"gcc" during
## gettext-tools' actual configure -- the external process kept running
## after shish itself was gone. Fixed by gating every setpgid()/
## tcsetpgrp() call behind sh->opts.monitor, and by making job_wait()
## fall back to waiting for any child (not just the pipeline's first
## member) when a job has no real process group of its own to wait on.
##
## These checks can't reproduce the SIGINT delivery/orphaning itself
## (that needs a real controlling terminal and signal delivery, not
## just process substitution) -- they check the underlying, directly
## testable condition that causes it: a non-interactive shish's
## external children (single command, pipeline member, and background
## job alike) must share its own process group, not get a separate one.
X103_SHISH_PGID=$(ps -o pgid= -p $$ | tr -d ' ')
X103_FG_PGID=$(/bin/sh -c 'ps -o pgid= -p $$' | tr -d ' ')
assert_equal "$X103_SHISH_PGID" "$X103_FG_PGID" "a foreground external command run by a non-interactive script must share the script's own process group, not get a separate one"

X103_PIPE_PGID=$(/bin/sh -c 'ps -o pgid= -p $$' | tr -d ' \n')
assert_equal "$X103_SHISH_PGID" "$X103_PIPE_PGID" "a pipeline member run by a non-interactive script must share the script's own process group, not get a separate one"

sleep 0.3 &
X103_BGPID=$!
X103_BG_PGID=$(ps -o pgid= -p "$X103_BGPID" | tr -d ' ')
wait
assert_equal "$X103_SHISH_PGID" "$X103_BG_PGID" "a backgrounded external command run by a non-interactive script must share the script's own process group, not get a separate one"

## fixes/104 (job-terminal-never-initialized): job_init() decided
## whether to enable terminal handoff (job_terminal, used by every
## tcsetpgrp() call) by checking fd_err->mode & FD_TERM -- but that
## flag is only ever set by term_init(), which sh_main.c always calls
## *after* job_init() (via sh_init()). job_terminal was therefore
## always -1, for every session, interactive or not. Fixed by moving
## the job_terminal/job_pgrp setup into a new job_terminal_init(),
## called only once term_init() has actually run. The interactive
## terminal-handoff behavior this restores can't be exercised from a
## non-interactive tests/fixed.sh run (no controlling terminal to hand
## off in the first place) -- what's directly testable here is the
## companion bug found investigating it: job_clean()'s "[N]+ Done ..."
## banner (job_update(), called every sh_loop() iteration) printed
## unconditionally instead of being gated on sh->opts.monitor like
## job_wait()'s own equivalent banner already was, so a job reaped
## asynchronously by the SIGCHLD handler before job_wait() got to it
## could leak a stray "Done" line into a non-interactive script's
## stderr. Run enough quick background jobs in a row to make that race
## likely, then check stderr for any such line.
X104_ERR=$(i=0; while [ $i -lt 30 ]; do : & i=$((i + 1)); done; wait 2>&1 1>/dev/null)
assert_nomatch "$X104_ERR" "Done" "a non-interactive script must never print a job-done banner, even when a job is reaped asynchronously by the SIGCHLD handler ahead of job_wait()"

## fixes/105 (trap-handler-runs-unsafely-in-signal-context): a real-
## signal trap ("trap CMD INT/TERM/...") used to run its whole body,
## including allocation-heavy eval_tree() and potentially "exit",
## directly from inside the raw OS signal handler -- the same class of
## bug already fixed once for SIGCHLD's own handler (fixes/87) but
## never applied to user traps. Redesigned the same way: a minimal,
## async-signal-safe relay (trap_relay()) just records that a signal
## fired; the actual dispatch (trap_run_pending()) happens from
## ordinary context -- sh_loop()'s main loop, term_read()'s select()
## wakeup, and job_wait()'s own retry loop, so it also fires promptly
## while blocked on a long-running external command (confirmed against
## the real gettext-tools configure script: a real SIGINT sent while
## blocked on an actual "gcc" invocation, matching autoconf's own
## standard "trap ... INT" cleanup boilerplate, now reliably kills
## both gcc and shish itself with no leftover processes, instead of
## shish surviving and continuing to the next command).
##
## This redesign surfaced (and fixes) a real, concrete bug: a trap
## signal can be delivered (trap_relay() sets its pending flag) before
## trap_run_pending() gets a chance to drain it -- e.g. while still
## inside a subshell that's now exiting and, as part of its own normal
## cleanup, uninstalling the very trap that signal was meant to fire
## (trap_snapshot_restore(), same as "trap - SIG"/re-trapping at the
## top level via trap_uninstall()). Left set, the next dispatch ran it
## against whatever's *now* installed for that signal -- if that's
## nothing, trap_handler()'s "no trap found" fallback called
## sh_exit(1), silently killing the whole shell for what looked like a
## completely untrapped signal. Both places that uninstall a real-
## signal trap now also discard any not-yet-dispatched pending
## occurrence of it.
X105B=$( (trap 'echo caught' TERM; kill -TERM $$); echo "outer: $?" )
assert_equal "outer: 0" "$X105B" "a signal trap uninstalled by its own subshell's exit before it's dispatched must not later kill the whole process as if it were untrapped"

## the trap's own output must land correctly (deferred dispatch runs
## from whatever context happened to trigger it, same fdstack as an
## ordinary command at that point -- see fixes/80's analogous concern
## for EXIT traps)
X105C=$( (trap 'echo caught-term' TERM; kill -TERM $$; sleep 0.2; echo after-term); echo "outer: $?" )
assert_equal "$(printf 'caught-term\nafter-term\nouter: 0')" "$X105C" "a signal trap's own output must not get lost when its dispatch is deferred to a later, safe point"

## The other half of fixes/105 -- an exit triggered by a real-signal
## trap must propagate through every enclosing in-process subshell and
## actually terminate the whole shish process, not stop at the first
## subshell boundary it happens to land in (sh_async_exit, sh.h /
## eval_subshell.c) -- can't be expressed as a same-process assertion
## here (the property under test *is* "the process running this test
## exits"), and there's no portable, non-fragile way from within this
## script to spawn+signal+observe a second shish instance without
## knowing this binary's own path ($0 is this test script, not the
## interpreter). Verified manually instead, extensively, against both
## a minimal repro and the real gettext-tools configure script (a real
## SIGINT while blocked inside "( eval "$ac_link" )", autoconf's own
## idiom -- confirmed shish now reliably terminates, with gcc, across
## repeated trials at different timings, both via a direct pty-based
## harness and the real controlling-terminal Ctrl-C path).

## fixes/106 (ln-trailing-slash-on-plain-destination): builtin_ln()
## unconditionally appended a "/" to the destination operand before
## ever checking whether it names an existing directory, so
## "ln -s target name" (name not an existing directory, the common
## case) always ended up calling symlink(target, "name/"), which
## fails with ENOTDIR/ENOENT since a trailing-slash path requires the
## component before it to already be a directory. Only appended it
## when dst is actually a directory to link *into* now, and rejects
## (rather than silently mis-linking) more than one source without an
## existing directory destination.
F106=$(mktemp -d)
D106=$(mktemp -d)
(
  cd "$F106"
  ln -s /etc/hostname mylink
) >/dev/null 2>&1
X106=$(readlink "$F106/mylink" 2>/dev/null)
assert_equal "/etc/hostname" "$X106" "ln -s target name (name not an existing directory) must create the symlink, not fail with ENOTDIR"

X106B=$(ln -s a.txt b.txt "$D106/notadir-child" >/dev/null 2>&1; echo "status:$?")
assert_equal "status:1" "$X106B" "ln with more than one source and a destination that isn't an existing directory must fail cleanly, not silently link only the last source"
rm -rf "$F106" "$D106"

## fixes/107 (set-errexit-not-enforced): "set -e"'s errexit bit was
## correctly set/reflected in "$-" but nothing ever actually checked
## it -- a failing command never aborted the script. Enforced now in
## the two places a sequential list of commands is actually walked
## (eval_tree.c, for a compound body/loop body/etc.; eval_cmdlist.c,
## for ";"/newline-separated commands sharing one N_LIST node), each
## calling sh_exit() the same way an explicit "exit" would (so it
## correctly unwinds just one subshell/function level, or the whole
## process at the top level) -- with POSIX's specific exemptions: the
## controlling list of if/while/until, a "!"-negated command, and any
## AND-OR list member other than the one that actually determined its
## overall result (which requires *not* re-checking an AND-OR node's
## own return value at the outer sequential-list level at all, only
## trusting eval_and_or()'s own inner check on its right operand --
## see eval_tree.c's comment for why re-checking there breaks
## short-circuited cases like "false && true").
X107=$(set -e; false; echo bad)
assert_equal "" "$X107" "set -e must abort the script right after a failing command, not let it continue"

X107B=$(set -e; if false; then echo bad; else echo ok; fi; echo after)
assert_equal "$(printf 'ok\nafter')" "$X107B" "set -e must not fire for the controlling list of an if/while/until"

X107C=$(set -e; f() { false; echo bad; }; f; echo after)
assert_equal "" "$X107C" "set -e must fire for a failing command inside a function body, not just at the top level"

X107D=$(set -e; false && true; echo after)
assert_equal "after" "$X107D" "set -e must not fire when an AND-OR list short-circuits on its non-last (exempt) operand, even though the list's own inherited result is nonzero"

X107E=$(set -e; true && false; echo after)
assert_equal "" "$X107E" "set -e must fire when an AND-OR list's actually-evaluated last operand fails"

X107F=$(set -e; ! true; echo after)
assert_equal "after" "$X107F" "set -e must not fire for a \"!\"-negated command, regardless of its negated result"

## The AND-OR/compound-construct exemption rules above only look one
## node deep unless the exemption itself propagates through nested
## calls (function bodies, subshells) too -- confirmed against real
## bash's own actual behavior (not just POSIX's text) and against
## tests/posix/errexit-p.tst (yash's own errexit conformance suite,
## 53 cases -- went from 7/53 to 49/53 passing after these fixes, the
## remaining 4 being unrelated, separately-filed bugs: BUGS:
## redirect-failure-does-not-block-execution-or-set-status and
## grouping-piped-loses-output-after-internal-failure).
X107G=$(set -e; false || false || true; echo after)
assert_equal "after" "$X107G" "set -e must not fire on a 3+-operand OR chain's un-exempted middle operand -- only the chain's true last operand matters"

X107H=$(set -e; f() { false; echo unreached; }; f && true; echo after)
assert_equal "$(printf 'unreached\nafter')" "$X107H" "set -e's exemption for an AND-OR list's non-last operand must survive into a function call reached while evaluating it"

X107I=$(set -e; { false && true; }; echo reached)
assert_equal "reached" "$X107I" "an AND-OR list's own exemption must survive being wrapped in a grouping -- confirmed against real bash"

## "set -e" firing inside a command substitution correctly terminates
## that substitution itself (matching sh_exit()'s usual "kill the
## innermost subshell/function" behavior -- a command substitution is
## one, see fixes/109), so the outer command substitution's own capture
## ends up empty: nothing after the "{ false; }"/"( false && true )"
## ever runs to produce output. Checking the *outer* shell's own exit
## status (from directly running these, not via a substitution) is
## what actually distinguishes "did -e fire in there or not".
X107J=$(set -e; { false; }; echo not_reached)
X107J_STATUS=$?
assert_equal "" "$X107J" "a grouping must not shield a genuinely non-exempt failure inside it, only an already-exempt one"
assert_greater "$X107J_STATUS" 0 "and that failure must be reported as a nonzero exit status"

X107K=$(set -e; ( false && true ); echo not_reached)
X107K_STATUS=$?
assert_equal "" "$X107K" "unlike a grouping, a subshell's own returned status is opaque/independent and must still be checked normally at the outer level, even if the failure inside it was itself exempt"
assert_greater "$X107K_STATUS" 0 "and that failure must be reported as a nonzero exit status"

X107L=$(set -e; if true; then false && true; fi; echo reached)
assert_equal "reached" "$X107L" "an AND-OR list's own exemption must survive being the last statement of an if-body"

X107M=$(set -e; for i in 1 2; do echo "a$i"; false && true; echo "b$i"; done; echo reached)
assert_equal "$(printf 'a1\nb1\na2\nb2\nreached')" "$X107M" "an AND-OR list's own exemption must survive being inside a for-loop body"

## fixes/108 (squoted-backslash-newline-swallowed): source_skip()/
## source_peekn() treated a backslash immediately followed by a
## newline as a line continuation and silently removed both bytes,
## unconditionally -- correct outside quotes and inside double quotes
## (fixes/93), but POSIX requires single quotes to preserve every
## character literally, no exceptions. A new source_squoted flag
## (source.h), set by parse_squoted.c around its own read loop, is how
## these primitives -- which sit below the parser, with no access to
## its quoting state -- know to skip the continuation-removal step.
X108_SCRIPT=$(mktemp)
printf "%s\n" "X='a\\" "b'" > "$X108_SCRIPT"
. "$X108_SCRIPT"
rm -f "$X108_SCRIPT"
X108=$(printf '%s' "$X" | od -An -c | tr -s ' ')
assert_equal " a \\ \n b" "$X108" "a backslash-newline inside single quotes must be preserved literally, not silently removed"

## the same primitives are shared by a heredoc with a quoted delimiter
## (parse_squoted.c's P_HERE path) -- its body must be equally literal
X108B=$(cat <<'EOF'
a\
b
EOF
)
assert_equal "$(printf 'a\\\nb')" "$X108B" "a heredoc with a quoted delimiter must preserve a backslash-newline in its body literally too, matching bash"

## fixes/109 (cmdsubst-does-not-isolate-shell-state): found while
## writing fixes/107's own regression tests above, all "$(set -e;
## ...)" style -- expand_command.c (command substitution, "$(...)"/
## backquotes) never called sh_push(), unlike eval_subshell.c's
## "(...)" -- even though POSIX defines command substitution as a
## subshell (2.6.3) too. "set -e"/any other "set" option, "cd",
## "umask", etc. run inside "$(...)" permanently changed the *calling*
## shell's own state once the substitution finished, instead of only
## affecting the substitution's own, discarded-afterward environment.
X109=$(x=$(set -e; true); echo "leaked:$-")
assert_nomatch "$X109" "leaked:.*e" "\"set -e\" run inside a command substitution must not leak into the calling shell once it's done"

BEFORE109=$(pwd)
X109B=$(cd /; pwd)
AFTER109=$(pwd)
assert_equal "/" "$X109B" "a cd done inside a command substitution must still apply to that substitution's own execution"
assert_equal "$BEFORE109" "$AFTER109" "a cd done inside a command substitution must not change the calling shell's own cwd once it's done"

## fixes/110 (test-ne-misdetected-as-nt): found triaging
## tests/posix/errexit-p.tst against fixes/107 above -- "-ne" (numeric
## not-equal) and "-nt" (file mtime, newer-than) both have "n" as
## their second character, and builtin_test.c's binary-operator
## dispatch only checked that second character to decide "this is a
## file-mtime comparison", so "test 1 -ne 2" silently ran as
## filetime("1") > filetime("2") between two (usually nonexistent)
## files named "1" and "2" instead of the numeric comparison it's
## supposed to be. Now also checks the third character ("t" vs "e"),
## which is what actually distinguishes them.
assert_equal "0" "$(test 1 -ne 2; echo $?)" "test 1 -ne 2 must be a numeric not-equal comparison (true), not a file mtime one"
assert_equal "1" "$(test 1 -eq 2; echo $?)" "test 1 -eq 2 must still correctly report false"

F110=$(mktemp -d)
touch "$F110/old"
sleep 1.1
touch "$F110/new"
assert_equal "0" "$(test "$F110/new" -nt "$F110/old"; echo $?)" "test -nt must still correctly compare file mtimes, unaffected by the -ne/-nt disambiguation fix"
assert_equal "0" "$(test "$F110/old" -ot "$F110/new"; echo $?)" "test -ot must still correctly compare file mtimes too"
rm -rf "$F110"

## fixes/111 (grouping-piped-loses-output-after-internal-failure):
## eval_pipeline.c forks each pipeline stage and tells the *last*
## command in it to exec() directly instead of returning (E_EXIT,
## eval_tree.c's own "exec the tail command instead of forking"
## optimization) by setting it on the shared e->flags before
## dispatching that stage's whole node. eval_tree()'s own per-node loop
## correctly restricts E_EXIT to just the last node of a list it's
## walking -- but a "{ ...; }" grouping (or a bare ";"-separated N_LIST)
## used as a pipeline stage dispatches straight to eval_cmdlist()
## instead, which never touched e->flags's E_EXIT bit at all, so it
## stayed set (inherited from the pipeline fork) for *every* member of
## the group's body, not just its last one -- eval_simple_command.c
## reads that bit directly to decide whether to exec() a command in
## place. The group's first member got treated as the tail call: it
## ran, then the forked pipeline stage exited immediately, silently
## losing everything after it. Confirmed independent of any failure
## inside the group (reproduced identically with "true" in place of
## "false") -- eval_cmdlist() now scopes E_EXIT to its own last member,
## matching eval_tree().
X111=$({ echo reached1; false; echo reached2; } | cat)
assert_equal "$(printf 'reached1\nreached2')" "$X111" "a grouping's own later commands must still run when the whole grouping is piped into another command"

X111B=$({ echo a; echo b; echo c; } | cat)
assert_equal "$(printf 'a\nb\nc')" "$X111B" "same as above, with no failing command inside the grouping at all -- this was never really about the failure"

## fixes/112 (redirect-failure-does-not-block-execution-or-set-status):
## a simple command whose own redirection fails (target file doesn't
## exist, etc.) still ran, using whatever fd it had before, and still
## reported exit status 0 -- POSIX requires the command not execute at
## all and the shell to treat it as a failure. Two separate gaps, both
## fixed: (1) exec_command.c resolves a builtin's pending fd 0/1/2
## redirection right before running it (the real open() is deferred
## that far), but never checked whether that resolution actually
## succeeded, so it ran the builtin regardless; (2) a redirection with
## no command at all ("<_no_such_file_" alone) never got resolved
## *at all* -- nothing forces that beyond exec_command.c, which this
## case never reaches -- so eval_simple_command.c now forces immediate
## (not the usual lazy) resolution specifically when there's no
## command to hand the pending fd off to.
X112=$(echo not_printed <_no_such_file_ 2>/dev/null; echo "status:$?")
assert_equal "status:1" "$X112" "a command whose own redirection fails must not run at all, and must report a nonzero exit status"

X112B=$(<_no_such_file_ 2>/dev/null; echo "status:$?")
assert_equal "status:1" "$X112B" "a bare redirection with no command at all must still be attempted and its failure reported"

F112=$(mktemp -d)
X112C=$(echo printed > "$F112/out"; cat "$F112/out"; echo "status:$?")
assert_equal "$(printf 'printed\nstatus:0')" "$X112C" "an ordinary, successful redirection on a real command must still work"
rm -rf "$F112"

## fixes/113 (random-seed-unset-nonseed-ignored, found while chasing down
## the yash-random-y-tst-hangs BUGS entry -- the hang itself no longer
## reproduces as of fixes/111/112, but running the test file for real
## once it stopped hanging turned up three genuine, separate $RANDOM
## bugs): expand_param.c's $RANDOM special-case always called
## uint32_random() directly on every read, completely ignoring any
## assignment to RANDOM -- "RANDOM=123" never seeded anything, a
## non-integer assignment (including empty) never turned the magic off
## the way bash/yash require, and "unset RANDOM" never permanently
## disabled it either. Fixed by adding var_random_active/_assign/_unset/
## _next (src/var/var_random.c) and hooking them into the actual
## assignment (var_setsa.c) and unset (var_unset.c) paths.
X113A=$(RANDOM=42; echo $RANDOM $RANDOM $RANDOM)
X113B=$(RANDOM=42; echo $RANDOM $RANDOM $RANDOM)
assert_equal "$X113A" "$X113B" "seeding RANDOM with the same integer twice must produce the same sequence both times"

X113C=$( (RANDOM=; echo [$RANDOM]); (RANDOM=X; echo [$RANDOM]) )
assert_equal "$(printf '[]\n[X]')" "$X113C" "assigning a non-integer (including empty) to RANDOM must turn off its magic and leave it holding that literal value"

X113D=$(unset RANDOM; echo ${RANDOM-unset}; RANDOM=123; echo $RANDOM $RANDOM $RANDOM)
assert_equal "$(printf 'unset\n123 123 123')" "$X113D" "unset RANDOM must permanently disable its magic -- it stays an ordinary variable even after being reassigned"

## fixes/114 (nounset-crashes-and-over-fires): "set -u" referencing a
## bare, truly-unset ${parameter}/$parameter segfaulted every time --
## expand_param.c's fatal path freed the argument-list node it was
## building the substitution into (tree_free(n)), but that node was
## already linked into the caller's own argument list
## (expand_args.c's "n->next = tree_newnode(...)"); freeing it without
## unlinking left that ->next dangling, and the next simple command's
## tree_count() crashed walking into freed memory. There was also a
## leftover debug "vartab_dump()" call left in the same path (dumping
## the whole variable table to stderr on every unbound-variable error)
## and a second, separate bug: the nounset check fired even for
## "${parameter:-word}"/":="/":?"/ ":+"" forms, which POSIX explicitly
## exempts since each already defines its own behavior for an unset
## parameter -- only a bare ${parameter}/$parameter with no operator
## at all is what nounset is meant to guard.
X114A=$(set -u; echo $UNSET_VAR_114A; echo not_reached 2>/dev/null)
X114A_STATUS=$?
assert_equal "" "$X114A" "a bare unset variable reference under set -u must not run any later command in the same (non-interactive) shell"
assert_equal "1" "$X114A_STATUS" "...and must exit with a plain nonzero status, not crash"

X114B=$(set -u; echo ${UNSET_VAR_114B-default})
assert_equal "default" "$X114B" "\${parameter-word} on an unset parameter must not trigger nounset at all"

X114C=$(set -u; echo ${UNSET_VAR_114C:-default})
assert_equal "default" "$X114C" "\${parameter:-word} on an unset parameter must not trigger nounset at all"

X114D=$(set -u; : ${UNSET_VAR_114D:=assigned}; echo $UNSET_VAR_114D)
assert_equal "assigned" "$X114D" "\${parameter:=word} on an unset parameter must not trigger nounset, and must still assign the default"

X114E=$(set -u; echo [${UNSET_VAR_114E:+alt}])
assert_equal "[]" "$X114E" "\${parameter:+word} on an unset parameter must not trigger nounset"

## fixes/115 (set-noglob-and-hashall-not-enforced): both `struct
## shopt` bits were set correctly by set -f/+f and -h/+h and reflected
## correctly in $-, but nothing ever actually checked them.
## expand_glob.c now skips glob() entirely (falling back to the same
## "treat as literal" path already used for a real no-match) when
## noglob is on. exec_hash.c now bypasses its whole hash-cache lookup/
## creation and just re-searches PATH every time when hashall is off,
## which is the only way "off" is observably different from "on" (the
## default) at all.
F115=$(mktemp -d)
touch "$F115/a.c" "$F115/b.c"
X115A=$(cd "$F115" && set -f; echo *.c)
assert_equal "*.c" "$X115A" "set -f disables pathname expansion -- a glob pattern is left literal"
X115B=$(cd "$F115" && set -f; set +f; echo *.c)
assert_equal "a.c b.c" "$X115B" "set +f turns it back on"
rm -rf "$F115"

F115H1=$(mktemp -d)
F115H2=$(mktemp -d)
printf '#!/bin/sh\necho FIRST\n' >"$F115H1/f115cmd"
chmod +x "$F115H1/f115cmd"
printf '#!/bin/sh\necho SECOND\n' >"$F115H2/f115cmd"
chmod +x "$F115H2/f115cmd"
X115C=$(PATH="$F115H1:$F115H2"; f115cmd; rm -f "$F115H1/f115cmd"; f115cmd 2>/dev/null)
assert_equal "FIRST" "$X115C" "set +h is not the default -- a removed-but-cached command location is still used and fails"
printf '#!/bin/sh\necho FIRST\n' >"$F115H1/f115cmd"
chmod +x "$F115H1/f115cmd"
X115D=$(set +h; PATH="$F115H1:$F115H2"; f115cmd; rm -f "$F115H1/f115cmd"; f115cmd)
assert_equal "$(printf 'FIRST\nSECOND')" "$X115D" "set +h re-searches PATH every time instead of trusting a cached (now-stale) location"
rm -rf "$F115H1" "$F115H2"

## fixes/116 (add tilde and brace expansion): neither existed at all
## before this -- "~"/"~user" and "{a,b,c}" were both printed back
## literally regardless of any option. See src/expand/expand_tilde.c
## and src/expand/expand_brace.c; tests/expand-tilde.sh and
## tests/expand-brace.sh cover the features themselves in depth, this
## is just the specific regression that motivated a private
## tree_copy() in expand_args.c/expand_vars.c: both rewrite the
## argument/assignment word text or structure, and ncmd->args/vars is
## the *permanent* parsed command tree, reused on every execution of
## that command node -- rewriting it in place the first time through a
## loop would leave every later iteration seeing an already-expanded
## (or, worse, half-rewritten) tree instead of expanding fresh.
X116=""
for i in 1 2 3; do
  X116="$X116$(echo ~/x{1,2})."
done
assert_equal "$HOME/x1 $HOME/x2.$HOME/x1 $HOME/x2.$HOME/x1 $HOME/x2." "$X116" \
  "tilde and brace expansion both re-run fresh on every loop iteration, never mutating the parsed command tree"

## fixes/117 (set-allexport-unimplemented, plus a much more dangerous
## bug found while adding it): "set -a"/"+a" (export every assignment)
## was entirely missing, so this adds struct shopt's "allexport" bit
## and hooks it into var_setsa.c (every "name=word" assignment),
## mirroring how var_random.c's RANDOM= handling is wired into the
## same function.
##
## Adding that bit as struct shopt's *first* field revealed
## sh_root.c's default struct env was built with plain positional
## initializers dressed up with "/* .field = */" comments -- not real
## C designated initializers -- so inserting a field at the front
## silently shifted every single default one slot down: hashall's "1"
## landed on noglob (turning pathname-hashing off and glob-disabling
## on by default), braceexpand's "1" landed on xtrace (turning on a
## trace of every command by default). Nothing caught this at compile
## time; only actually running the shell and noticing "$-" now read
## "afx" *by default* (nothing had even run "set -x") gave it away.
## Fixed by converting sh_root.c's struct shopt initializer to real
## ".field = value" designated initializers, which can't silently
## misalign like this again regardless of future field reordering.
X117A=$(echo $-)
assert_equal "hB" "$X117A" "the shell's own default option flags must be exactly hashall+braceexpand, nothing else -- regression guard for the sh_root.c positional-initializer bug"

X117B=$(set -a; X117VAR=exported; sh -c 'echo $X117VAR')
assert_equal "exported" "$X117B" "set -a exports every subsequent assignment automatically"

X117C=$(X117VAR2=notexported; sh -c 'echo [$X117VAR2]')
assert_equal "[]" "$X117C" "without set -a, a plain assignment is not exported"

X117D=$(set -a; set +a; X117VAR3=notexported; sh -c 'echo [$X117VAR3]')
assert_equal "[]" "$X117D" "set +a turns allexport back off for later assignments"

## fixes/118 (set-noexec-unimplemented): "set -n"/"-n" (read and fully
## parse commands -- so a later syntax error is still caught -- but
## never execute any of them; ignored for interactive shells) was
## entirely missing. Hooked into sh_loop.c's main loop: skips the
## eval_tree() call for each top-level list (but not the parse_list()
## call just above it) whenever noexec is on and the shell isn't
## interactive. "." /"source" reuses this same loop (see
## builtin_source.c), so a noexec shell sourcing another file is
## covered for free.
F118=$(mktemp -d)
printf 'echo should-not-run\ntouch "%s/marker"\n' "$F118" >"$F118/script.sh"
X118A=$(set -n; . "$F118/script.sh")
assert_equal "" "$X118A" "a noexec shell sourcing a file runs nothing -- no output"
X118A_MARKER=$(test -e "$F118/marker"; echo $?)
assert_equal "1" "$X118A_MARKER" "...and no side effects (the touch never ran) either"
rm -rf "$F118"

## "set -n" still catching a later syntax error (sh_loop.c's noexec
## check only skips eval_tree(), never parse_list()) was confirmed
## manually ("shish -n bad.sh" reports the error and exits 1) rather
## than here: sourcing a file with a syntax error turned out to
## always kill the whole top-level process, even from inside a real
## subshell or $(...) -- completely unrelated to noexec (reproduces
## identically with an ordinary, unrelated command in the subshell
## too) but unsafe to trigger from inside this suite, since it would
## abort the rest of fixed.sh's own tests along with it. Logged
## separately as BUGS: source-syntax-error-kills-whole-process-not-
## just-subshell.

## "set -n" only takes effect from sh_loop.c's *next* top-level parse
## iteration onward -- it can't retroactively stop a list already
## mid-evaluation, so this needs "set -n" and "echo two" to be
## genuinely separate top-level commands (separate lines sourced from
## a file), not joined by ';' into one already-committed list.
F118B=$(mktemp -d)
printf 'echo one\nset -n\necho two\n' >"$F118B/script.sh"
X118C=$(. "$F118B/script.sh")
assert_equal "one" "$X118C" "set -n mid-script stops running any later top-level command"

## once noexec takes effect in a non-interactive script, nothing after
## it runs at all -- not even a later "set +n" itself, since that's
## just another command that never gets executed either. Confirmed
## this is real bash/dash behavior too, not just an implementation
## quirk: both print only "one" for this exact script.
printf 'echo one\nset -n\nset +n\necho three\n' >"$F118B/script2.sh"
X118D=$(. "$F118B/script2.sh")
assert_equal "one" "$X118D" "a later set +n in the same noexec script never runs either, so noexec sticks for the rest of it"
rm -rf "$F118B"

## fixes/119 (set-o-longopts-unimplemented): "-o name"/"+o name" (set/
## query an option by its POSIX/bash long name instead of a letter)
## and bare "set -o"/"set +o" (print every option's current state)
## didn't exist. Since struct shopt's members are 1-bit bitfields --
## which C doesn't allow taking the address of -- this is a
## name-to-letter table in builtin_set.c rather than one pointer per
## name, dispatched through the same set_apply()/set_get() helpers the
## ordinary letter options now also go through. "-o" has no colon in
## the optstring (its own argument is read by hand, one argv element
## at a time, not through shell_getopt_r) -- confirmed while writing
## this that shell_getopt_r() only advances past an option's whole
## argv element when the option takes an argument via ":", so
## opt.ind needed an explicit extra advance past "-o"/"+o" itself
## before reading the word after it.
X119A=$(set -o allexport; echo $-)
assert_equal "ahB" "$X119A" "set -o NAME turns an option on by its long name"

X119B=$(set -a; set +o allexport; echo $-)
assert_equal "hB" "$X119B" "set +o NAME turns it back off"

X119C=$(set -o | grep -c '^allexport ')
assert_equal "1" "$X119C" "bare set -o lists every option's current state, one per line"

X119D=$(set +o | grep -c '^set [-+]o allexport$')
assert_equal "1" "$X119D" "bare set +o lists every option as a reusable \"set -o\"/\"set +o\" line"

## "set -o badname" is a special-built-in utility error, which POSIX
## requires to kill a non-interactive shell outright (2.8.1,
## "Consequences of Shell Errors": "Special built-in utility error"
## -> shell shall exit); dash matches this exactly. That means the
## subshell below dies right at the "set" line -- no later command in
## it, including a trailing "echo $?", ever runs -- so the only way to
## observe the failure is the subshell's own exit status, not
## something it prints afterward.
X119E_STATUS=$( (set -o this_is_not_a_real_option >/dev/null 2>&1); echo $? )
assert_match "$X119E_STATUS" "[1-9]*" "set -o with an unknown name is an error, not silently ignored"

## fixes/120 (set-dashdash-with-no-operands-prints-everything,
## set-bare-dash-not-consumed): two small, unrelated builtin_set.c
## bugs found while surveying POSIX's missing set options.
##
## "set --" with no operands fell into the "print every variable and
## function" branch, since that was gated on "no real option was ever
## recognized" (got_opt) rather than "no arguments were given at all"
## -- "--" itself is consumed by shell_getopt_r() without ever being
## returned as a recognized option character, so it never set
## got_opt either. Now gated on "argc <= 1" (a truly bare "set")
## instead; "got_opt" itself is gone, nothing else used it.
X120A=$(set --; echo "$#")
assert_equal "0" "$X120A" "set -- with no operands must just clear the positional parameters, not print every variable"

X120B=$(set -a 2>&1)
assert_equal "" "$X120B" "a real option with no operands must still print nothing (not a regression from the -- fix above)"

## "set -" (a bare "-", intended to end option processing the same as
## "--" while also turning off -x) left the "-" itself as the new $1
## instead of being consumed. Fixed in builtin_set.c specifically (not
## the shared shell_getopt_r(), which other builtins -- "cat -" reads
## stdin -- rely on leaving a lone "-" alone as a literal operand).
X120C=$(set - a b c; echo "$1 $2 $3")
assert_equal "a b c" "$X120C" "a bare set - is consumed as an end-of-options marker, not left as \$1"

X120D=$(set -x; set -; echo $-)
assert_equal "hB" "$X120D" "set - also turns -x back off, per POSIX"

X120E=$(set -- - -- baz; bracket() { for a; do printf '[%s]' "$a"; done; }; bracket "$@")
assert_equal "[-][--][baz]" "$X120E" "a literal '-' appearing *after* an explicit -- must stay a plain operand, not be re-treated as the special bare-dash form"

## fixes/121 (source-syntax-error-kills-whole-process-not-just-subshell):
## found while adding regression tests for fixes/118 ("set -n"),
## unrelated to it. parse_error.c called a raw exit(1) directly for
## any syntax error while parsing a real (mmap'd) script file -- which
## a "."/"source" of one is -- completely bypassing sh_exit()'s
## subshell-aware unwind (the same longjmp-based mechanism eval_exit()
## uses for every other fatal error, and that sh_loop.c's own,
## now-unreachable-for-this-case, "if(!interactive) sh_exit(...)" call
## right after already relies on). Since shish's "(...)" subshells run
## in-process (setjmp/longjmp, not fork -- see eval_subshell.c), a raw
## exit(1) here always killed the whole process outright instead of
## unwinding back to the nearest enclosing subshell/$(...) frame.
F121=$(mktemp -d)
printf 'echo before\nif [ 1 = 1 ]\n' >"$F121/bad.sh"

X121A=$( (. "$F121/bad.sh") >/dev/null 2>&1; echo AFTER_SUBSHELL)
assert_equal "AFTER_SUBSHELL" "$X121A" "a syntax error sourced inside a real (...) subshell must not kill the rest of the script"

X121B_STATUS=$(X=$(. "$F121/bad.sh" 2>/dev/null); echo "$?")
assert_equal "1" "$X121B_STATUS" "...and \$(...) around it must still report the failure's own nonzero status"
rm -rf "$F121"

## fixes/122 (set-privileged-unimplemented, plus a scary global-state
## leak found while adding it): "-p"/"set -p" (privileged mode: don't
## process $ENV) was entirely missing, along with $ENV-sourcing itself
## (there was nothing yet for -p to suppress) and every "set"-letter
## option working identically as a shish command-line startup flag
## (only -c/-x/-e/-n existed there before).
##
## Wiring the new startup option loop through sh_main.c's *existing*
## shell_getopt() -- the process-global, non-reentrant one -- with a
## leading '+' in its optstring (needed for "+p" etc.) turned out to
## leak: nothing in this codebase ever resets shell_optind to 0
## between *unrelated* builtins' own shell_getopt() calls, so the
## global "+-" vs. "-"-only prefix set chosen once at shell startup
## silently persisted for the rest of the process, making
## "chmod +x file" (builtin_chmod.c's own, completely unrelated,
## shell_getopt() call) misparse "+x" as an invalid *option* instead
## of chmod's own mode-string operand. Fixed by giving sh_main.c's own
## parsing loop a *local* struct optstate + shell_getopt_r(), exactly
## matching how builtin_set.c's identical loop already avoided this
## same trap.
X122A=$(chmod +x "$0" 2>&1; echo done)
assert_match "$X122A" "*done" "chmod +x must still parse its own operand correctly (regression guard for the shell_getopt global-state leak)"

X122B=$(set -p; echo $-)
assert_equal "hpB" "$X122B" "set -p turns privileged mode on"

X122C=$(set -p; set +p; echo $-)
assert_equal "hB" "$X122C" "set +p turns it back off"

X122D=$(set -o | grep -c '^privileged ')
assert_equal "1" "$X122D" "privileged is exposed as an -o long-option name like every other letter option"

## the rest need a genuinely separate process invoked with real
## command-line flags/environment -- "readlink /proc/\$\$/exe" (not
## /proc/self/exe, which would resolve to the forked readlink itself)
## finds this running shish's own binary to re-invoke.
SHISH_SELF=$(readlink "/proc/$$/exe" 2>/dev/null)

if [ -n "$SHISH_SELF" ] && [ -x "$SHISH_SELF" ]; then
  X122E=$("$SHISH_SELF" -p -c 'echo $-')
  assert_equal "hpB" "$X122E" "-p on the command line turns privileged mode on at startup"

  X122F=$("$SHISH_SELF" -o noexec -c 'echo should-not-run')
  assert_equal "" "$X122F" "-o name works as a startup flag too, not just via the set builtin"

  X122G=$(printf 'echo "$1 $2"\n' | "$SHISH_SELF" -s foo bar)
  assert_equal "foo bar" "$X122G" "-s reads commands from stdin and leaves the remaining args as positional parameters"

  F122=$(mktemp -d)
  printf 'echo ENV_WAS_SOURCED\n' >"$F122/rc.sh"
  X122H=$(script -qec "ENV=\"$F122/rc.sh\" \"$SHISH_SELF\" -c 'exit' </dev/null" /dev/null 2>/dev/null)
  case $X122H in
    *ENV_WAS_SOURCED*) X122H_MATCHED=yes ;;
    *) X122H_MATCHED=no ;;
  esac
  assert_equal "no" "$X122H_MATCHED" "\$ENV must NOT be sourced for a non-interactive -c invocation"
  rm -rf "$F122"
fi

## fixes/123, fixes/124, fixes/125: found chasing a Termux (Android/
## bionic) segfault report in this repo's own "shish configure" by
## rebuilding with -fsanitize=address,undefined (see cfg-cmake.sh's
## cfg()) -- confirmed to reproduce identically under Linux/glibc, so
## none of the three are bionic-specific. All three are real UB
## (shift-by-type-width, address-of-a-member-through-a-null-pointer,
## memcpy() with a null source pointer even at length 0) that happen to
## be silent under a plain (non-sanitized) build on every libc/arch
## this project currently builds for -- x86/ARM shift instructions mask
## the count by the type width, and a 0-length memcpy is a no-op in
## practice -- so unlike this file's other entries, none of these three
## have a behavioral difference an assert_equal/assert_match here could
## actually distinguish pre- vs. post-fix. Per the "Writing a test"
## exception in CLAUDE.md for fixes that can't get a real regression
## assertion, these are instead verified by rebuilding with
## -fsanitize=address,undefined and re-running both this suite and this
## repo's own "shish configure" (which exercises var_rndhash on every
## variable-name hash, exec_search on every simple-command dispatch,
## and tree_copy's N_ARG/N_ASSIGN path via any "$var"/`eval`-driven
## re-evaluated node) -- clean of any UndefinedBehaviorSanitizer report
## at these three call sites, confirming the fix without a false-signal
## assertion here.
##
## fixes/123 (src/var/var_rndhash.c): ROL/ROR rotated by
## "VAR_BITS - c"; when the caller's rotate count c was 0 (a & VAR_MASK
## or b & VAR_MASK can legitimately be 0), that term shifted by
## VAR_BITS itself -- UB for a shift count equal to the operand's own
## width. Fixed by special-casing c == 0 to a no-op rotate.
##
## fixes/124 (src/exec/exec_search.c): the function-table search took
## "&functions->nfunc" to seed its walk before checking whether
## "functions" (the global list head) was NULL -- forming a member
## address through a null pointer, UB regardless of the member's
## offset. Fixed by seeding the walk with NULL directly when the list
## is empty, which is also the very common case (no user functions
## defined yet) hit by every simple-command dispatch until a script's
## first function definition.
##
## fixes/125 (src/tree/tree_copy.c): N_ARG/N_ASSIGN's stralloc_copy()
## ran unconditionally even when the source node's stralloc was never
## allocated (".s" still NULL, as it is for any argument word that
## carries no literal string, e.g. one that's pure parameter/command
## substitution) -- stralloc_copyb() then called byte_copy()/memcpy()
## with a null source pointer, UB even though the length is 0. The
## sibling N_ARGSTR case just below already guarded this the same way;
## this makes N_ARG/N_ASSIGN match it.
X123=$(f() { local x=abcdefghijklmnopqrstuvwxyz012345 y=zyxwvutsrqponmlkjihgfedcba543210; echo "$x$y"; }; f)
assert_equal "abcdefghijklmnopqrstuvwxyz012345zyxwvutsrqponmlkjihgfedcba543210" "$X123" "var_rndhash-exercising variable names must still hash/store/retrieve correctly (see comment above: real verification is the ASan/UBSan build, not this assertion)"

X124=$(f_never_called_but_defined() { echo unreachable; }; echo direct-exec-still-works)
assert_equal "direct-exec-still-works" "$X124" "exec_search's empty-function-list walk must still fall through to direct exec (see comment above)"

X125=$(g() { echo "sub:$(echo inner)"; }; g; g)
assert_equal "sub:inner
sub:inner" "$X125" "tree_copy must still re-evaluate a function body (which re-copies its N_ARG nodes) correctly across repeated calls (see comment above)"

## fixes/126: lib/path/path_fnmatch.c's '*' handling recursed once per
## character of the string under test (to search for a split point
## where the rest of the pattern matches) -- so any single
## "*"-containing glob/case pattern matched against a long enough
## string overflowed the stack and segfaulted. This is what a Termux
## (Android/bionic) segfault report in this repo's own "shish
## configure" traced back to (autoconf-generated scripts do exactly
## this kind of glob matching over long accumulated strings, e.g.
## "case $ac_configure_args in *\'*)"); confirmed to reproduce
## identically under Linux (not bionic-specific) via a
## -fsanitize=address build, which pinpointed the recursion via its
## stack-overflow report. Fixed by making the whole function fully
## iterative: a single "most recently seen '*'" backtrack bookmark
## (set when a '*' is matched, consulted whenever a later match
## attempt fails) replaces all recursion -- the standard technique for
## wildcard matching, using no stack of any kind (not even a bounded
## one), so match cost no longer depends on either the string's length
## or the pattern's number of '*'s. This is a real behavioral
## difference (crash vs. no crash), so it gets an actual regression
## case instead of a comment-only entry.
LONG126=$(printf 'x%.0s' $(seq 1 200000))
X126=$(case "$LONG126" in x*x) echo matched;; *) echo no-match;; esac)
assert_equal "matched" "$X126" "a '*' pattern matched against a very long string must not overflow the stack"

## fixes/127: two more real-configure-run UB findings from the same
## sanitizer sweep as fixes/123-125, found once fixes/126 let the run
## get past the point it used to stack-overflow at. Same "comment-only,
## no assertion can actually distinguish pre/post-fix output" situation
## as fixes/123-125 (see the long comment above them for why) -- both
## verified the same way, by rebuilding with -fsanitize=address,undefined
## and confirming a clean run of both this suite and "shish configure".
##
## src/redir/redir_source.c: the here-doc-processing loop advanced via
## "&redir_list->data->nredir" without checking whether "data" (the
## next here-doc's node, NULL once the last one in the list is
## reached) was itself NULL first -- same null-member-address UB as
## fixes/124's exec_search.c fix, same reason it's silent under a
## plain build (the member is at offset 0). Any script with more than
## one here-doc in it exercises this loop past its first (only) real
## element, e.g. the case below.
##
## lib/stralloc/stralloc_catb.c and stralloc_copyb.c: both called
## byte_copy() (a #define for memcpy(), lib/byte.h) unconditionally,
## even at len == 0 with buf == NULL (an empty/never-allocated
## stralloc -- e.g. a command substitution with no output at all).
## memcpy()'s second argument carries a nonnull attribute regardless of
## length. This is the same root cause as fixes/125's tree_copy.c fix,
## but fixed here at the shared primitive instead of the one call site
## that happened to be found first (tree_copy.c's own guard from
## fixes/125 is left in place too -- harmless, and it also skips an
## unnecessary stralloc_ready() call). Exercised by any empty command
## substitution glued next to other text, e.g. the case below.
X127A=$(cat <<A
first
A
cat <<B
second
B
)
assert_equal "first
second" "$X127A" "more than one here-doc in the same command must still all be read correctly (see comment above)"

X127B=$(echo "prefix$(true)suffix")
assert_equal "prefixsuffix" "$X127B" "text glued around an empty command substitution must still concatenate correctly (see comment above)"

## fixes/128: reported directly (not found via the ASan sweep above,
## but the same bug class as the already-documented, not-fixed
## BUGS: ubsan-buffer-op-proto-function-type-mismatch): lib/buffer.h's
## generic buffer_op_proto callback type is "ssize_t(int,void*,size_t,
## void*)", but stralloc_write() and term_read() (the only two of
## shish's own functions actually called through it at runtime -- the
## rest of that BUGS entry's cast sites are either external libc
## read()/write(), which -fsanitize=function can't flag at all, or
## dead code) each declared a different, incompatible signature and
## got cast at the call site instead. Calling through a mismatched
## function pointer type is UB regardless of whether the real ABI
## tolerates it (which it does here, on every platform this project
## targets -- hence no assertion here can distinguish pre/post-fix
## output, same situation as fixes/123-125's entry above). Fixed by
## changing both functions' own signatures to match buffer_op_proto
## exactly, verified the same way: a full ./configure run under
## -fsanitize=address,undefined with zero runtime-error reports left
## (stralloc_write's path is exercised by any command substitution,
## e.g. the case below; term_read's is interactive-terminal-only, not
## reachable from a non-interactive test file at all).
X128=$(echo "$(echo nested)")
assert_equal "nested" "$X128" "command substitution (stralloc_write's own call path) must still work correctly (see comment above)"

## fixes/129, fixes/130: two real memory leaks found chasing ASan leak
## reports on a full "./configure" run (BUGS:
## asan-leak-residue-not-fully-triaged). Both are pure resource-usage
## bugs -- a leaked buffer doesn't change any command's output -- so,
## like fixes/123-125/127 above, no assertion here can distinguish
## pre/post-fix behavior; verified instead by running the reproducers
## below directly under the ASan/UBSan build and confirming their
## leak reports are gone (they still run correctly here, just without
## anything to check the leak itself against).
##
## fixes/129 (lib/stralloc/stralloc_trunc.c): never updated the
## stralloc's allocated-capacity field after growing the buffer, so a
## stralloc freshly stralloc_init()'d (capacity 0) and then grown via
## stralloc_trunc() looked "unallocated" to a later stralloc_free() on
## it -- silently leaking the real buffer instead of freeing it.
## var_setvsa() hits this exact sequence once per for-loop iteration
## for the loop variable, so every for-loop leaked (once per
## iteration) before this fix.
X129=$(for x in a b c d e f g h i j; do :; done; echo "$x")
assert_equal "j" "$X129" "a for-loop must still set its loop variable correctly across many iterations (see comment above)"

## fixes/130 (src/parse/parse_simple_command.c): a local scratch
## stralloc used only to pass a nul/length-safe string to
## parse_findalias() was never freed, leaking on every simple command
## whose first word is a literal string -- i.e. most commands in most
## scripts.
X130=$(echo one; echo two; echo three)
assert_equal "one
two
three" "$X130" "ordinary simple commands (parse_simple_command's alias-lookup path) must still parse and run correctly (see comment above)"

## fixes/131 (src/tree.h): the real Termux segfault that kicked off
## this whole investigation, previously mis-blamed on packed-node
## alignment being merely "cosmetic UB, tolerated everywhere" (see
## BUGS's corrected entry for the full story) -- root cause was
## src/tree.h's own `__packed` macro colliding by name with Android
## Bionic's <sys/cdefs.h>, which already defines `__packed` for real,
## silently overriding shish's own deliberately-a-no-op definition and
## corrupting every union node field access on that one platform only.
## Per the "Writing a test" exception in CLAUDE.md for a fix that only
## triggers on a platform this repo isn't developed on (no Termux/
## Bionic build here to run a real regression case against), this is
## comment-only -- verified instead by reproducing the exact collision
## locally on a Linux ASan/UBSan build via
## -D__packed=__attribute__\(\(packed\)\), which crashed reliably
## (tree_cat.c:34, a corrupted near-null node pointer) before this fix
## and ran this repo's own full "./configure" clean afterward, with
## the same forcing flag still active -- i.e. the fix was confirmed to
## neutralize the exact condition Bionic creates, not just observed to
## "currently" not crash.

## fixes/132 (src/sh/sh_main.c): $SHELL was left untouched at startup,
## so it still held whatever shell the environment inherited it from
## (e.g. "/bin/bash" from a login shell) even while running under
## shish -- any script or program that consults $SHELL to find "the
## current shell" got a wrong answer. sh_main() now overwrites it with
## argv[0] (shish's own invocation path) right after importing the
## rest of the environment, the same way real shells set it to
## themselves. Reuses the same "readlink /proc/$$/exe to re-invoke
## this running shish" idiom as fixes/122 above (see $SHISH_SELF
## there), since this also needs a genuinely separate process invoked
## with a known argv[0].
if [ -n "$SHISH_SELF" ] && [ -x "$SHISH_SELF" ]; then
  X132=$(SHELL=/nonexistent/not-shish "$SHISH_SELF" -c 'echo "$SHELL"')
  assert_equal "$SHISH_SELF" "$X132" "\$SHELL must be overwritten with shish's own argv[0], not inherited from the environment (see comment above)"
fi

## fixes/133 (src/redir/redir_eval.c, src/redir/redir_dup.c): the
## local "stralloc sa" redir_eval() builds for every redirection's
## target word (via expand_copysa()) was never freed for a plain
## "> file"/"< file" redirection (redir_open() only ever str_dup()'d
## it) and only freed on 2 of redir_dup()'s 3 return paths (missed on
## "[n]<&[n]" self-referring-duplicate) -- a pure resource-usage bug
## like fixes/129/130/132 above, so no assertion here can distinguish
## pre/post-fix behavior; verified instead by running the reproducers
## below directly under the ASan/UBSan build and confirming their
## leak reports (previously present on every one of these forms) are
## gone. Still checked here for ordinary correctness, since freeing
## the wrong thing (or double-freeing redir_dup()'s now-caller-owned
## sa) would break every one of these redirection forms outright.
## (X133C below now asserts a "0" exit status rather than the "1" this
## comment's era actually produced -- self-referring duplicates are no
## longer rejected outright, see fixes/137.)
F133=$(mktemp -d)
: < /dev/null
assert_equal "0" "$?" "a plain input redirection must still succeed (see comment above)"

echo hi > "$F133/a" 2>"$F133/b"
X133A=$(cat "$F133/a")
assert_equal "hi" "$X133A" "output/error redirection (redir_open's R_OPEN path) must still work correctly"

exec 3>&1
exec 3>&-
X133B=$?
assert_equal "0" "$X133B" "fd-duplicating redirection (redir_dup's non-error path) must still work correctly"

(exec 3<&3) >/dev/null 2>&1
X133C=$?
assert_equal "0" "$X133C" "a self-referring duplicate redirection on a never-opened fd must be the no-op POSIX defines, not an error (see fixes/137 -- this case used to be (wrongly) rejected here too)"

X133D=$(cat <<EOF
hello
EOF
)
assert_equal "hello" "$X133D" "a here-document (redir_here's sa-ownership-transfer path) must still work correctly"

rm -rf "$F133"

## fixes/134 (src/parse/parse_pipeline.c): a pipeline ending in a
## dangling "|" (nothing after it, e.g. "echo hi |") made
## parse_pipeline()'s post-"|" loop hand a NULL "node" (parse_command()
## found nothing left to parse) straight to the tree_link() macro,
## which unconditionally computes "&(node)->next" -- a null-pointer
## member access, UB, caught by UBSan (pipeline-trailing-pipe-null-deref
## in BUGS). Release builds didn't crash (the bogus "address" was never
## actually dereferenced), so the truncated pipeline was just silently
## accepted and run as if the trailing "|" wasn't there. Now reports a
## proper syntax error and refuses to run it instead, matching how
## every other malformed construct in this parser already behaves.
## Run via $SHISH_SELF -c rather than inline here: a syntax error while
## parsing *this* test file itself (read via mmap, not "-c") takes
## parse_error()'s immediate sh_exit(1) path (see its own comment,
## BUGS: source-syntax-error-kills-whole-process-not-just-subshell's
## fix), which would kill this whole test run rather than just the
## intended nested reproducer.
if [ -n "$SHISH_SELF" ] && [ -x "$SHISH_SELF" ]; then
  X134A=$("$SHISH_SELF" -c 'echo before; echo hi |' 2>&1)
  case $X134A in
    *"unexpected token"*) X134A_MATCHED=yes ;;
    *) X134A_MATCHED=no ;;
  esac
  assert_equal "yes" "$X134A_MATCHED" "a pipeline ending in a dangling '|' must be reported as a syntax error, not silently accepted"

  X134B=$("$SHISH_SELF" -c 'echo hi | cat' 2>&1)
  assert_equal "hi" "$X134B" "an ordinary, complete pipeline must still work correctly"
fi

## fixes/135 (src/eval/eval_node_bgnd.c, src/exec/exec_command.c,
## src/exec/exec_program.c): backgrounding anything ("cmd &") printed
## the "[id] pid" job-start banner unconditionally, even in a plain
## non-interactive script -- job_wait.c's matching Done/Stopped
## banners were already correctly gated behind "sh->opts.monitor" (its
## own comment: "for interactive use only; suppress it in scripts so
## configure's stderr stays clean"), but the start banner at all three
## places a job gets created was missed. Confirmed against a real
## autoconf-generated `configure` (job-start-banner-printed-
## noninteractively): a stray "[1] 12345" line on stderr in the middle
## of an otherwise clean configure run. All three call sites now check
## the same "sh->opts.monitor" flag.
X135A=$(sleep 0.1 & wait; echo done)
assert_equal "done" "$X135A" "backgrounding a simple command must not print a job-start banner in a non-interactive shell"

X135B=$({ sleep 0.1; } & wait; echo done)
assert_equal "done" "$X135B" "backgrounding a compound command must not print a job-start banner in a non-interactive shell (eval_node_bgnd's path)"

X135C=$(true & wait; echo done)
assert_equal "done" "$X135C" "backgrounding an external program must not print a job-start banner in a non-interactive shell (exec_program's path)"

if [ -n "$SHISH_SELF" ] && [ -x "$SHISH_SELF" ]; then
  X135D=$("$SHISH_SELF" -mc 'sleep 0.1 & wait' 2>&1)
  case $X135D in
    "[1] "*) X135D_MATCHED=yes ;;
    *) X135D_MATCHED=no ;;
  esac
  assert_equal "yes" "$X135D_MATCHED" "the job-start banner must still print when monitor mode is actually on (-m)"
fi

## fixes/136 (src/expand/expand_param.c): "${a=$x} ${b=$x} ${c=$x}" inside
## a single double-quoted word assigned each later variable the *whole*
## accumulated text of the word so far (literal text plus every earlier
## substitution), not just its own default expansion -- S_ASGNDEF handed
## var_setvsa the shared accumulator node's entire stralloc instead of
## only the bytes appended by this substitution. Found via a real
## autoconf `configure` (gettext-runtime): its bare-positional-arg
## fallback `: "${build_alias=$ac_option} ${host_alias=$ac_option}
## ${target_alias=$ac_option}"` left host_alias/target_alias holding
## stray leading spaces, which made "checking host system type" invoke
## config.sub with no argument at all.
unset X136A X136B X136C
: "${X136A=} ${X136B=} ${X136C=}"
assert_equal "" "$X136A" "first default-assign in a quoted word must not pick up trailing text from later substitutions"
assert_equal "" "$X136B" "second default-assign in a quoted word must not inherit the literal space preceding it"
assert_equal "" "$X136C" "third default-assign in a quoted word must not inherit accumulated text from the first two"

## fixes/137 (src/redir/redir_eval.c, src/redir/redir_dup.c): "[n]<&n"/
## "[n]>&n" (duplicating a descriptor onto itself) was unconditionally
## rejected as an error, but POSIX defines dup2(fd, fd) as a no-op that
## succeeds trivially -- and the *unprefixed* form of this ("cmd >&1",
## which just defaults its source fd to 1) is an extremely common,
## totally unremarkable idiom, e.g. inside a "{ ...; } > file" group.
## autoconf's own generated `config.status` emits exactly that pattern
## while writing config.h, so building real projects (gettext) failed
## outright with "config.sub: missing argument" / "1: self-referring
## duplicate" (self-referring-duplicate-rejected-config-status, BUGS).
## The fix has to special-case this in redir_eval(), before fd_new()/
## fd_push() overwrite (or, for a persistent "exec" reusing the same fd
## number, destructively fd_reinit()/fd_close()) whatever currently
## occupies that fd slot -- by the time redir_dup() used to see it,
## the original binding was already gone, so there is no correct point
## afterward to special-case it from. See tests/fixed.sh's X133C above
## for the companion case (a self-dup of an fd that was never opened
## at all) that this same fix also corrected.
X137A=$(: >&1; echo done)
assert_equal "done" "$X137A" "a bare self-referencing '>&1' inside a command must be a no-op, not an error"

X137B=$(exec 3>&1; exec 3>&3; echo hi >&3)
assert_equal "hi" "$X137B" "self-dup of an fd that is itself already a dup of another fd must still leave it usable afterward"

if [ -n "$SHISH_SELF" ] && [ -x "$SHISH_SELF" ]; then
  X137C=$("$SHISH_SELF" -c 'exec 3>&1; (exec 3<&3); echo "exit=$?"')
  assert_equal "exit=0" "$X137C" "self-dup must not fail just because the requested direction (read) differs from how the fd was originally opened (write) -- dup2(fd,fd) never checks direction"
fi

## fixes/138 (src/expand/expand_cat.c): unquoted field-splitting planted a
## spurious empty argument in front of the first real field whenever a
## word's own value started with IFS whitespace *and* it wasn't the first
## word in the list -- expand_args.c pre-creates an empty placeholder node
## ahead of each subsequent word (so a later word that turns out genuinely
## empty, e.g. an unquoted expansion of "", still contributes an empty
## argument instead of vanishing); expand_cat()'s split loop couldn't tell
## that placeholder apart from a node that already held a real, finished
## field, so it "finalized" the (empty) placeholder as its own argument
## before starting the next one. Found via a real autoconf `configure`
## (gettext-runtime): its subdir-recursion loop, `for ac_dir in :
## $subdirs`, builds $subdirs by repeated `subdirs="$subdirs name"`
## starting from empty, leaving a leading space -- the bogus empty
## $ac_dir this produced made every subdirectory's own ./configure get
## invoked recursively with an empty argument instead of a real path.
X138A=$(v=" b c"; for x in a $v; do echo "[$x]"; done)
X138A_EXPECT="[a]
[b]
[c]"
assert_equal "$X138A_EXPECT" "$X138A" "a later word's own leading IFS whitespace must not plant a spurious empty argument ahead of it"

X138B=$(for x in a "" b; do echo "[$x]"; done)
X138B_EXPECT="[a]
[]
[b]"
assert_equal "$X138B_EXPECT" "$X138B" "a genuinely empty (quoted) word must still contribute its own empty argument, unlike X138A's case"

X138C=$(v=""; v="$v intl"; v="$v libasprintf"; for x in : $v; do echo "[$x]"; done)
X138C_EXPECT="[:]
[intl]
[libasprintf]"
assert_equal "$X138C_EXPECT" "$X138C" "a variable built up piecewise from empty (leaving a leading space) must not produce a spurious leading empty field when split (the exact autoconf subdirs pattern above)"

## fixes/139 (src/builtin/builtin_trap.c, lib/stralloc/stralloc_ready.c,
## src/sh/sh_forked.c): "trap CODE SIG1 SIG2 ..." parses CODE once and
## shared that single parsed tree, unowned, across a separate trap-list
## entry for every listed signal -- uninstalling those traps one at a
## time (e.g. "trap - 1 2 15", exactly what libtool's generated cleanup
## code does) tree_free()d the same shared tree again for the second and
## third signal, corrupting the heap every time. The corruption itself
## went unnoticed until some later, unrelated allocation finally walked
## into it, surfacing as a delayed, hard-to-place glibc
## "malloc_consolidate(): unaligned fastbin chunk detected" abort --
## confirmed root-caused (not just worked around) via valgrind directly
## on a real `config.status` run: zero errors before this fix, exactly
## one clean double-free report (via trap_uninstall -> tree_free) after
## isolating this fix alone, zero again with it applied. Also picked up
## and fixed two related, smaller issues found chasing this down along
## the way: sh_forked() (after a real fork()) unconditionally freed
## every ancestor env frame's positional-parameter array, even one the
## surviving frame still shared a pointer to (non-owning frames just
## copy arg.v verbatim, see sh_pushargs()) -- a real heap-use-after-free
## once that surviving frame's own $1/$2/... was expanded again,
## confirmed via ASan. And stralloc_ready() copied the *new, target*
## length from an old, aliased-but-unowned buffer (a==0, s!=NULL: e.g.
## var_set() pointing straight at an environment string) instead of the
## buffer's own real length, reading past wherever that buffer's actual
## allocation ends.
X139=$(trap "echo hi" 1 2 15; trap - 1 2 15; echo survived)
assert_equal "survived" "$X139" "uninstalling a trap that was installed for multiple signals at once (sharing one parsed command tree) must not corrupt the heap on the second/third signal"

if command -v valgrind >/dev/null 2>&1 && [ -n "$SHISH_SELF" ] && [ -x "$SHISH_SELF" ]; then
  X139_VG=$(valgrind --error-exitcode=17 -q "$SHISH_SELF" -c 'trap "echo hi" 1 2 15; trap - 1 2 15; echo ok' 2>&1)
  assert_equal "ok" "$X139_VG" "the same multi-signal trap uninstall must also be clean under valgrind (no invalid free/write reported)"
fi

## 140: "unalias" was registered as a bare alias of builtin_alias(),
## which has no removal logic at all and no "-a" option -- so
## "unalias name" silently left the alias in place (falling into
## builtin_alias's "define" path instead) and "unalias -a" just
## errored out with "invalid option", even though both were
## documented as working in help_alias's own text.
X140=$(alias foo=bar; unalias foo; alias)
assert_equal "" "$X140" "unalias NAME must actually remove the alias"

X140_ALL=$(alias foo=bar; alias baz=qux; unalias -a; alias)
assert_equal "" "$X140_ALL" "unalias -a must remove every defined alias"

X140_ERR=$(unalias nosuchalias 2>&1 1>/dev/null)
assert_match "$X140_ERR" "*no such alias*" "unalias on an undefined name must report an error instead of silently succeeding"

## 141: parse_arith_unary()'s operand fallback called parse_arith_value()
## (primaries only) instead of recursing back into parse_arith_unary(),
## so a *chained* unary operator ("-+-2": minus of (plus of (minus of
## 2))) left the outer unary node's operand NULL once the inner '+'/'-'
## was reached -- expand_arith_expr() then dereferenced that NULL node,
## segfaulting the whole shell instead of just failing to evaluate the
## one expression.
X141=$(echo $((-+-2)) $((~-2)) $((!!5)))
assert_equal "2 1 1" "$X141" "chained unary arithmetic operators (sign/bitwise-not/logical-not) must not segfault"

## 142: an unset (or empty) variable used directly in arithmetic
## context ("$((x))") is required by POSIX to evaluate as 0, but
## N_ARGPARAM's "scanned zero digits" case in expand_arith_expr()
## returned failure (ret = 1) even for this intentional "value is
## empty, default to 0" path -- expand_arith() treats any nonzero
## return as "expansion failed" and produces no output at all, rather
## than substituting the 0 it had already computed into *r.
X142=$(unset x; echo $((x)))
assert_equal "0" "$X142" "an unset variable in arithmetic context must expand to 0, not silently produce no output"

## 143: several precedence/tokenizing bugs in parse_arith_binary(),
## all found chasing down the same "operator precedence" test group:
## (a) the bitwise "&"/"|"/"^" check only excluded a following '=', so
## it also matched the *first* character of "&&"/"||", misparsing e.g.
## "3&&-5" as one-character bitwise-AND "3&" plus a dangling "&-5";
## (b) the same collision existed between relational "<"/">" and shift
## "<<"/">>", breaking chained shifts like "1<<2<<1"; (c) the do-while
## that searches downward for the matching precedence level kept
## decrementing its "precedence" variable even after finding a match on
## the very first check, so the right operand's own recursion bound
## itself one level too loose and a later, tighter operator (e.g. the
## "*3" in "1+2*3") got left for an ancestor frame to wrongly re-group
## as "(1+2)*3" instead of "1+(2*3)".
X143=$(echo $((3&&-5)) $((3||-5)) $((1<<2<<1)) $((1+2*3)) $((9-2*3)))
assert_equal "1 1 8 7 3" "$X143" "&&/|| vs &/|, <</>> vs <//>, and cross-precedence grouping must all parse correctly"

## 144: expand_arith_binary() evaluated both operands of "&&"/"||"
## unconditionally before even looking at which operator it was, so
## the right operand's side effects (e.g. an assignment) always ran
## even when the left operand already decided the result and POSIX
## requires short-circuiting -- "0 && (a=5)" wrongly still set a to 5.
X144=$(a=0; : $((0&&(a=5))); echo $a)
assert_equal "0" "$X144" "arithmetic && must not evaluate its right operand once the left one is already false"

X144_OR=$(a=0; : $((1||(a=5))); echo $a)
assert_equal "0" "$X144_OR" "arithmetic || must not evaluate its right operand once the left one is already true"

## 145: parse_arith_value() only recognized "$(...)"/"$((...))" as a
## command-substitution-shaped operand, never a bare "\`cmd\`" -- so
## "$((1+\`echo 10\`))" failed to parse at all even though the
## "$(...)" equivalent worked fine.
X145=$(echo $((1+`echo 10`)))
assert_equal "11" "$X145" "legacy backquoted command substitution must be usable as an arithmetic operand"

## 146: eval_for() pushes its own "en" eval frame (so break/continue
## can target this loop specifically) but runs the loop body against
## the *caller's* "e" instead, needed so the body's commands update
## the same $?/errexit state the rest of the script sees -- yet it
## returned eval_pop(&en)'s "en.exitcode", a field nothing ever wrote,
## always 0 regardless of the body. This was invisible whenever a
## later command in the *same* top-level list (e.g. "for ...; done;
## echo $?" all on one line) read "$?" before sh_loop() next synced
## its own status global from a *different* top-level list's result --
## but a for loop sitting on its own lines (the common, idiomatic
## case) immediately clobbered "$?" to 0 right after finishing, no
## matter what its body actually exited with.
X146=$(for i in 1; do
  false
done
echo $?)
assert_equal "1" "$X146" "a for-loop on its own must report its body's real exit status, not always 0"

X146_EMPTY=$(true; for i in ; do false; done; echo $?)
assert_equal "0" "$X146_EMPTY" "a for-loop whose item list is empty (body never runs) must report exit status 0"

X146_BREAK=$(for i in 1 2; do false; break; done; echo $?)
assert_equal "0" "$X146_BREAK" "a for-loop ended by 'break' must report break's own exit status (0), not stale state"

## 147: the same bug as 146, in eval_loop() (while/until), plus a
## second one specific to it: even after tracking the body's own
## exitcode, the *next* iteration's test re-run (needed to decide
## whether to keep looping) executes against the same shared "e" and
## overwrites it before the loop exits -- so the loop's reported
## status came out as that of its final, loop-ending *test* instead of
## its last real *body* command. Needed a snapshot taken immediately
## after each body run, not a read of "e" after the surrounding
## for(;;) in eval_loop() has already moved on.
X147=$(i=0
while [ "$i" -lt 1 ]; do
  i=1
  false
done
echo $?)
assert_equal "1" "$X147" "a while-loop on its own must report its last body command's real exit status, not the loop-ending test's"

X147_UNTIL=$(i=0
until [ "$i" -ge 1 ]; do
  i=1
  false
done
echo $?)
assert_equal "1" "$X147_UNTIL" "same as above, for until"

X147_NEVER=$(true; while false; do echo nope; done; echo $?)
assert_equal "0" "$X147_NEVER" "a while-loop whose test fails immediately (body never runs) must report exit status 0"

## 148: eval_for() distinguished "for x in <list>" from a bare
## "for x; do" (which POSIX says must fall back to iterating the
## positional parameters) purely by checking whether nfor->args was
## non-NULL -- but an *explicit*, merely empty "in" list ("for x in ;
## do") leaves nfor->args NULL exactly the same way as never having an
## "in" clause at all, so it wrongly fell into the positional-param
## fallback and iterated $1, $2, ... instead of zero times. In a
## command-substitution subshell context where sh->arg.c/sh->arg.v can
## disagree about how many args are actually live, that same wrong
## fallback also dereferenced a bogus pointer and segfaulted the whole
## shell instead of just running the loop zero times.
set -- pos1 pos2
X148=$(for i in ; do echo "got:$i"; done; echo done)
assert_equal "done" "$X148" "'for x in ;' (explicit, empty list) must iterate zero times, not fall back to \$1 \$2 ..."

X148_BARE=$(for i; do echo "got:$i"; done)
assert_equal "got:pos1
got:pos2" "$X148_BARE" "a bare 'for x;' (no 'in' clause at all) must still fall back to the positional parameters"

X148_SUBSHELL=$(true; for i in ; do false; done; echo marker=$?)
assert_equal "marker=0" "$X148_SUBSHELL" "'for x in ;' inside a command substitution must not crash and must report exit status 0"

## 149: eval_jump() (break/continue) discarded a *successfully* matched
## enclosing loop ("j") the moment it walked as far as a function/
## subshell/top-level (E_ROOT) boundary with leftover, unsatisfied
## "levels" -- conflating "break N asked for more levels than there
## are enclosing loops here" (bash: not an error, just targets the
## outermost loop reachable without crossing the boundary) with "no
## loop was found *at all* before hitting the boundary" (a real
## escape attempt, correctly a no-op). A top-level script's own eval
## frame also carries E_ROOT (see sh_loop.c's "E_ROOT | E_LIST"
## tempflags), so even an ordinary "break 2" from a single enclosing
## loop at the top of a plain script silently failed to break
## anything at all instead of breaking that one loop.
X149=$(for i in 1; do
  break 2
  echo not_reached
done
echo after)
assert_equal "after" "$X149" "'break N' with N greater than the actual nesting depth must still break the outermost enclosing loop"

X149_DEEP=$(for i in 1; do
  for j in a; do
    break 3
    echo not_reached_1
  done
  echo not_reached_2
done
echo after)
assert_equal "after" "$X149_DEEP" "'break N' exceeding nesting depth by more than one level must still break all enclosing loops"

## 150: $((cond ? a : b)) had no parser support at all -- parse_arith_binary.c
## stopped at the binary operators, with no precedence level or node kind
## for "?:". Added parse_arith_ternary.c as its own level above the binary
## chain: right-associative (chained "a?b:c?d:e" groups as "a?b:(c?d:e)")
## and short-circuiting (only the taken branch's side effects run).
X150=$(echo $((1?2:3)) $((0?2:3)))
assert_equal "2 3" "$X150" "\$((cond ? a : b)) must parse and evaluate to the taken branch"

X150_CHAIN=$(echo $((0?1:0?2:3)))
assert_equal "3" "$X150_CHAIN" "chained '?:' must associate right-to-left"

a=0 b=0
: $((1?(a=5):(b=-5)))
assert_equal "5 0" "$a $b" "only the taken branch of '?:' may run its side effects"

## 151: "&", "^", "|" were all handled by a single combined precedence
## level in parse_arith_binary.c instead of three distinct, increasingly
## looser C/POSIX levels ("&" tightest), and likewise "&&"/"||" shared one
## level instead of "&&" binding tighter than "||". Split each into its
## own level (renumbering everything above accordingly).
X151_BIT=$(echo $((1^0&0)))
assert_equal "1" "$X151_BIT" "'&' must bind tighter than '^' (1^(0&0), not (1^0)&0)"

X151_LOG=$(echo $((1||0&&0)))
assert_equal "1" "$X151_LOG" "'&&' must bind tighter than '||' (1||(0&&0), not (1||0)&&0)"

## 152: $((a=5)) and $((a++)) against a readonly variable silently
## succeeded (and actually reassigned it) instead of erroring.
## var_setv() and var_setvint() now check V_READONLY the same way
## var_set() already did; arithmetic callers sh_exit(1) on failure.
readonly a152=3
OUT152=$( (echo $((a152=5))) 2>/dev/null)
assert_equal "" "$OUT152" "\$((a=5)) must not print when a is readonly"

assert_equal "3" "$a152" "readonly variable must not be reassigned by \$(())"

OUT152_PLUS=$( (echo $((a152+=5))) 2>/dev/null)
assert_equal "" "$OUT152_PLUS" "\$((a+=5)) must not print when a is readonly"

## 153: kill -s was not recognized — only the -signal operand form
## was parsed, so "kill -s USR1 $$" failed with "invalid signal
## specification". Added -s signal_name option parsing and -l to
## list signal names.
OUT153_L=$(kill -l)
assert_match "$OUT153_L" "*HUP*" "kill -l must list signal names including HUP"
assert_match "$OUT153_L" "*TERM*" "kill -l must list signal names including TERM"

OUT153_BOGUS=$(kill -s BOGUS 0 2>&1)
assert_match "$OUT153_BOGUS" "*invalid signal*" "kill -s BOGUS must report invalid signal"

## 154: ${#-default} and ${#+alternate} were misparsed — the parser
## saw '#' followed by a valid parameter character ('-' or '+') and
## treated '#' as the length operator instead of the parameter name.
## POSIX requires these to be parameter expansions with default/
## alternate values for parameter # (number of positional params).
set --
assert_equal "${#-empty}" "0" "\${#-default} must expand to value of #, not length of -"
assert_equal "${#+alternate}" "alternate" "\${#+alternate} must expand to alternate when # is 0"
set -- a b c
assert_equal "${#-empty}" "3" "\${#-default} with 3 args must expand to 3"
assert_equal "${#+yes}" "yes" "\${#+alternate} with args must expand to alternate"
set --

## 155: special builtin errors were not fatal in non-interactive mode.
## POSIX requires the shell to exit when a special builtin (break,
## continue, eval, exec, export, readonly, return, set, shift, times,
## trap, unset, ., :, or a syntax error in any of these) encounters
## an error in non-interactive mode. Fixed by adding sh_exit() calls
## in exec_command() and eval_simple_command() when cmd->id == H_SBUILTIN.
OUT155_SHIFT=$( (shift 999 2>/dev/null; echo "not reached") 2>&1; echo "parent alive")
assert_equal "parent alive" "$OUT155_SHIFT" "a special builtin's error ends the subshell it happened in, not the parent"

OUT155_READONLY=$( (readonly 123invalid 2>/dev/null; echo "reached") 2>&1)
assert_equal "reached" "$OUT155_READONLY" "readonly error in subshell must not kill parent"

# Assignment errors on special builtins should cause subshell to exit
OUT155_ASSIGN=$( (readonly x=1; x=2 :) 2>/dev/null )
assert_equal "" "$OUT155_ASSIGN" "assignment error to readonly on special builtin must kill subshell"

## 156: PPID was not set to parent process ID. The shell never called
## getppid() to initialize $PPID. Fixed by adding var_setvint("PPID",
## getppid(), 0) in sh_init.c after setting sh_pid.
PPID_VAL=$PPID
assert_match "$PPID_VAL" "[0-9]*" "\$PPID must be set to a numeric value"

## 157: unset on a readonly variable silently succeeded instead of
## rejecting the operation. POSIX requires unset to fail on readonly
## variables (2.9.1.43). Fixed by checking V_READONLY flag in
## builtin_unset() before calling var_unset().
readonly UNSET157=readonlyval
unset UNSET157 2>/dev/null
assert_equal "readonlyval" "$UNSET157" "unset must not remove a readonly variable"

## 158: test -g, -p, -u were not implemented. The unary operators for
## set-group-ID, named pipe (FIFO), and set-user-ID were missing from
## builtin_test.c's getopt string and switch statement. Fixed by adding
## g:u: to getopt and cases for 'g' and 'u' in the switch (p was already
## present but not in getopt string).
TESTFIFO=/tmp/shish-test-fifo-158
mkfifo "$TESTFIFO" 2>/dev/null
test -p "$TESTFIFO"
assert_equal "0" "$?" "test -p must return true for a FIFO"
rm -f "$TESTFIFO"

TESTGID=/tmp/shish-test-gid-158
touch "$TESTGID"
/bin/chmod g+s "$TESTGID" 2>/dev/null
test -g "$TESTGID"
assert_equal "0" "$?" "test -g must return true for set-group-ID file"
rm -f "$TESTGID"

TESTUID=/tmp/shish-test-uid-158
touch "$TESTUID"
/bin/chmod u+s "$TESTUID" 2>/dev/null
test -u "$TESTUID"
assert_equal "0" "$?" "test -u must return true for set-user-ID file"
rm -f "$TESTUID"

# Negative tests: ensure these return false when bits are not set
TESTPLAIN=/tmp/shish-test-plain-158
touch "$TESTPLAIN"
test -g "$TESTPLAIN"
assert_equal "1" "$?" "test -g must return false for file without set-group-ID"
test -u "$TESTPLAIN"
assert_equal "1" "$?" "test -u must return false for file without set-user-ID"
rm -f "$TESTPLAIN"

## 159: the times builtin was not implemented at all. POSIX requires
## it to print accumulated user and system times for the shell and
## its children in XmY.YYYYYYs format (6 decimal places). Fixed by
## adding builtin_times.c using times(2) and buffer/fmt functions.
TIMES_OUT=$(times)
TIMES_LINES=$(echo "$TIMES_OUT" | wc -l)
assert_equal "2" "$TIMES_LINES" "times must produce exactly 2 lines of output"
echo "$TIMES_OUT" | head -1 | grep -qE '^[0-9]+m[0-9]+\.[0-9]{6}s [0-9]+m[0-9]+\.[0-9]{6}s$'
assert_equal "0" "$?" "times line 1 must match XmY.YYYYYYs XmY.YYYYYYs format"
echo "$TIMES_OUT" | tail -1 | grep -qE '^[0-9]+m[0-9]+\.[0-9]{6}s [0-9]+m[0-9]+\.[0-9]{6}s$'
assert_equal "0" "$?" "times line 2 must match XmY.YYYYYYs XmY.YYYYYYs format"

# Verify times is a special builtin (can be invoked without PATH)
OLDPATH="$PATH"
PATH=""
times >/dev/null 2>&1
TIMES_STATUS=$?
PATH="$OLDPATH"
assert_equal "0" "$TIMES_STATUS" "times must be invocable without PATH (special builtin)"

## 160: readonly builtin reassignment was not fatal in non-interactive mode.
## POSIX requires special builtins to exit the shell on error in
## non-interactive mode. readonly a=1; readonly a=2 should kill the shell.
OUT160=$( (readonly a=1; readonly a=2; echo "reached") 2>/dev/null)
assert_nomatch "$OUT160" "reached" "readonly reassignment must kill non-interactive shell before reaching echo"

## 161: unset -f did not remove functions from the exec_hash cache.
## After unset -f, type and command lookup still found the stale cached
## function entry. Fixed by invalidating exec_hash entry after freeing
## the function node (same as eval_function.c does on redefinition).
f161() { echo "f161 called"; }
unset -f f161
f161_OUT=$(f161 2>&1)
assert_match "$f161_OUT" "*No such file*" "unset -f must remove function so subsequent calls fail"

## 162: export on a readonly variable silently succeeded instead of
## rejecting the assignment. Fixed by checking V_READONLY before
## calling var_copys in builtin_export.c.
readonly RO162=oldval
export RO162=newval 2>/dev/null
assert_equal "oldval" "$RO162" "export must not reassign a readonly variable"

## 163: kill -l with a signal number should translate it to a signal name,
## not list all signals. POSIX requires kill -l <exit_status> to translate
## an exit status (128+N) or signal number to its name. Fixed by adding
## argument handling to the -l option in builtin_kill.c.
KILL_L_15=$(kill -l 15)
assert_equal "TERM" "$KILL_L_15" "kill -l 15 must print TERM"
KILL_L_143=$(kill -l 143)
assert_equal "TERM" "$KILL_L_143" "kill -l 143 (128+15) must print TERM"
KILL_L_9=$(kill -l 9)
assert_equal "KILL" "$KILL_L_9" "kill -l 9 must print KILL"

## 164: . (source/dot) builtin did not search PATH for scripts without
## slashes. POSIX requires that if the filename does not contain a slash,
## the shell shall search PATH. Fixed by adding source_search_path() that
## searches PATH for readable (not executable) files.
echo 'echo "hello from PATH"' > /tmp/test164.sh
export OLD_PATH="$PATH"
export PATH=/tmp
DOT_OUT=$(. test164.sh)
export PATH="$OLD_PATH"
rm -f /tmp/test164.sh
assert_equal "hello from PATH" "$DOT_OUT" "dot must search PATH for scripts without slashes"

## 165: command -v did not recognize reserved words (if, for, while, etc.)
## as commands. Fixed by checking if the name is a keyword before attempting
## PATH lookup in builtin_command.c.
CMD_V_IF=$(command -v if)
assert_equal "if" "$CMD_V_IF" "command -v must recognize reserved word 'if'"
CMD_V_FOR=$(command -v for)
assert_equal "for" "$CMD_V_FOR" "command -v must recognize reserved word 'for'"
CMD_V_WHILE=$(command -v while)
assert_equal "while" "$CMD_V_WHILE" "command -v must recognize reserved word 'while'"

## fixes/100: chmod -R aborted the whole operation with "No such file or
## directory" as soon as it hit a symlink encountered while recursing
## into a directory -- it stat()ed (dereferencing) and chmod()ed every
## entry unconditionally, so a dangling symlink (or even a valid one,
## once the dangling case was fixed to not stat() at all) made the
## traversal fail instead of being left untouched, unlike GNU chmod
## ("neither symbolic link 'x' nor referent has been changed"). Fixed
## by adding a 'toplevel' flag to chmod_path(): a symlink found via
## readdir() during recursion is now skipped entirely (no stat()/
## chmod() attempted), while a symlink named directly as a file
## operand is still dereferenced and chmoded as before, so an
## explicitly-given dangling symlink is still reported as an error.
##
## Not exercisable through this default ctest build: chmod is an
## EXTRA_BUILTINS entry, off by default (BUILTIN_CHMOD=0), so "chmod"
## here resolves to the system PATH binary, which doesn't have this
## bug in the first place -- a test running against it would pass
## whether or not the fix is present, so it wouldn't actually be
## testing anything (same situation as fixes/40). Verified instead by
## building a separate tree with -DENABLE_CHMOD=ON and confirming:
## "chmod -R -v a+rX dir" over a directory containing a dangling
## symlink now succeeds (exit 0), prints "neither symbolic link 'dir/x'
## nor referent has been changed" for it while still applying the mode
## to real files/directories alongside it, and a dangling symlink
## given directly as a file operand (not found via recursion) still
## fails with "No such file or directory", matching GNU chmod.

## fixes/101: shift printed a "can't shift that many" diagnostic to
## stderr whenever n > $#. POSIX only specifies the exit status for
## this case ("> 0 if n>$#; otherwise, it is zero") and leaves any
## diagnostic message unspecified/implementation-defined; shift should
## report failure through its exit status alone and otherwise stay
## silent. Fixed by dropping the message from builtin_shift.c (the
## n > $# check now just returns 1).
SHIFT101_ERR=$( (set -- a b; shift 5) 2>&1 >/dev/null)
assert_equal "" "$SHIFT101_ERR" "shift with n > \$# must not print anything to stderr"
(set -- a b; shift 5)
assert_equal "1" "$?" "shift with n > \$# must still exit with status 1"
## "shift 5" with $# = 2 is an operand error, and an operand error in a
## special builtin ends a non-interactive shell, so nothing running in
## that same shell afterwards can look at the positional parameters --
## the silence and the status above are what is left to check here.
SHIFT101_ARGS=$( (set -- a b; shift 5 2>/dev/null; echo "$#:$*") 2>&1)
assert_equal "" "$SHIFT101_ARGS" "shift with n > \$# must run nothing further in that shell"

## fixes/102: getopts diverged from POSIX in several ways:
## - $OPTIND was never initialized at shell startup (POSIX: "Whenever
##   the shell is invoked, OPTIND shall be initialized to 1"), so a
##   script reading it before the first getopts call saw it unset.
## - the "arg..." form ("getopts optstring name arg...", parsing the
##   given operands instead of $1..) was off by one: it included the
##   'name' operand itself as the first argument to parse, so this
##   form never actually worked.
## - getopts' own persistent parser state ignored $OPTIND entirely
##   once set: manually resetting OPTIND=1 (the POSIX-documented way
##   to restart parsing) had no effect, since only the internal
##   struct optstate, never re-synced from the variable, drove
##   parsing.
## - a leading ':' in optstring (silent error reporting) wasn't
##   implemented at all: OPTARG was never set to the offending option
##   character, and diagnostics couldn't be suppressed.
## - exit status was wrong (2, not 0) whenever an unknown option was
##   returned -- POSIX only wants a non-zero exit once the end of
##   options is reached -- and 'name' was set to the offending option
##   letter instead of '?'/':' on errors, or left unset (instead of
##   '?') once options were exhausted.
## - $OPTIND lagged by one call: it kept pointing at the argument just
##   consumed instead of the next one to process, once that argument
##   had no characters left in it (shell_getopt_r() only advanced past
##   a fully-consumed argv element on the *following* call).
## Fixed across src/sh/sh_init.c, src/builtin/builtin_getopts.c, and
## lib/shell/shell_getopt.c (the last one is shared by every other
## builtin using shell_getopt(), but they only ever check the final
## post-loop position, never an intermediate value, so advancing
## eagerly doesn't affect them).
if [ -n "$SHISH_SELF" ] && [ -x "$SHISH_SELF" ]; then
  GETOPTS102_OPTIND=$("$SHISH_SELF" -c 'echo $OPTIND')
  assert_equal "1" "$GETOPTS102_OPTIND" "OPTIND must be initialized to 1 when the shell is invoked"

  GETOPTS102_EXPORTED=$("$SHISH_SELF" -c 'getopts a: o -a arg; export -p' | grep -c OPTIND)
  assert_equal "0" "$GETOPTS102_EXPORTED" "OPTIND must not be exported by default"
fi

GETOPTS102_ARGFORM=$(getopts ab:c o -a -b arg -c && printf '1[%s]' "$o"
  getopts ab:c o -a -b arg -c && printf '2[%s]' "$o"
  getopts ab:c o -a -b arg -c && printf '3[%s]' "$o")
assert_equal "1[a]2[b]3[c]" "$GETOPTS102_ARGFORM" "getopts must parse its own \"arg...\" operands, not the name operand"

GETOPTS102_RESET=$(set -- -a -b
  getopts ab o >/dev/null
  getopts ab o >/dev/null
  OPTIND=1
  getopts ab o && printf '%s' "$o")
assert_equal "a" "$GETOPTS102_RESET" "setting OPTIND=1 must restart option parsing"

GETOPTS102_UNKNOWN_STATUS=$(getopts '' o -a 2>/dev/null; echo "$?:$o")
assert_equal "0:?" "$GETOPTS102_UNKNOWN_STATUS" "exit status must be 0 and name '?' when an unknown option is found"

GETOPTS102_SILENT=$(getopts :a: v -a; printf '%s|%s' "$v" "$OPTARG")
assert_equal ":|a" "$GETOPTS102_SILENT" "a leading ':' in optstring must report a missing argument via name=':' and OPTARG=<opt>"

GETOPTS102_END=$(getopts a x -a >/dev/null; getopts a x -a; printf '%s' "$x")
assert_equal "?" "$GETOPTS102_END" "name must be set to '?', not left unset, once options are exhausted"

GETOPTS102_OPTIND_ADV=$(set -- -a -b
  getopts ab o >/dev/null; printf '%s' "$OPTIND"
  getopts ab o >/dev/null; printf ':%s' "$OPTIND")
assert_equal "2:3" "$GETOPTS102_OPTIND_ADV" "OPTIND must already point past a fully-consumed option by the time getopts returns"

## fixes/177: asynchronous lists ("cmd &") were broken in three
## independent ways:
## - the lexer's character-class table classified '&' as C_ARITHOP
##   only, not C_CTRL, so "true&echo ok" (no space before '&') was
##   tokenized as a single word "true&echo" instead of three tokens --
##   real shells accept '&' immediately after a word with no
##   whitespace, same as ';' or '|'.
## - POSIX 2.9.3.1 requires an asynchronous list's stdin to default to
##   /dev/null (before its own explicit redirections, if any) unless
##   job control is enabled; shish never did this, so a backgrounded
##   command with no redirection of its own read whatever fd 0
##   happened to mean in the shell (e.g. the running script itself).
## - fd_in and fd_src share the same underlying buffer object (see
##   sh_main.c) -- fork() copies that buffer's already-read-ahead
##   bytes into the child, so even after adding the /dev/null default
##   above, a backgrounded command reading via fd_in->r could still
##   see stale, already-buffered script bytes before its first real
##   read(2) ever reached the new fd.
## Fixed across src/parse/parse_chartable.c (the lexer table),
## src/eval/eval_simple_command.c (the /dev/null default, applied
## before a command's own redirections so it's still correctly
## overridden by e.g. "cmd <&0 &"), and src/job/job_fork.c (discarding
## the stale read-ahead buffer in the child). A matching fix was also
## added to exec_program.c and later removed again once confirmed
## redundant: H_PROGRAM commands always reach exec_program() via
## eval_simple_command.c, which the eval_simple_command.c fix above
## already covers.
if [ -n "$SHISH_SELF" ] && [ -x "$SHISH_SELF" ]; then
  ASYNC177_LEX=$("$SHISH_SELF" -c 'true&echo ok; wait' 2>&1)
  assert_equal "ok" "$ASYNC177_LEX" "'&' must end a command even with no preceding whitespace"

  ASYNC177_STDIN=$(printf '' | timeout 5 "$SHISH_SELF" -c 'cat & wait; echo done')
  assert_equal "done" "$ASYNC177_STDIN" "an unredirected background command's stdin must default to /dev/null, not hang reading the shell's own input"

  ASYNC177_BUFFER=$(printf 'cat& wait\necho hello\n' | timeout 5 "$SHISH_SELF")
  assert_equal "hello" "$ASYNC177_BUFFER" "a background command must not see script bytes the parent had already buffered ahead of it"
fi

## fixes/178: a case pattern (or the case word itself) that spells a
## reserved word verbatim, e.g. "case if in if) ...; esac", failed to
## parse. parse_case.c already requested P_NOKEYWD for pattern words,
## but parse_gettok()'s pushback mechanism only re-tokenizes when
## !p->pushback -- the token immediately after "in" (fetched by the
## loop's own P_SKIPNL-only, non-P_NOKEYWD parse_gettok() call, needed
## so it can still recognize "esac") had therefore already been
## resolved to its keyword's token bit before the P_NOKEYWD-requesting
## pattern-word loop ever got a chance to ask for it as a plain word.
## Fixed by downgrading that already-resolved keyword token back to
## T_NAME before pushing it back, since parse_keyword() never mutates
## the raw text backing it. Also gave the case word itself (between
## "case" and "in") P_NOKEYWD, for the same "case if in ..." reason.
if [ -n "$SHISH_SELF" ] && [ -x "$SHISH_SELF" ]; then
  CASE178_PATTERN=$("$SHISH_SELF" -c 'case if in if) echo matched;; esac')
  assert_equal "matched" "$CASE178_PATTERN" "a case pattern spelling a reserved word verbatim must still match as a plain word"

  CASE178_WORD=$("$SHISH_SELF" -c 'case if in *) echo matched;; esac')
  assert_equal "matched" "$CASE178_WORD" "the case word itself may spell a reserved word verbatim"
fi

## also part of fixes/178: a linebreak between the case word and the
## "in" keyword ("case foo\nin foo) ...; esac", grammatically valid
## per 3.9.4.3's own <linebreak>) failed to parse, since parse_case.c
## asked for T_IN without P_SKIPNL. Fixed by adding it, same as
## parse_for.c's own linebreak before "do".
if [ -n "$SHISH_SELF" ] && [ -x "$SHISH_SELF" ]; then
  CASE178_LINEBREAK=$(printf 'case foo\n\nin foo) echo matched;; esac\n' | "$SHISH_SELF")
  assert_equal "matched" "$CASE178_LINEBREAK" "a linebreak between the case word and 'in' must be allowed"
fi

## also part of fixes/178: an empty case body ("case x in esac", no
## patterns at all -- syntactically valid, POSIX-unspecified exit
## status aside) crashed the shell. tree_cat_n()'s N_CASE branch
## passes ncase.list (NULL for an empty case) straight to
## tree_catlist_n() with no NULL check, and tree_catlist_n() itself
## dereferenced it unconditionally in its do/while loop --
## unreachable through eval_case.c (which does check), but sh_loop()
## unconditionally stringifies every parsed command via tree_catlist()
## for history, regardless of whether the shell is interactive. Fixed
## by making tree_catlist_n() a no-op on a NULL node, the actual crash
## site, so every other (already correctly NULL-checking) caller stays
## unaffected.
if [ -n "$SHISH_SELF" ] && [ -x "$SHISH_SELF" ]; then
  CASE178_EMPTY=$("$SHISH_SELF" -c 'case x in esac; echo ok')
  assert_equal "ok" "$CASE178_EMPTY" "an empty case body must not crash the shell"
fi

## fixes/179: cd/CDPATH had several bugs:
## - path_canonicalize() never verified that the final path component
##   actually existed (its S_ISDIR check was #if 0'd out), so
##   path_realpath() always "succeeded" syntactically regardless of
##   whether the target existed. builtin_cd.c's CDPATH search loop
##   relied on that success/failure to know when to stop, so it always
##   stopped after the *first* CDPATH component, real or not, instead
##   of trying the rest of $CDPATH.
## - newcwd (the stralloc receiving each candidate) was never cleared
##   between attempts, and path_realpath() only prepends the shell's
##   cwd when its target's length is still zero, so a second attempt
##   after a first failure would silently corrupt further.
## - the operand's raw, uncanonicalized candidate is now stat()'d
##   directly (not the canonicalized result) before accepting it, so a
##   candidate that runs through a non-directory component (e.g.
##   "file/../dev") is correctly rejected the same way a real chdir()
##   would reject it, instead of "succeeding" once ".." is collapsed
##   away textually.
## - CDPATH was consulted even when the operand began with "." or
##   "..", which POSIX excludes; and once every CDPATH component
##   failed, cd gave up instead of falling back to the operand
##   relative to the current directory (POSIX's final fallback step).
## - OLDPWD was never set, and "cd -" (change to $OLDPWD, print the
##   new directory) was not implemented at all.
## - separately, lib/path/path_canonicalize.c resolved symlinks
##   unconditionally, even when told to keep them (the "symbolic"
##   parameter, i.e. cd's default -L mode) -- only -P (the "symbolic
##   == 0" / physical case) is supposed to walk through a symlink to
##   its target.
## All fixed in src/builtin/builtin_cd.c and
## lib/path/path_canonicalize.c.
if [ -n "$SHISH_SELF" ] && [ -x "$SHISH_SELF" ]; then
  CD179_DIR=$(mktemp -d)
  CD179_DIR=$(cd "$CD179_DIR" && pwd)
  mkdir -p "$CD179_DIR/cdpath1" "$CD179_DIR/cdpath2/foo" "$CD179_DIR/dev"
  ln -s cdpath2/foo "$CD179_DIR/link"
  >"$CD179_DIR/file"

  CD179_MULTI=$(cd "$CD179_DIR" && CDPATH=cdpath1:cdpath2 "$SHISH_SELF" -c 'cd foo >/dev/null && pwd')
  assert_equal "$CD179_DIR/cdpath2/foo" "$CD179_MULTI" "CDPATH search must try every component, not just the first"

  CD179_OLDPWD=$(cd "$CD179_DIR" && "$SHISH_SELF" -c 'cd cdpath1 >/dev/null; cd - >/dev/null; pwd')
  assert_equal "$CD179_DIR" "$CD179_OLDPWD" "cd - must change to \$OLDPWD"

  CD179_DOT=$(cd "$CD179_DIR" && CDPATH=cdpath2 "$SHISH_SELF" -c 'cd ./dev && pwd')
  assert_equal "$CD179_DIR/dev" "$CD179_DOT" "an operand starting with './' must ignore CDPATH"

  CD179_NOTDIR=$(cd "$CD179_DIR" && "$SHISH_SELF" -c 'cd ./file/../dev' 2>/dev/null; echo "$?")
  assert_equal "1" "$CD179_NOTDIR" "cd must fail when a non-final path component is not a directory"

  CD179_FALLBACK=$(cd "$CD179_DIR" && CDPATH=cdpath1:cdpath2 "$SHISH_SELF" -c 'cd cdpath1 && pwd')
  assert_equal "$CD179_DIR/cdpath1" "$CD179_FALLBACK" "cd must fall back to the plain operand relative to the current directory once CDPATH is exhausted"

  CD179_LOGICAL=$(cd "$CD179_DIR" && "$SHISH_SELF" -c 'cd -L link && pwd')
  assert_equal "$CD179_DIR/link" "$CD179_LOGICAL" "cd -L must keep the operand's own symlink component unresolved"

  rm -rf "$CD179_DIR"
fi

## fixes/180: field splitting had four separate bugs:
## - expand_arith() never received the caller's quoting flags, so a
##   quoted "$((...))" was still split on IFS characters that happen
##   to match a digit in its result.
## - expand_cat()'s quoted/nosplit branch reused an already-closed
##   field instead of opening a sibling, merging adjacent quoted
##   chunks of one word together.
## - parse_param.c recycled an empty N_ARGSTR placeholder node into
##   the new N_ARGPARAM node without checking whether it was quoted,
##   discarding a preceding quoted-empty string's flag (e.g. the ''
##   in "''$a"), losing it as an empty field.
## - expand_cat() failed to close a field that was empty but already
##   quoted (as opposed to a never-touched virgin placeholder), so
##   IFS whitespace between two quoted-empty fragments of one word
##   merged them into a single field instead of splitting them.
## All fixed in src/parse/parse_arith.c, src/parse/parse_param.c,
## src/expand.h, src/expand/expand_arg.c, src/expand/expand_arith.c,
## and src/expand/expand_cat.c.
IFS=' 0'
set -- "-$((708))-"
assert_equal "1" "$#" "quoted arithmetic substitution must not be field-split (count)"
assert_equal "-708-" "$1" "quoted arithmetic substitution must not be field-split (value)"
unset IFS

a='1 2'
set -- ${a+"-${a}-" "-3 4-"}
assert_equal "2" "$#" "adjacent quoted chunks inside \${a+...} must not merge (count)"
assert_equal "-1 2-" "$1" "adjacent quoted chunks inside \${a+...} must not merge (first field)"
assert_equal "-3 4-" "$2" "adjacent quoted chunks inside \${a+...} must not merge (second field)"

a=
set -- ''$a
assert_equal "1" "$#" "'' before an empty unquoted expansion must survive as an empty field"

b=' '
set -- ''$b'' ""$b""
assert_equal "4" "$#" "IFS whitespace between two quoted-empty fragments of one word must split them"

## fixes/181: mktemp with no TEMPLATE operand created its file/directory
## relative to the current directory instead of under $TMPDIR (or /tmp),
## since the "temp" flag deciding whether to prepend a base directory
## was only ever set by -t/-p, never implied by the no-template case.
if [ -n "$SHISH_SELF" ] && [ -x "$SHISH_SELF" ]; then
  MT181_DIR=$(mktemp -d)
  MT181_CWD=$(mktemp -d)

  MT181_FILE=$(cd "$MT181_CWD" && TMPDIR="$MT181_DIR" "$SHISH_SELF" -c 'mktemp')
  case "$MT181_FILE" in
    "$MT181_DIR"/*) MT181_UNDER_TMPDIR=yes ;;
    *) MT181_UNDER_TMPDIR=no ;;
  esac
  assert_equal "yes" "$MT181_UNDER_TMPDIR" "mktemp with no TEMPLATE must create under \$TMPDIR, not cwd"

  MT181_REL=$(cd "$MT181_CWD" && "$SHISH_SELF" -c 'mktemp foo.XXXXXX')
  case "$MT181_REL" in
    */*) MT181_STAYED_RELATIVE=no ;;
    *) MT181_STAYED_RELATIVE=yes ;;
  esac
  assert_equal "yes" "$MT181_STAYED_RELATIVE" "mktemp with an explicit relative TEMPLATE must stay relative to cwd"

  rm -f "$MT181_FILE" "$MT181_CWD/$MT181_REL"
  rm -rf "$MT181_DIR" "$MT181_CWD"
fi

## fixes/182 (wasm-buffer-op-signature-mismatch): lib/buffer.h's
## buffer_op_proto is a 4-arg function type, but every call site
## installing the real read()/write() (3-arg) as a buffer's op cast
## across that mismatch instead of going through a same-signature
## wrapper. Native platforms tolerate the extra argument; WebAssembly's
## call_indirect enforces an exact type match and traps. This only
## reproduces under an Emscripten/WASM build, a target not built or run
## natively by this test suite, so there's no assertion to add here --
## verified instead by building with "cfg-emscripten -DUSE_MMAP=OFF",
## serving the resulting shish.{html,js,wasm}, and confirming
## "echo hi" no longer traps with "RuntimeError: function signature
## mismatch" in-browser.

## fixes/183 (wasm-mmap-memory-access-out-of-bounds, misdiagnosed --
## turned out to have nothing to do with mmap): sh_main.c's main()
## took a non-portable 3rd "envp" parameter and walked it to import
## the environment. Emscripten's callMain() only ever calls
## _main(argc, argv) -- envp arrives as 0, and envp[c] traps with
## "RuntimeError: memory access out of bounds" on the very first
## script run, before mmap is ever touched (this is what the
## USE_MMAP=ON build's crash was actually hitting all along).
## Switched to the portable "extern char** environ" instead, which
## every libc here populates regardless of how main() was called.
## Only reproduces under Emscripten, so there's no native assertion to
## add -- verified by building with plain "cfg-emscripten" (USE_MMAP=ON,
## the default), serving the result, and confirming
## "echo hello from shish; for i in 1 2 3; do echo \"i=\$i\"; done"
## now runs and exits 0 in-browser instead of trapping.

## fixes/184 (ifs-not-reset-from-environment): a new shell must start
## with the default IFS (space/tab/newline) even if a different value
## was inherited via the environment, unless the script itself
## assigns IFS -- sh_init.c's var_import("IFS=...", V_INIT, ...) only
## sets IFS when unset, so an inherited environment value silently
## overrode the POSIX default. Fixed by dropping V_INIT for this one
## call, letting it unconditionally overwrite whatever the plain
## environ-import loop just set.
if [ -n "$SHISH_SELF" ] && [ -x "$SHISH_SELF" ]; then
  IFS184_OUT=$(IFS=X "$SHISH_SELF" -c 'printf "%d:" "${#IFS}"; IFS=,; X="a,b"; set -- $X; printf "%s\n" "$#"')
  assert_equal "3:2" "$IFS184_OUT" "a new shell must reset IFS (length 3: space/tab/newline) to the default even when IFS is inherited from the environment, and a later script-set IFS must still take effect"
fi

## fixes/185 (pipeline-compound-commands-broken): eval_tree() propagates
## E_EXIT ("this is the tail command, exec instead of forking") down
## into whatever node it dispatches -- correct for a plain simple
## command, but eval_if.c/eval_loop.c never masked it off before their
## own *internal* eval_tree() calls for the if/while test, so the
## test's nested eval_tree() saw the stale E_EXIT and exited the whole
## process right after the test ran, before the branch/body ever got a
## chance to. Only reachable when the if/while is itself forked
## directly as a pipeline stage (eval_pipeline.c forks each stage and
## hands its own eval_tree() call E_EXIT) -- wrapping it in "(...)" or
## letting another command precede it in the same "{...}" avoided the
## bug by construction, not by coincidence: eval_subshell.c/
## eval_cmdlist.c both already mask E_EXIT the same way this fixes for
## eval_if.c/eval_loop.c.
X185=$(echo "test" | if true; then cat; fi)
assert_equal "test" "$X185" "an if-statement used as the sole/last command in a pipeline stage must still run its branch, not exit after the test"

X185B=$(printf 'a\nb\nc\n' | while read -r line185; do echo "got:$line185"; done)
assert_equal "got:a
got:b
got:c" "$X185B" "a while-loop used as the sole/last command in a pipeline stage must run every iteration, not exit after the first"

## fixes/186 (fd-table-bookkeeping-vs-real-close-desync): lib/buffer's
## buffer_close() used to guard "if(b->fd > 2) close(b->fd)", silently
## no-opping any close() of fd 0/1/2 -- which papered over several
## places in src/fd*/src/fdtable* that destroy a struct's bookkeeping
## (fd_setfd(x,-1)/fd_pop()) without checking whether some *other*,
## currently-active struct has since claimed that same real fd number.
## With the guard narrowed to "fd>=0" (matching what a real close()
## should do), those desyncs became live bugs: fdstack_flatten() (run
## in a forked child right before execve()) or fd_close() would close
## a real fd that a sibling struct -- reached via a different, later
## dup()/dup2() bet landing on the same number -- was still actively
## using, so a later pipeline stage's stdin/stdout vanished out from
## under it ("Bad file descriptor", or fd 0 missing from the child's
## fd table entirely, at other times causing rev(1)/etc. to busy-loop
## reading a closed stdin instead of exiting). Root-caused via `strace
## -f` on the accumulated real fork()/dup2()/close() sequence, not the
## shell's own bookkeeping (which agreed with itself right up to the
## real close() call). Fixed in three places: fdtable_gap()'s
## FORCE-eviction branch and fdtable_dup()'s dup2-landing branch now
## *relocate* a merely-shadowed occupant via a fresh dup() instead of
## destroying it outright, and fd_close() now checks fd_list[] before
## actually close()ing rb.fd/wb.fd, neutering instead of closing when
## some other struct is already the registered owner.
X186DIR=$(mktemp -d)
mkdir "$X186DIR/real"
cat >"$X186DIR/real/mycmd" <<'EOF'
#!/bin/sh
echo real-command-ran
EOF
X186=$(echo hi | sed 1q)
assert_equal "hi" "$X186" "a pipeline running after enough prior mktemp/mkdir/heredoc fd churn must not lose its real stdin/stdout to an unrelated struct's stale close()"

## fixes/187 (subshell-fd-table-not-scoped): "(...)" subshells run
## in-process (no fork()) via fdstack_push()/setjmp()/vartab_push()/
## sh_push(), mirroring the variable stack -- but nothing analogous
## restored the *global* real-fd bookkeeping (fd_expected/fd_list[]/
## fd_top/fd_lo/fd_hi in src/fd.h) on the way back out. A persistent
## ("exec") redirection inside the subshell (e.g. swapping stdout/
## stderr) left that global state mutated after the subshell returned,
## corrupting whatever real fd resolution ran next -- reproduced as a
## build-specific (default -Os only, not under gdb/ASan/-O0) segfault.
## Fixed by adding struct fd_state + fd_state_save()/fd_state_restore()
## (src/fd/fd_state_save.c, src/fd/fd_state_restore.c) and calling them
## around eval_subshell.c's existing fdstack_push()/fdstack_pop() pair.
X187DIR=$(mktemp -d)
( exec 3>&1 1>&2 2>&3 3>&-; echo a187; echo b187 >&2 ) >"$X187DIR/out" 2>"$X187DIR/err"
assert_equal "b187" "$(cat "$X187DIR/out")" "a subshell that swaps stdout/stderr via persistent redirections must not corrupt fd bookkeeping for what follows (stdout side)"
assert_equal "a187" "$(cat "$X187DIR/err")" "a subshell that swaps stdout/stderr via persistent redirections must not corrupt fd bookkeeping for what follows (stderr side)"
rm -rf "$X187DIR"

## fixes/189 (debug-fprintf-left-in-eval_return-and-builtin_source):
## eval_return() and builtin_source() carried committed
## fprintf(stderr, "DEBUG ...") calls (plus the <stdio.h> this
## codebase otherwise avoids), so every function return and every
## dot-script return wrote four lines of pointer/flag noise to
## stderr. return-p.tst scored 0/25 purely because of it.
X189=$(f189() { return 3; }; f189 2>&1)
assert_equal "" "$X189" "returning from a function must write nothing to stderr"
X189DIR=$(mktemp -d)
printf 'return 4\n' > "$X189DIR/s189"
X189B=$(. "$X189DIR/s189" 2>&1)
assert_equal "" "$X189B" "returning from a dot-sourced script must write nothing to stderr"
rm -rf "$X189DIR"

## fixes/190 (trap-empty-arg-does-not-ignore, trap-clear-with-no-trap-
## kills-shell, trap-hup-silently-skipped, plus a use-after-free found
## fixing them): POSIX has three dispositions, shish tracked two --
## "trap '' SIG" (ignore) was handled exactly like "trap - SIG"
## (default). Clearing a trap that was never set returned 1, which for
## a *special* builtin exits a non-interactive shell. builtin_trap()'s
## "if(signum != 1)" skipped SIGHUP outright. And a trap body that
## uninstalls its own trap ("trap 'echo x; trap - INT' INT") had
## trap_uninstall() tree_free() the tree eval_tree() was still walking.
##
## fixes/191 (trap-not-dispatched-between-commands-on-one-line):
## trap_run_pending() only ran from sh_loop()/term_read()/job_wait(),
## i.e. at line and blocking-call boundaries, so a trap never fired
## between two ";"-separated commands of the same list. eval_tree() and
## eval_cmdlist() now drain pending traps per node. trap_handler()
## restores "$?" around a real-signal body (POSIX: the body sees the
## interrupted command's status, and it is restored afterwards).
##
## fixes/192 (signal-killed-child-exit-status): a child killed by a
## signal reported "$?" as 0 -- WEXITSTATUS() of a status word that
## never carried one. POSIX/bash report 128 + the signal number.
##
## All of these need a real separate shish process (a signal has to be
## delivered to a shell that is not this one), so they reuse
## $SHISH_SELF from fixes/122 above.
if [ -n "$SHISH_SELF" ] && [ -x "$SHISH_SELF" ]; then
  X190A=$("$SHISH_SELF" -c 'trap "" INT; kill -s INT $$; echo ok' 2>&1)
  assert_equal "ok" "$X190A" "trap '' SIG must ignore the signal, not reset it to the default action"

  X190B=$("$SHISH_SELF" -c 'trap - INT; echo ok' 2>&1)
  assert_equal "ok" "$X190B" "trap - SIG for a signal with no trap set is a no-op success, not an error that exits the shell"

  X190C=$("$SHISH_SELF" -c 'trap "echo t" HUP; echo ok' 2>&1)
  assert_equal "ok" "$X190C" "SIGHUP is trappable like every other signal"

  X190D=$("$SHISH_SELF" -c 'trap "echo trapped; trap - TERM" TERM
kill -s TERM $$
echo ok' 2>&1)
  assert_equal "$(printf 'trapped\nok')" "$X190D" "a trap body that uninstalls its own trap must not free the tree it is running from"

  X191=$("$SHISH_SELF" -c 'trap "echo t" TERM; kill -s TERM $$; echo after' 2>&1)
  assert_equal "$(printf 't\nafter')" "$X191" "a trap fires between two commands on the same line, not only at line boundaries"

  X191B=$("$SHISH_SELF" -c 'trap "false" TERM; kill -s TERM $$; echo $?' 2>&1)
  assert_equal "0" "$X191B" "a signal trap body's own exit status does not leak into \$? (bash prints 0 here too)"

  X192=$("$SHISH_SELF" -c '"$SHISH_SELF" -c "kill -s INT \$\$"; echo $?' 2>/dev/null)
  assert_equal "130" "$X192" "a command killed by SIGINT sets \$? to 130 (128 + 2), not 0"

  X192B=$("$SHISH_SELF" -c '"$SHISH_SELF" -c "kill -s TERM \$\$"; echo $?' 2>/dev/null)
  assert_equal "143" "$X192B" "a command killed by SIGTERM sets \$? to 143 (128 + 15)"
fi

## fixes/193 (exec-redirection-and-error-broken): fd_dup() only sets up
## a *pending* dup -- it copies pointers into the source struct and
## leaves the real dup2() to a later fdtable_dup(). For a persistent
## ("exec") redirection that is too late: the next redirection in the
## same list runs fd_new() -> fdtable_newfd() -> fd_reinit() on the
## very struct the pending dup chases via ->dup, so
##   exec >&2 2>/dev/null
## resolved fd 1 against the *new* /dev/null occupant instead of the
## original fd 2, and "reached" went to the original stdout.
## redir_dup() now resolves a persistent dup eagerly, via
## fdtable_dup(FDTABLE_FORCE | FDTABLE_CLOSE) -- but only outside a
## subshell, since "(...)" does not fork and the real dup2()/close()
## would outlive it (see TODO.md Goal 4, problem 3).
if [ -n "$SHISH_SELF" ] && [ -x "$SHISH_SELF" ]; then
  O193=$(mktemp)
  E193=$(mktemp)
  "$SHISH_SELF" <<'X193IN' >"$O193" 2>"$E193"
exec >&2 2>/dev/null
echo reached
./_no_such_command_
X193IN
  assert_equal "" "$(cat "$O193")" "exec >&2 2>/dev/null must leave nothing on the original stdout"
  assert_equal "reached" "$(cat "$E193")" "exec >&2 2>/dev/null sends later output to the stream fd 2 named *before* fd 2 was retargeted"
  rm -f "$O193" "$E193"

  ## the eager resolution must stay out of a subshell's way: a real
  ## dup2()/close() there outlives the subshell, and the fd table then
  ## claims a real fd something else holds -- which showed up as
  ## "fdtable: redirection cycle detected" from the next external
  ## command, and as a segfault deeper into a script.
  X193C=$("$SHISH_SELF" -c '( exec 3>&1 1>&2 2>&3 3>&- ; echo hi ) >/dev/null 2>&1
/bin/true
echo after' 2>&1)
  assert_equal "after" "$X193C" "a persistent redirection inside a subshell must leave the fd table usable for the next external command"
fi

## fixes/194 (cmdsubst-does-not-scope-traps-or-fd-bookkeeping): POSIX
## 2.6.3 makes command substitution a subshell environment, and
## expand_command() does run it in one -- fdstack_push(), vartab_push(),
## sh_push(), exec_functions_save() -- but it was missing the two
## process-global lists those calls do not cover: the trap list (so a
## "trap" inside "$(...)" stayed installed in the calling shell) and
## the real-kernel-fd bookkeeping fd_state_save()/fd_state_restore()
## scope for "(...)". eval_subshell() had both already; this is the
## other in-process subshell (TODO.md Goal 4, problem 2 -- the "some
## other call site has the same exposure" one).
X194=$(trap "echo T194" TERM; echo x)
assert_equal "x" "$X194" "a trap set inside \$(...) still lets the substitution produce its own output"
X194B=$(trap)
assert_equal "" "$X194B" "a trap set inside \$(...) must not stay installed in the calling shell"

## fixes/195 (cmake/Builtins.cmake) has no assertion here, deliberately:
## it is a build-configuration fix, and this file runs *inside* an
## already-built shish, so it cannot observe which builtins a different
## configure run would enable. Verified by configuring instead --
## -DCMAKE_BUILD_TYPE=Debug, -DBUILD_DEBUG=ON, -DENABLE_DUMP=ON and
## -DENABLE_ALL_BUILTINS=ON each now produce "#define BUILTIN_DUMP 1"
## in <builddir>/src/builtin_config.h (all four produced 0 before), the
## default build is byte-identical, and "dump -t" prints the fd table
## in a build configured with -DDEBUG_FDTABLE=ON.

## fixes/196 (posix-2.8.1-error-semantics): POSIX 2.8.1 lists which
## failures a non-interactive shell must not survive. shish printed a
## message (or, for a syntax error inside "eval", nothing at all) and
## carried on regardless:
##   - an expansion error ("$x" under "set -u", "${x?}")
##   - a variable assignment error ("readonly r=1; r=2")
##   - a redirection error on a special builtin ("shift <_no_such_file_")
##   - a shell syntax error in the string "eval" was given
## An interactive shell, and any of these on a plain utility, must
## still carry on -- the last case below.
X196A=$( (eval fi; echo "not reached") 2>/dev/null; echo "st=$?")
assert_equal "st=1" "$X196A" "a syntax error inside eval must end that (non-interactive) shell"
X196B=$( (eval fi) 2>&1 >/dev/null)
assert_nomatch "" "$X196B" "a syntax error inside eval must be reported"
X196C=$(eval 'echo ok'; eval '')
assert_equal "ok" "$X196C" "eval of a valid list, and of an empty one, are unaffected"
X196D=$( (set -u; echo "$NOSUCH196"; echo "not reached") 2>/dev/null; echo "st=$?")
assert_equal "st=1" "$X196D" "an unset variable under set -u must end that shell"
X196E=$( (echo "${NOSUCH196?nope}"; echo "not reached") 2>/dev/null; echo "st=$?")
assert_equal "st=1" "$X196E" "a \${x?} expansion error must end that shell"
X196F=$( (readonly RO196=1; RO196=2; echo "not reached") 2>/dev/null; echo "st=$?")
assert_equal "st=1" "$X196F" "an assignment to a readonly variable must end that shell"
X196G=$( (shift <_no_such_file_; echo "not reached") 2>/dev/null; echo "st=$?")
assert_equal "st=1" "$X196G" "a redirection error on a special builtin must end that shell"
X196H=$(echo one <_no_such_file_ 2>/dev/null; echo "reached st=$?")
assert_equal "reached st=1" "$X196H" "the same redirection error on a plain utility must not"

## fixes/196, second half: "exec <file" replaced fdtable[n] -- closing
## the descriptor the old entry owned -- and only then open()ed the
## file, so a failure cost the shell that fd for good ("exec
## <_no_such_file_" left an interactive shell with no stdin at all).
## The file is opened first now and the descriptor handed over, which
## also has to keep working when the fd is one "exec" already owns.
F196A=$(mktemp)
F196B=$(mktemp)
exec 4>"$F196A"
echo first >&4
exec 4>"$F196B"
echo second >&4
exec 4>&-
assert_equal "first" "$(cat "$F196A")" "exec 4>file writes to that file"
assert_equal "second" "$(cat "$F196B")" "a second exec on the same fd must retarget it, not lose it"
rm -f "$F196A" "$F196B"

## fixes/197 (test-binary-and-or-segfault): "test 1 -a 1" segfaulted,
## and several POSIX forms were read as the wrong thing, because the
## expression was dispatched on where an operator sat rather than on
## how many arguments there were. POSIX XCU decides by argument count
## first (1: non-null string; 2: "!" or a unary primary; 3: binary
## primary, then "!", then "( x )"; 4: "!", then "( x y )"), and only
## what that table does not cover is parsed as a "!"/-a/-o/parenthesis
## grammar -- which is also where the 3- and 4-argument "-a"/"-o"
## forms POSIX leaves unspecified are handled.
test 1 -a 1
assert_equal "0" "$?" "test 1 -a 1 must be true, not a segfault"
test "" -a 1
assert_equal "1" "$?" "test with an empty operand on the left of -a is false"
test 1 -o ""
assert_equal "0" "$?" "-o is true when either side is"
test "" -a 1 -o 1
assert_equal "0" "$?" "-a binds tighter than -o"
test ! = !
assert_equal "0" "$?" "3 arguments: a binary primary wins over a leading !"
test "(" = ")"
assert_equal "1" "$?" "3 arguments: a binary primary wins over parentheses too"
test !
assert_equal "0" "$?" "1 argument: ! is a non-null string, not an operator"
test ! "" -a ""
assert_equal "0" "$?" "4 arguments: ! negates the whole 3-argument reading"
test "(" ! a = a ")"
assert_equal "1" "$?" "parentheses group an expression"
test 5 -gt 3 -a 2 -lt 1
assert_equal "1" "$?" "a false -a operand makes the whole expression false"
X197=$(test 1 -a 2>&1; echo "st=$?")
assert_match "$X197" "*st=2*" "an incomplete expression is an error (status 2), not a crash"

## fixes/199 (CMakeLists.txt, src/term/term_complete.c,
## src/builtin/builtin_help.c): the HAVE_WINSIZE compile probe only
## included <sys/ioctl.h>, but dietlibc declares struct winsize in
## <termios.h>, so the probe failed there and the whole dietlibc
## target refused to link ("undefined reference to `term_size'").
## Fixed by including <termios.h> in the probe, and by guarding the
## two term_size.ws_col readers with #ifdef HAVE_WINSIZE (falling back
## to an assumed 80-column terminal) so a target where the probe
## legitimately stays unset -- mingw has no struct winsize at all --
## still compiles instead of failing on an incomplete-type member
## access. Per the "Writing a test" exception in CLAUDE.md for a fix
## that only compiles/runs on a platform this repo isn't being
## developed on, this is comment-only: verified instead by building
## for real with `. ./cfg.sh && cfg-diet` (previously "undefined
## reference to `term_size'", now links and runs, 152072 bytes
## stripped) and by compiling src/builtin/builtin_help.c and
## src/term/term_complete.c under `cfg-mingw64` (previously failed to
## parse `term_size.ws_col` against an incomplete struct winsize, now
## compiles clean).

## fixes/200 (src/builtin/builtin_test.c): S_ISGID, S_ISUID and
## S_ISLNK (test -g/-u/-h/-L) are not defined by mingw's headers, so a
## mingw cross build failed to compile builtin_test.c at all. Each
## primary is now guarded by #ifdef on its own macro, falling through
## to test's normal "unsupported primary" error path (status 2) on a
## platform that lacks it, the same way the file already handles
## S_ISSOCK. Per the "Writing a test" exception in CLAUDE.md for a fix
## that only compiles/runs on a platform this repo isn't being
## developed on, this is comment-only: verified by compiling
## src/builtin/builtin_test.c under `cfg-mingw64` (previously "'S_ISGID'
## undeclared" / "'S_ISUID' undeclared", now compiles clean with no
## warnings), and by confirming glibc's `tests/*.sh` and `tests/fixed.sh`
## (this file) still pass/fail exactly as they did before this change
## (`builtin-rmdir.sh` and this file's own 5 pre-existing failures,
## same as on `main`).

## fixes/201 (src/builtin/builtin_times.c): the file included
## <sys/times.h> and called times()/struct tms unconditionally, but
## mingw has neither, so a mingw cross build failed to compile it.
## The minute/second/microsecond formatting was pulled out into a
## shared print_microsecs(), which both a POSIX branch (unchanged:
## times() ticks converted via sysconf(_SC_CLK_TCK)) and a new
## WINDOWS_NATIVE branch feed -- the latter reads
## GetProcessTimes(GetCurrentProcess(), ...)'s user/kernel FILETIMEs
## (100ns units, converted straight to microseconds) and reports zero
## for the children line, since Windows has no cumulative-child-CPU-
## time equivalent without shish tracking every child itself, which it
## does not. The refactor's POSIX path is covered by the existing
## fixes/159 assertions above (still passing). Per the "Writing a
## test" exception in CLAUDE.md for the WINDOWS_NATIVE half
## specifically, that part is comment-only: verified by compiling
## src/builtin/builtin_times.c under `cfg-mingw64` (previously "fatal
## error: sys/times.h: No such file or directory", now compiles clean
## with no warnings).

## fixes/202 (cmake/Checks.cmake): lib/mmap/mmap_read.c,
## mmap_read_fd.c, mmap_unmap.c, lib/buffer/buffer_mmapread.c,
## buffer_mmapread_fd.c, buffer_munmap.c and lib/stralloc/mmap_filename.c
## already had working WINDOWS_NATIVE/_WIN32 implementations
## (CreateFileMapping/MapViewOfFile/UnmapViewOfFile), but
## HAVE_MMAP_SUPPORT was a POSIX-only probe (<sys/mman.h> + mmap() +
## munmap()), always false on mingw -- which both filtered those seven
## files out of the build in libowfat.cmake (breaking the link) *and*
## force-disabled USE_MMAP/HAVE_MMAP (silently routing every mmap
## consumer, e.g. src/fd/fd_mmap.c, through the non-mmap fallback
## instead). Not a porting gap, a probe that only asked about POSIX:
## HAVE_MMAP_SUPPORT is now also set when targeting
## WIN32/WIN64/MINGW/WINDOWS, the same platform test cmake/Checks.cmake
## already uses for socket-library detection, since Windows'
## CreateFileMapping-based code *is* this platform's mmap support, not
## an absence of it. Per the "Writing a test" exception in CLAUDE.md
## for a fix that only compiles/runs on a platform this repo isn't
## being developed on, this is comment-only: verified by reconfiguring
## and rebuilding under cfg-mingw64 (previously "undefined reference to
## `mmap_read'"/`mmap_unmap'`/`buffer_mmapread'`/`buffer_munmap'", and
## USE_MMAP force-disabled with a warning; now all seven files compile
## clean, those four symbols are gone from the link error list, and
## config.h shows HAVE_MMAP 1 with no warning), and by confirming
## tests/*.sh and tests/fixed.sh on glibc are unchanged (glibc takes
## the original HAVE_SYS_MMAN_H branch, so the new elseif is never
## reached there).

## fixes/203 (CMakeLists.txt, src/sh/sh_init.c, lib/unix/getppid.c,
## lib/unix.h): sh_init.c called getppid() unconditionally to seed
## $PPID, but mingw has no such function, so a mingw cross build
## failed to link. getppid is now probed via CMakeLists.txt's existing
## check_functions(...) call (HAVE_GETPPID, the same mechanism already
## used for sigaction/signal/etc.) instead of assumed present, and
## lib/unix/getppid.c supplies a real getppid() -- returning pid_t,
## via CreateToolhelp32Snapshot/Process32First/Process32Next, falling
## back to 0 if the snapshot lookup fails -- #if WINDOWS_NATIVE only,
## named and declared (lib/unix.h) exactly like lib/unix/readlink.c's
## own WINDOWS_NATIVE-only readlink(), so sh_init.c has one call site
## (`#if defined(HAVE_GETPPID) || WINDOWS_NATIVE` / getppid()) with no
## platform-specific function name leaking into src/. Per the "Writing
## a test" exception in CLAUDE.md for the WINDOWS_NATIVE half
## specifically, that part is comment-only: verified by rebuilding
## under cfg-mingw64 (previously "undefined reference to `getppid'",
## now compiles and links clean, getppid gone from the
## undefined-reference list, leaving only kill/killpg, tcsetpgrp and
## sig_action -- see mingw-porting.md). The HAVE_GETPPID/POSIX path is
## exercised for real below and by fixes/159's PPID-adjacent output;
## rebuilt on glibc and dietlibc, $PPID still reports the real parent
## pid on both, and tests/*.sh/tests/fixed.sh are unchanged.
X203_SELF=$(readlink "/proc/$$/exe" 2>/dev/null)

if [ -n "$X203_SELF" ] && [ -x "$X203_SELF" ]; then
  X203=$("$X203_SELF" -c 'echo $PPID')
  assert_equal "$$" "$X203" "\$PPID in a freshly spawned shish reports this script's own pid, its real parent"
fi

## fixes/204 (signal-refactor.md Phase 1 -- lib/sig/sig_table.c,
## sig_stack.c, sig.h, sig_catch.c, sig_push.c, sig_block.c,
## sig_unblock.c, sig_number.c removed, src/builtin/builtin_kill.c,
## src/exec/exec_program.c, src/job/job_fork.c, src/sh/sh_init.c):
## seven mechanical cleanups with no intended behavior change on any
## platform this repo already worked on -- populated sig_table.c's
## Windows signal-name table (previously empty, #if !WINDOWS_NATIVE
## around the whole thing, despite sig.h defining real Windows signal
## numbers one file over); introduced SHISH_NSIG (sig.h) so
## sig_stack.c bounds-checks against sig.h's own SIGHUP..SIGSYS range
## on WINDOWS_NATIVE instead of the host's NSIG (23 on mingw, vs. 31 --
## SIGURG..SIGSYS were being silently rejected); merged sig_byname()/
## sig_number() into one resolver (sig_number.c deleted, kill_signum()
## in builtin_kill.c now calls sig_byname() directly, dropping its own
## redundant "SIG"-prefix stripping and the 0-vs-EXIT disambiguation
## sig_number()'s ambiguous return forced); added explicit `return`s
## to sig_catch()/sig_push()'s excluded-platform branches (previously
## UB, an int function falling off the end); deleted four dead-code
## spots (a discarded sigemptyset() in sig_block.c, an abandoned #if 0
## block in sig_unblock.c, a commented-out sig_block(SIGINT) in
## exec_program.c, a commented-out signal(SIGTTOU/SIGTTIN) pair in
## sh_init.c); dropped redundant #if !WINDOWS_NATIVE guards around
## sig_block()/sig_unblock()/sig_blocknone() call sites in
## exec_program.c and job_fork.c (the callee already no-ops safely --
## the guard added nothing and hid the calls, e.g. setpgid/tcsetpgrp,
## that do matter); walked sigtable[] to its terminator in
## builtin_kill.c's kill_list() instead of a hardcoded 31. See
## signal-refactor.md for the full investigation and rationale.
##
## The glibc/dietlibc-visible half of this (the sig_byname merge,
## kill_list()'s rewrite) is exercised for real by this file's own
## existing fixes/153/163/190/191/192 assertions above and by
## tests/builtin-kill.sh/tests/builtin-trap.sh, all of which already
## round-trip through kill_signum()/kill_list() -- confirmed
## byte-identical pass/fail results before and after (423 passed, the
## same 5 pre-existing failures as main). Per the "Writing a test"
## exception in CLAUDE.md for the platform-specific half (the Windows
## sig_table.c population, the SHISH_NSIG mingw fix), that part is
## comment-only: verified by preprocessing lib/sig/sig_stack.c under
## cfg-mingw64 (SHISH_NSIG resolves to 32, not the host's NSIG 23) and
## by rebuilding under cfg-mingw64 (undefined-reference list unchanged
## at exactly sig_action/kill/killpg/tcsetpgrp -- sig_number and the
## old empty-table gap no longer appear, confirming neither regressed).

## fixes/205 (cmake/Checks.cmake): the two HAVE_ALLOCA_ALLOCA_H/
## HAVE_ALLOCA_MALLOC_H probes use check_run(), which calls try_run() --
## and try_run() hard-errors the whole configure ("try_run() invoked in
## cross-compiling mode") when cross-compiling without a
## CMAKE_CROSSCOMPILING_EMULATOR, which none of this project's
## toolchain files set. `cfg-msys64` hit this and failed to configure
## at all. Both check_run() calls are now skipped when
## CMAKE_CROSSCOMPILING is set, same as the SIZEOF_SSIZE_T/SIGSET_T/
## PID_T/UID_T checks just above them in the same file. Per the
## "Writing a test" exception in CLAUDE.md for a configure-time issue
## that only reproduces while cross-compiling, this is comment-only:
## verified by reconfiguring under cfg-msys64 (previously "Configuring
## incomplete, errors occurred!", now configures and builds shish.exe/
## shformat.exe clean) and cfg-mingw64 (still configures clean,
## unaffected), and by confirming the native glibc build reconfigures,
## rebuilds, and passes this same tests/fixed.sh unchanged (glibc is
## never cross-compiling, so the new guard is never taken there).

## fixes/206 (cmake/Checks.cmake): fixes/205 made the alloca probes
## skip cross-compiling, leaving HAVE_ALLOCA unset (assumed absent)
## on every cross target rather than actually testing for it. Both
## probes now use check_compile() (already used elsewhere in this
## file, e.g. HAVE_WINSIZE in CMakeLists.txt) instead of check_run():
## try_compile() alone answers "does `alloca(23)` compile", which
## needs no CMAKE_CROSSCOMPILING_EMULATOR and is a real per-target
## result instead of a blanket skip. Per the "Writing a test"
## exception in CLAUDE.md for a configure-time change that only
## differs under cross-compiling, this is comment-only: verified by
## reconfiguring under cfg-msys64 (now genuinely determines
## HAVE_ALLOCA_ALLOCA_H=TRUE instead of leaving it unset, and still
## builds shish.exe/shformat.exe clean) and cfg-mingw64 (correctly
## finds no <alloca.h> there, same HAVE_ALLOCA=FALSE result as
## before -- mingw's link failure past that point is the pre-existing,
## unrelated `mingw-missing-sig-action` BUGS entry), and by confirming
## the native glibc build's HAVE_ALLOCA_ALLOCA_H is still TRUE and a
## full `ctest` run has the identical set of failing tests with and
## without this change (same pre-existing failures, byte-for-byte).

## fixes/207 (lib/sig.h, lib/sig/sig_action.c, sig_push.c, sig_catch.c):
## sig_action()'s entire body was `#ifdef SA_RESTART`, absent on
## mingw (no sigaction/mask API at all there), so `sig_action` was
## undefined at link time -- the `mingw-missing-sig-action` BUGS entry.
## Decided, after comparing against the equivalent module in the
## sibling c-utils project (see mingw-porting.md section 3): don't
## build the signal()-based shim this was heading toward -- sig_action
## now compiles unconditionally and returns -1 honestly on
## WINDOWS_NATIVE, and sig_push()/sig_catch() (which used to
## short-circuit to `return 0` there, claiming success while doing
## nothing) now call through and let that -1 propagate for real.
## Also fixed in the same change: sig_catch.c's guard was
## `#if !(defined(_WIN32) || defined(__MSYS__))`, wrongly treating
## MSYS the same as WINDOWS_NATIVE (MSYS has a real sigaction) --
## msys64 builds were silently getting the same fake-success no-op,
## and its designated initializer's `.sa_restorer = 0` doesn't compile
## there at all (that field is Linux/glibc-specific, not standard
## POSIX). Per the "Writing a test" exception in CLAUDE.md for the
## WINDOWS_NATIVE half (sig_action's honest -1 can't be exercised on
## this dev machine's targets), that part is comment-only: verified by
## rebuilding under cfg-mingw64 (previously "undefined reference to
## `sig_action'"; now `sig_action` is gone from the link-error list
## entirely, leaving only the separate, already-tracked
## `mingw-missing-kill-killpg`/`mingw-missing-tcsetpgrp` symbols). The
## MSYS half is exercised for real: cfg-msys64 previously failed to
## *compile* sig_catch.c ("has no member named 'sa_restorer'"; that
## file was never even reached on this target before, since the old
## guard excluded it), now compiles and links shish.exe/shformat.exe
## clean, and msys64's sig_catch genuinely installs a real handler
## instead of the old no-op stub. Native glibc's sig_catch/sig_push/
## sig_action behavior is unchanged by this refactor -- confirmed via
## a full `ctest` run with the identical set of pre-existing failures
## before and after.

## fixes/208 (lib/unix/kill.c, lib/unix/killpg.c, lib/unix.h,
## lib/unix/Makefile.in, builtin_kill.c, builtin_jobs.c): neither
## kill() nor killpg() exists on mingw, so both builtin_kill.c and
## builtin_jobs.c's job_resume() were undefined at link time -- the
## `mingw-missing-kill-killpg` BUGS entry. Implemented per the
## decision table mingw-porting.md section 4 proposed: kill() maps
## SIGKILL/SIGTERM to OpenProcess+TerminateProcess, SIGINT to
## GenerateConsoleCtrlEvent (only actually fires for a real console
## process group, which shish doesn't create yet -- fails honestly for
## an ordinary pid, same as every other signal number, which gets
## ENOSYS rather than a faked success). killpg() delegates to kill()
## outright, since section 5's setpgid() is still unimplemented and
## job->pgrp is therefore never a real process group -- just the
## leading process's own pid. Per the "Writing a test" exception in
## CLAUDE.md for a fix that only compiles/links on a platform this
## repo isn't being developed on, this is comment-only: verified by
## rebuilding under cfg-mingw64 (previously "undefined reference to
## `killpg' (and `kill')"; now both symbols are gone from the
## link-error list, leaving only the separate, already-tracked
## `mingw-missing-tcsetpgrp`), and by confirming native glibc and
## msys64 (where these two new WINDOWS_NATIVE-gated files compile to
## nothing) build clean and glibc's full `ctest` run is unchanged (same
## 79 pre-existing failures before and after).

## fixes/209 (src/sh/sh_main.c, src/builtin/builtin_set.c,
## src/job/job_foreground.c): mingw's `tcsetpgrp'/`tcgetpgrp'/
## `setpgid' (`mingw-missing-tcsetpgrp`). `tcgetpgrp'/`setpgid' were
## already `#if !WINDOWS_NATIVE`-gated from earlier, untracked work;
## `job_foreground.c`'s `tcsetpgrp` gains the same guard here -- no
## longer an actual link failure once all three are gated. What was
## still live: `sh->opts.monitor` got set to 1 for any interactive session
## (`sh_main.c`) and was settable via `set -m` (`builtin_set.c`) on
## every platform including `WINDOWS_NATIVE`, so job-control
## bookkeeping gated on it (stop/resume announcements,
## `wait_pid_untraced` selection, ...) kept running even though the
## `setpgid`/`tcsetpgrp` primitives underneath it never execute there.
## Both now force `monitor` to stay 0 under `WINDOWS_NATIVE`,
## completing mingw-porting.md section 5's "compile interactive job
## control out entirely" fix at the bookkeeping layer, not just the
## syscall layer. Per the "Writing a test" exception in CLAUDE.md for
## a fix that only changes behavior on a platform this repo isn't
## being developed on, this is comment-only: verified by rebuilding
## under cfg-mingw64 -- `cfg-mingw64`/`cmake --build` now completes
## with zero undefined references (previously `tcsetpgrp`; combined
## with fixes/207/208, every symbol in mingw-porting.md's original
## nine-undefined-reference list is now resolved, shish.exe/shformat.exe
## link and run) -- and by confirming native glibc and msys64 build
## clean and glibc's full `ctest` run is unchanged (same 79
## pre-existing failures before and after; `set -m`'s glibc branch is
## untouched by the `#if WINDOWS_NATIVE` addition).

## fixes/210: cleared all 20 mingw build warnings (cfg-mingw64/
## cfg-mingw32, both now build with zero warnings). Grouped by cause:
## - "PATH_MAX redefined" (exec_path.c, history_init.c, sh_getcwd.c):
##   each unconditionally #define's PATH_MAX after mingw's own headers
##   already have; wrapped in #ifndef.
## - fd_stat.c's own `#define stat _stat`/`#define fstat _fstat`
##   shadowed <sys/stat.h>'s own _FILE_OFFSET_BITS=64-aware mapping
##   (stat/fstat -> _stat64/_fstat64) with the older, deprecated
##   32-bit-time_t variant -- a real correctness bug, not just a
##   warning (silently truncated file times/sizes on mingw). Removed;
##   mingw's own macros are already right for this project's flags.
## - lib/path/path_canonicalize.c's `struct _stat`/`_stat` function
##   pointer used mingw's *other* stat alias (_stat64i32, 32-bit time_t)
##   while `stat` itself resolves to _stat64 -- an actual type mismatch
##   this project's own `#ifndef _stat #define _stat stat #endif` shim
##   couldn't paper over once mingw started predefining `_stat` itself.
##   Switched to plain `struct stat`/`stat` (what every other stat call
##   site in the tree already uses), dropped the now-dead shim.
## - lstat()/S_IFLNK/S_ISLNK didn't exist on mingw at all (no symlink
##   bit in its <sys/stat.h>) -- builtin_chmod.c/builtin_rm.c/
##   builtin_test.c (`test -L`) all need them for real, not just to
##   silence a warning. New lib/unix/lstat.c: stat() then patches
##   st_mode to S_IFLNK if is_symlink() (already implemented via
##   reparse-point detection) says so.
## - fork()/execve() implicit-declaration and conflicting-types
##   warnings (exec_program.c, job_fork.c): both functions link fine on
##   mingw already (fork() via src/fork.c's RtlCloneUserProcess shim,
##   execve() via mingw-w64's oldnames compat layer) but had no shared,
##   correctly-typed prototype -- exec_program.c's own local `pid_t
##   fork(void)` didn't even match src/fork.c's real `int fork(void)`.
##   Centralized both in lib/unix.h (execve() via <process.h>, which
##   already declares it -- a hand-written prototype collided with its
##   dllimport attribute) alongside kill()/killpg()/getppid()/lstat().
## - usleep()/getpid() (job_wait.c, sh_forked.c, sh_init.c -- the
##   latter closes the `mingw-getpid-implicit-declaration` BUGS entry):
##   mingw's own <unistd.h> declares both; the project's `#if
##   !WINDOWS_NATIVE #include <unistd.h> #endif` guards were wider than
##   they needed to be (only the POSIX-only <termios.h>/<io.h> half of
##   those blocks actually needs the platform split).
## - buffer_frombuf.c's `b->op = &buffer_dummyreadbuf` and
##   buffer_prefetch.c/fd_close.c's raw `(void(*)())`/`(ssize_t(*)())`
##   comparisons against buffer_op_proto-typed fields: buffer_op_proto
##   hardcodes `int fd`, but WINDOWS_NATIVE's `fd_t` is `intptr_t` (a
##   real size difference, x86_64) -- cast through
##   `(buffer_op_proto*)(void*)` like every other `op` assignment
##   already does (BUFFER_INIT macros).
## - fork.c's FARPROC-to-typed-function-pointer assignments
##   (GetProcAddress() results) needed the explicit casts idiomatic
##   Win32 code always uses; its unconditional `#define _WIN32_WINNT
##   0x0600` redefined a value mingw's own headers already default to
##   0xA00 -- wrapped in #ifndef.
## - buffer_putptr.c (found separately, building under cfg-msys64):
##   `(uint64)(uintptr_t)ptr` depended on `uintptr_t` being declared;
##   this project's own typedefs.h only pulls <stdint.h> for
##   __MINGW32__/__MINGW64__, not plain MSYS. Switched to `(uint64)
##   (size_t)ptr` -- size_t is reliably pointer-width everywhere this
##   project builds, unlike this codebase's own uintptr_t availability.
## Also moved src/fork.c to lib/unix/fork.c (matching the
## one-function-per-file/self-#if-WINDOWS_NATIVE-gated convention
## already used by getppid.c/kill.c/killpg.c/lstat.c) and dropped the
## HAVE_FORK/FORK_SOURCE CMake special-casing it needed at its old
## location -- lib/*/*.c is already glob-picked-up unconditionally.
## Per the "Writing a test" exception in CLAUDE.md for changes whose
## effect is mingw/msys-build-only, this is comment-only: verified via
## a full clean rebuild of cfg-mingw64, cfg-mingw32, and cfg-msys64
## (previously 20 warnings between the two mingw targets, 1 on msys64;
## all now build with zero warnings and zero errors, shish.exe/
## shformat.exe still link), and by confirming native glibc still
## builds clean and this same `ctest` run is unchanged (same 79
## pre-existing failures before and after).

## fixes/211: lib/byte/byte_copyr.c's LINK_STATIC fallback (used
## instead of the memmove() macro on statically-linked builds) always
## copied back-to-front, which only overlap-safe for a *rightward*
## shift (out > in). A leftward shift (out < in, e.g. removing an
## argv slot by shifting the tail down) got corrupted: each element
## was overwritten by its neighbor before being read, collapsing the
## whole shifted range to a copy of its last element. Fixed by
## picking the copy direction from out vs. in, like memmove() does.
## This is exercised through the "touch" builtin's long-option
## extraction (removing "--time=WORD" from argv left-shifts every
## argument after it) rather than here, since "touch" is an opt-in
## EXTRA_BUILTIN not enabled in this file's default build -- see
## "-t/-d survive a preceding removed --time=WORD" in
## tests/builtin-touch.sh.

## fixes/212: eval_pipeline() had no non-forking execution path, so on
## a platform without a working fork() (see
## eval-pipeline-silent-on-fork-failure, now fixed) every pipeline
## silently produced no output instead of erroring or running. Added
## eval_pipeline_sequential(), used instead of the normal job_fork()
## path whenever HAVE_FORK is undefined (see cmake/Checks.cmake,
## configure.ac): each stage runs fully in-process, a non-last stage's
## stdout captured via fd_subst() and fed to the next stage's stdin
## via fd_here(), matching "$(...)"/heredoc plumbing already used
## elsewhere. This is comment-only, not a real assertion: HAVE_FORK is
## a compile-time choice (cmake/Checks.cmake forces it TRUE on every
## platform this repo is developed/tested on -- native Linux has a
## real fork(), so eval_pipeline_sequential() isn't even compiled into
## this file's shish binary), and the codebase isn't otherwise built
## against a real forkless target (Emscripten/WASI) in this test
## harness. Verified instead by configuring a throwaway native build
## with CMAKE_C_COMPILER pointed at a wrapper script whose basename
## matches cmake/Checks.cmake's Emscripten detection (forcing
## HAVE_FORK=FALSE while still linking/running as real native code),
## confirming: "echo hi | cat" and multi-stage builtin pipelines
## produce identical output to the normal fork() path, exit status
## propagates ("true | false; echo $?" -> 1), each stage's variables
## stay isolated from the caller's (POSIX 2.9.2 subshell-environment
## semantics), and "cmd1 | cmd2 &" errors loudly ("background
## pipelines are not supported without a working fork()") instead of
## silently doing nothing.

## fixes/213 (posix-signal-ignored-on-entry-can-be-trapped): POSIX
## 2.11 -- a signal already SIG_IGN when a non-interactive shell
## starts must stay ignored; "trap CMD SIG"/"trap - SIG" for it are
## silent no-ops, and a forked/exec'd child inherits the same ignore.
## sh_init() now snapshots each signal's disposition once
## (sig_snapshot()/sig_was_ignored(), lib/sig/sig_snapshot.c) before
## anything else touches one, and trap_install()/trap_uninstall()
## (src/builtin/builtin_trap.c) no-op for a signal that was already
## ignored, but only when source->mode lacks SOURCE_IACTIVE -- an
## interactive shell has no such restriction (confirmed against
## tests/posix/signal.sh's own "final_trap=ignore" rule, which fires
## exactly when parent_action=ignored and the shell is
## non-interactive). Also fixed in the same pass, since it was
## silently defeating the "-i"/"+i" distinction the *-p.tst files
## depend on to select interactive vs. non-interactive: sh_main.c's
## own "-i"/"+i" option parsing ignored the +/- prefix entirely
## ("case 'i': force_interactive = 1"), so both forced interactive
## regardless of which was given.
##
## Like fixes/105 above, "a signal was already ignored before this
## process even started" isn't expressible as a same-process
## assertion here (there is no portable, non-fragile way to spawn a
## second shish with a chosen signal pre-ignored without knowing this
## binary's own path -- $0 is this test script, not the interpreter).
## Verified instead against tests/posix, which drives exactly this via
## an external system shell pre-ignoring the signal before exec'ing
## the testee: sigint2-p/sighup2-p/sigquit2-p/sigterm2-p.tst went from
## 124/180 each to 180/180, sigurg2-p/sigcont2-p from 164-177/180 to
## 180/180, with sigint6-p/sigquit6-p/sigterm6-p/sighup6-p (the
## interactive combo, correctly unaffected by this fix) and the full
## tests/fixed.sh + ctest suite unchanged against a stashed-back
## baseline.

## fixes/214: 44 tests/posix/*.tst files (the "%REQUIRETTY%" ones --
## sigtstp/sigttin/sigttou/sigstop's *3-p/*7-p/*8-p combos, kill4-p,
## bg-p/fg-p/job-p, testtty-p, wait-p) gate themselves on
## "../checkfg" and had always been silently skipping via a
## "command not found" error, not a real "no controlling terminal"
## check: tests/checkfg.c (a tiny helper that reports whether the
## calling process is in its controlling terminal's foreground
## process group) existed in this repo's git history but had gone
## missing from the tree. Restored, and wired into CMakeLists.txt
## (built automatically whenever DO_CONFORMANCE_TESTS is on, landing
## at tests/posix/checkfg where the *.tst files' own relative
## reference expects it) so the self-skip check is real either way.
##
## Also added tests/pty-run.c, a small single-file POSIX-pty wrapper
## (posix_openpt/grantpt/unlockpt/TIOCSCTTY -- no libc convenience
## forkpty()) that gives a wrapped command a genuine controlling
## terminal and session, and a new DO_PTY_TESTS CMake option (off by
## default) that wraps just the 44 "%REQUIRETTY%" files in it instead
## of letting them self-skip.
##
## Like fixes/105/213 above, "does this process have a real
## controlling terminal" isn't expressible as a same-process assertion
## here (this script's own testee already IS the thing under test, and
## spawning a second nested pty-run'd shish from inside it to check
## its own foreground status would test pty-run's fork correctness,
## not shish). Verified instead by building with -DDO_PTY_TESTS=ON and
## running the full 44-file set directly (see BUGS:
## wait-interrupted-by-trap-hangs and BUGS:
## job-control-real-terminal-hangs-vs-kill-driven-ok for what it found,
## and TODO.md's Phase 6 for the pass/fail breakdown): checkfg reports
## "foreground" (exit 0) under pty-run and "not foreground" (exit 1)
## without it, and 10 of the 44 files now pass cleanly end to end where
## every one of them previously reported 0 cases run.

summary
