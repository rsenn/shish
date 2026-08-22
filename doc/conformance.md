# Conformance

shish targets the shell language as POSIX defines it, and nothing else.
**No construct is invented here**: if it is not in POSIX, or already in
another shell, it does not go in. Anything shish accepts beyond the
standard — `local`, `source`, brace expansion, history expansion — is
spelled the way bash, ksh or dash already spell it, and is a compile-time
or run-time option rather than the default (`-B` for brace expansion,
`-H` for history expansion). A script
that runs under shish is meant to keep running under `sh`.

## Where it stands

The measurable target is yash's POSIX conformance suite, which ships in
`tests/posix` (123 files, 12195 cases).

```
cases 12195   passed 5270   failed 822   skipped 6103
```

The 6103 skips are not passes: most of them need a controlling terminal
(the `sigttin`/`sigttou`/`sigtstp` families, `testtty-p`, `wait-p`) and
are not run in a normal CI environment.

Clean files, as of this writing: `andor arith cd errexit error eval exec
for fsplit getopts grouping if kill4 nop option path ppid readonly test
until while`.

Where the remaining failures are:

| area | state |
|---|---|
| signal disposition (`sig*-p`) | 567 failures, nearly all "a signal ignored on entry must stay ignored" |
| `alias` | printing and subshell inheritance are broken |
| `read` | IFS splitting rules, `-r`, one-line-only reads |
| `kill` | `kill -s NAME` fails for most names |
| quoting / parameter expansion | backslash edge cases, some `${...}` forms |
| `command`, `unset`, `umask`, `set -o` names | option handling gaps |

`BUGS` lists every confirmed defect with a repro; `TODO.md` is the
work plan, phase by phase, with the evidence for why each item is where
it is in the queue.

## Running the suites

```sh
cd build/x86_64-linux-gnu
ctest                       # tests/*.sh + tests/posix/*.tst
ctest -R if.sh -V           # one file
```

A single conformance file, without a rebuild — the testee path must be
absolute:

```sh
sh tests/run-tst.sh "$PWD/build/x86_64-linux-gnu/shish" tests/posix exec-p.tst
```

`tests/yash` (yash's own suite, 119 more files) is registered too but off
by default; `-DDO_YASH_TESTS=ON` includes it.

### The scoreboard

Every run leaves `.trs` files behind:

```sh
cd tests/posix && for f in *.trs; do
  t=$(grep -Ec '^%%+ (PASSED|FAILED|SKIPPED):' "$f")
  x=$(grep -Ec '^%%+ FAILED:' "$f")
  [ "$x" -gt 0 ] && printf '%4d %-14s %d/%d\n' "$x" "${f%.trs}" "$((t-x))" "$t"
done | sort -rn
```

## The project's own tests

`tests/*.sh` are plain shell scripts run through the freshly built shell.
`tests/fixed.sh` is the regression file: every fix in `fixes/` has a case
there that fails without it. It is 430 assertions long and is the first
thing to run after a change.

## A note on measurement

The `sig*` files are timing-sensitive and their scores move with machine
load — the same binary has scored `sigterm1-p` 36/180 and 177/180 in
consecutive runs on a busy machine. Measure signals on an idle one, and
re-measure before concluding anything from a change.
