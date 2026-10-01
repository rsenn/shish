DIR=$(dirname "${0}")
. "$DIR/common.sh"

## tail builtin (src/builtin/filter/builtin_tail.c); skipped when tail is external
case $(type tail) in
*builtin*) ;;
*) echo "tail is not a builtin, skipping" 1>&2; summary ;;
esac

T=$(mktemp -d)
cd "$T" || exit 1

seq 1 25 >n25
printf 'a\nb\nc' >nonl
: >empty

assert_equal "16|17|18|19|20|21|22|23|24|25|" "$(tail n25 | tr '\n' '|')" "the default is 10 lines"
assert_equal "23|24|25|" "$(tail -n 3 n25 | tr '\n' '|')" "-n 3"
assert_equal "23|24|25|" "$(tail -3 n25 | tr '\n' '|')" "-3 is the old form of -n 3"
assert_equal "23|24|25|" "$(tail -n -3 n25 | tr '\n' '|')" "-n -3 is the same as -n 3"
assert_equal "" "$(tail -n 0 n25)" "-n 0 copies nothing"
assert_equal "25" "$(tail -n 100 n25 | wc -l | tr -d ' ')" "asking for more lines than there are copies everything"
assert_equal "23|24|25|" "$(tail -n +23 n25 | tr '\n' '|')" "-n +23 starts at line 23"
assert_equal "25" "$(tail -n +1 n25 | wc -l | tr -d ' ')" "-n +1 copies everything"
assert_equal "" "$(tail -n +30 n25)" "-n +N past the end copies nothing"
assert_equal "b|c" "$(tail -n 2 nonl | tr '\n' '|')" "an unterminated last line counts as a line"
assert_equal "b|c" "$(tail -n +2 nonl | tr '\n' '|')" "and stays unterminated"
assert_equal "" "$(tail empty)" "an empty file"

assert_equal "5 6 7 8 9" "$(seq 1 9 | tail -n 5 | tr '\n' ' ' | sed 's/ $//')" "a pipe"
assert_equal "24|25|" "$(tail -n 2 <n25 | tr '\n' '|')" "standard input redirected from a file"
assert_equal "4|5|" "$(seq 1 5 | tail -n +4 | tr '\n' '|')" "-n +4 on a pipe"

## bytes
assert_equal "23
24
25" "$(tail -c 9 n25)" "-c counts bytes"
assert_equal "6
7" "$(tail -c +11 n25 | head -n 2)" "-c +11 starts at byte 11"
assert_equal "" "$(tail -c 0 n25)" "-c 0 copies nothing"
assert_equal "$(wc -c <n25 | tr -d ' ')" "$(tail -c 1000 n25 | wc -c | tr -d ' ')" "-c larger than the file copies it all"
assert_equal "c" "$(tail -c 1 nonl)" "-c on an unterminated file"

## a big input does not need to be kept in full
i=0
: >big
while [ $i -lt 6000 ]; do echo "line $i"; i=$((i + 1)); done >big
assert_equal "line 5999" "$(tail -n 1 big)" "the last line of a big file"
assert_equal "1500" "$(tail -n 1500 big | wc -l | tr -d ' ')" "the last 1500 lines of a big file"
assert_equal "line 4500" "$(tail -n 1500 big | head -n 1)" "and they are the right ones"
assert_equal "100" "$(tail -c 100 big | wc -c | tr -d ' ')" "the last 100 bytes of a big file"
assert_equal "line 5990" "$(tail -n +5991 big | head -n 1)" "-n +5991 skips what comes before"

## several files
assert_equal "==> n25 <==
24
25

==> nonl <==
b
c" "$(tail -n 2 n25 nonl)" "several files get headers, apart from each other"
assert_equal "25
c" "$(tail -q -n 1 n25 nonl)" "-q drops the headers"
assert_equal "==> n25 <==
25" "$(tail -v -n 1 n25)" "-v adds one for a single file"
assert_equal "==> standard input <==
25" "$(tail -v -n 1 <n25)" "standard input is named as such"

## -f follows a growing file; timeout ends it
echo one >fl
(timeout 3 tail -f fl >fo 2>&1) &
sleep 1
echo two >>fl
sleep 1
echo three >>fl
wait
assert_equal "one|two|three|" "$(tr '\n' '|' <fo)" "-f copies what is appended"
assert_equal "x" "$(echo x | tail -f)" "-f is ignored on a pipe"
tail -f n25 nonl >/dev/null 2>&1
assert_equal "1" "$?" "-f takes one file"

## errors
tail -n x n25 >/dev/null 2>&1
assert_equal "1" "$?" "a bad line count fails"
tail nosuch >/dev/null 2>&1
assert_equal "1" "$?" "a missing file fails"
assert_equal "24|25|" "$(tail -n 2 nosuch n25 2>/dev/null | grep -v '==>' | grep . | tr '\n' '|')" "the other files are still shown"
tail -Z n25 >/dev/null 2>&1
assert_equal "1" "$?" "an unknown option fails"

cd / && rm -rf "$T"
summary
