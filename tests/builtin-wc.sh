DIR=$(dirname "${0}")
. "$DIR/common.sh"

## Testing wc builtin

TESTDIR=$(mktemp -d)
cd "$TESTDIR" || exit 1

printf 'hello world\nfoo\n' > f1

## default (no flags) prints lines, words, bytes
X=$(echo $(wc < f1))
assert_equal "2 3 16" "$X" "wc with no flags prints lines words bytes"

## -l / -w / -c / -m individually
X=$(echo $(wc -l < f1))
assert_equal "2" "$X" "-l prints the line count alone"

X=$(echo $(wc -w < f1))
assert_equal "3" "$X" "-w prints the word count alone"

X=$(echo $(wc -c < f1))
assert_equal "16" "$X" "-c prints the byte count alone"

X=$(echo $(wc -m < f1))
assert_equal "16" "$X" "-m prints the character count alone"

## -L prints the longest line's display width
X=$(echo $(wc -L < f1))
assert_equal "11" "$X" "-L prints the longest line's width"

## combined flags print in fixed lines/words/chars/bytes/maxlen order
X=$(echo $(wc -c -l < f1))
assert_equal "2 16" "$X" "combined flags print in fixed column order regardless of flag order"

## a file argument's name is echoed after the counts
X=$(echo $(wc -l f1))
assert_equal "2 f1" "$X" "a file operand's name is printed after its counts"

## multiple files add a total line
printf 'a b c d\n' > f2
LASTLINE=$(echo $(wc -l f1 f2 | tail -1))
assert_equal "3 total" "$LASTLINE" "multiple files get a trailing total line"

## a nonexistent file is an error
wc nosuch >/dev/null 2>&1
assert_equal "1" "$?" "wc fails on a nonexistent file"

cd /
rm -rf "$TESTDIR"

summary
