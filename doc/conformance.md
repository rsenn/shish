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
`tests/posix` (120 files, 12261 cases).

```
cases 12261   passed 6151   failed 7   skipped 6103
```

The 6103 skips are not passes: they need a controlling terminal (the
`sigttin`/`sigttou`/`sigtstp` families, `testtty-p`, `wait-p`) and are not
run in a normal CI environment.

115 of the 120 files have no failure. The seven failing cases:

| file | failures | what |
|---|---|---|
| `alias-p` | 3 | an alias expanding to `()` after a function name; an alias ending in `\` before a newline; an alias set inside a function body and used by an earlier-parsed `$(...)` |
| `input-p` | 1 | a command substitution reads more input than it needs |
| `quote-p` | 1 | an alias expanded inside a double-quoted command substitution (dash does the same) |
| `option-p` | 1 | expects `hash` to list a `.../cat` path; `cat` is a builtin |
| `simple-p` | 1 | expects `PATH=; echo x` to report "not found"; `echo` is a builtin |

The last two are deliberate: with the utilities built in, `PATH=; mkdir -p a; cat a`
works (see [Builtins](builtins.md)). Every one of them is described in `BUGS`.

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

### What a family is failing on

```sh
grep -h -E '^%%+ FAILED' tests/posix/sig*.trs | sed -E 's/.*: SIG[A-Z]+ //; s/ \(.*//' \
  | sort | uniq -c | sort -rn
```

Do not trust a `sig*-p` number from a busy machine: the same binary scored 36/180 on `sigterm1-p`
in one run and 177/180 in the next. Measure on an idle machine.

### Tests that need a terminal

44 files carry `%REQUIRETTY%` (the `sigttin`/`sigttou`/`sigtstp`/`sigstop` combinations, `kill4-p`,
`bg-p`, `fg-p`, `job-p`, `testtty-p`, `wait-p`) and skip themselves without a controlling terminal.
`-DDO_PTY_TESTS=ON` runs them under `tests/pty-run.c`, a single-file POSIX `pty` wrapper that gives the
testee a pseudo-terminal and kills it after a 60 s alarm:

```sh
cmake -S . -B build/x86_64-linux-gnu -DDO_PTY_TESTS=ON
cmake --build build/x86_64-linux-gnu -j
cd build/x86_64-linux-gnu
NAMES=$(grep -l '%REQUIRETTY%' ../../tests/posix/*.tst \
        | xargs -n1 basename | sed 's/\.tst$//' | tr '\n' '|' | sed 's/|$//')
ctest -R "posix/(${NAMES})\.tst\$" -j4
```

## The `time` keyword

`time [-p] pipeline` is a reserved word, spelled as in bash and ksh. It times
the whole pipeline (or compound command, function or builtin after it) and
writes the report to stderr after the command; the pipeline's status is the
result, and `set -e` fires after the report.

```
$ time sleep 1             $ time -p sleep 1
                           real 1.00
real	0m1.001s           user 0.00
user	0m0.000s           sys 0.00
sys	0m0.000s
```

- It is recognised only at the start of a pipeline, before or after `!`.
  `a | time b`, `command time`, `\time` and `"time"` run `/usr/bin/time`.
- `TIMEFORMAT` is not supported. A lone `time` is a syntax error.

## Where shish is stricter than bash and dash

- `pwd` takes no operands (POSIX gives it none): `pwd a` is a usage error,
  status 1. bash and dash ignore the operand.
- `break` and `continue` do not cross a function or subshell boundary, as in
  bash, and a loop's `break` after an inner loop's own `break` reaches the
  enclosing loop.

## The project's own tests

`tests/*.sh` are plain shell scripts run through the freshly built shell.
`tests/fixed.sh` is the regression file: every fix in `fixes/` has a case
there that fails without it. It is 750 assertions long and is the first
thing to run after a change.

## A note on measurement

The `sig*` files are timing-sensitive and their scores move with machine
load — the same binary has scored `sigterm1-p` 36/180 and 177/180 in
consecutive runs on a busy machine. Measure signals on an idle one, and
re-measure before concluding anything from a change.
