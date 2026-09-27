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

## -m counts characters when the locale variables name UTF-8, else bytes
## (LC_ALL wins over LC_CTYPE over LANG, an empty one is skipped)
printf '\303\251\342\202\254\360\237\230\200\n' > u8

X=$(LC_ALL=C.UTF-8; echo $(wc -m < u8))
assert_equal "4" "$X" "-m counts each UTF-8 sequence once when LC_ALL names UTF-8"

X=$(LC_ALL=C.UTF-8; echo $(wc -c < u8))
assert_equal "10" "$X" "-c still counts bytes in a UTF-8 locale"

X=$(LC_ALL=C; echo $(wc -m < u8))
assert_equal "10" "$X" "-m counts bytes in the C locale"

X=$(LC_ALL=; LC_CTYPE=; LANG=en_US.UTF-8; echo $(wc -m < u8))
assert_equal "4" "$X" "empty LC_ALL and LC_CTYPE fall through to LANG"

X=$(LC_ALL=C; LANG=en_US.UTF-8; echo $(wc -m < u8))
assert_equal "10" "$X" "LC_ALL=C overrides a UTF-8 LANG"

X=$(LC_ALL=C.UTF-8; echo $(wc -L < u8))
assert_equal "3" "$X" "-L is in characters in a UTF-8 locale"

printf 'a\377b\303' > bad
X=$(LC_ALL=C.UTF-8; echo $(wc -m < bad))
assert_equal "4" "$X" "an invalid byte and a cut-off sequence count as one character each"

## a character straddling the 4 KiB read window is still one character
awk 'BEGIN { for(i = 0; i < 5000; i++) printf "\303\251"; printf "\n" }' > big 2>/dev/null || {
  i=0; : > big; while [ $i -lt 500 ]; do printf '\303\251\303\251\303\251\303\251\303\251\303\251\303\251\303\251\303\251\303\251' >> big; i=$((i+1)); done; echo >> big
}
X=$(LC_ALL=C.UTF-8; echo $(wc -m < big))
assert_equal "5001" "$X" "characters across read windows are counted whole"

X=$(LC_ALL=C.UTF-8; cat big | wc -m | tr -d ' ')
assert_equal "5001" "$X" "the same through a pipe"

cd /
rm -rf "$TESTDIR"

summary
