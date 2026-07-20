#!/bin/sh
# run-tst.sh: CTest wrapper around tests/{posix,yash}/run-test.sh
#
# The upstream (yash) harness needs $LINENO and alias-expansion-in-scripts,
# neither of which POSIX guarantees and neither of which dash provides;
# bash has both, just with alias expansion off by default in non-interactive
# shells, hence -O expand_aliases below.
#
# usage: run-tst.sh <path-to-testee> <test-dir> <test-file>
#   test-dir is tests/posix or tests/yash (relative to the repo root);
#   test-file is a single *.tst file's basename within that directory.
#
# Exits 0 if every test case in the file passed, 1 otherwise (or if the
# harness didn't even produce a result file, e.g. the testee crashed hard
# enough to take the harness down with it).

set -u

testee="$1"
test_dir="$2"
test_file="$3"

if ! command -v bash >/dev/null 2>&1; then
  echo "run-tst.sh: bash not found, needed to run the yash test harness (\$LINENO + aliases-in-scripts)" >&2
  exit 1
fi

# only tests/posix/ carries the harness itself; tests/yash/ shares it
harness="$(cd "$(dirname "$0")/posix" && pwd)/run-test.sh"

cd "$test_dir" || exit 1

trs="${test_file%.*}.trs"
rm -f "$trs"

bash -O expand_aliases "$harness" "$testee" "$test_file" >/dev/null 2>"${trs}.stderr"
harness_status=$?

if [ ! -f "$trs" ]; then
  echo "run-tst.sh: $test_dir/$test_file: no .trs result file produced (harness exited $harness_status)" >&2
  cat "${trs}.stderr" >&2
  rm -f "${trs}.stderr"
  exit 1
fi

total=$(grep -Ec '^%%+ (PASSED|FAILED|SKIPPED):' "$trs")
failed=$(grep -Ec '^%%+ FAILED:' "$trs")
skipped=$(grep -Ec '^%%+ SKIPPED:' "$trs")

echo "$test_file: $((total - failed - skipped))/$total passed, $skipped skipped"

if [ "$failed" -gt 0 ]; then
  grep -E '^%%+ FAILED:' "$trs" >&2
  echo "(see $test_dir/$trs for full diffs)" >&2
  exit 1
fi

exit 0
