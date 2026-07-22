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

summary
