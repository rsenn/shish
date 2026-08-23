DIR=$(dirname "${0}")
. "$DIR/common.sh"

## Testing tee builtin

TESTDIR=$(mktemp -d)
cd "$TESTDIR" || exit 1

## tee copies stdin to stdout and to the given file
X=$(printf 'line1\nline2\n' | tee out1)
assert_equal "$(printf 'line1\nline2')" "$X" "tee copies stdin through to stdout"
assert_equal "$(printf 'line1\nline2')" "$(cat out1)" "tee writes the same data to its file argument"

## tee writes to multiple files at once
printf 'both\n' | tee out2 out3 >/dev/null
assert_equal "both" "$(cat out2)" "tee writes to the first of multiple files"
assert_equal "both" "$(cat out3)" "tee writes to the second of multiple files"

## without -a, an existing file is overwritten, not appended to
printf 'second\n' | tee out1 >/dev/null
assert_equal "second" "$(cat out1)" "tee without -a overwrites an existing file"

## -a appends instead of overwriting
printf 'first\n' > out4
printf 'second\n' | tee -a out4 >/dev/null
assert_equal "$(printf 'first\nsecond')" "$(cat out4)" "tee -a appends to an existing file"

## a file that can't be opened is reported but doesn't stop the copy
X=$(printf 'data\n' | tee /nonexistent/dir/file 2>/dev/null)
assert_equal "data" "$X" "tee still copies to stdout when a file argument fails to open"
RET=$(printf 'data\n' | tee /nonexistent/dir/file >/dev/null 2>&1; echo $?)
assert_equal "1" "$RET" "tee reports failure when a file argument fails to open"

cd /
rm -rf "$TESTDIR"

summary
