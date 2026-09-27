DIR=$(dirname "${0}")
. "$DIR/common.sh"

## head builtin (src/builtin/extra/builtin_head.c); skipped when head is external
case $(type head) in
*builtin*) ;;
*) echo "head is not a builtin, skipping" 1>&2; summary ;;
esac

T=$(mktemp -d)
printf '1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n12\n' >"$T/n12"
printf 'a\nb\n' >"$T/ab"

assert_equal "10" "$(head "$T/n12" | wc -l | tr -d ' ')" "the default is 10 lines"
assert_equal "1 2 3" "$(head -n 3 "$T/n12" | tr '\n' ' ' | sed 's/ $//')" "-n 3 copies three lines"
assert_equal "1 2" "$(head -2 "$T/n12" | tr '\n' ' ' | sed 's/ $//')" "-2 is the old form of -n 2"
assert_equal "0" "$(head -n 0 "$T/n12" | wc -c | tr -d ' ')" "-n 0 copies nothing"
assert_equal "12" "$(head -n 99 "$T/n12" | wc -l | tr -d ' ')" "asking for more lines than there are copies the file"
assert_equal "1
2" "$(head -c 4 "$T/n12")" "-c counts bytes"

assert_filter "head -n 2 on a pipe" "1
2" '1\n2\n3\n4\n' "head -n 2"
assert_filter "head keeps a last line without a newline" "a
b" 'a\nb' "head -n 5"
assert_filter "head -c 3" "a
b" 'a\nb\nc\n' "head -c 3"

## more than one file: headers, a blank line between, -q and -v
assert_equal "==> $T/ab <==
a

==> $T/n12 <==
1" "$(head -n 1 "$T/ab" "$T/n12")" "several files get headers, apart from each other"
assert_equal "a
1" "$(head -q -n 1 "$T/ab" "$T/n12")" "-q drops the headers"
assert_equal "==> $T/ab <==
a" "$(head -v -n 1 "$T/ab")" "-v adds one for a single file"
assert_equal "==> standard input <==
a" "$(head -v -n 1 <"$T/ab")" "standard input is named as such"

## the rest of the input is left for whoever reads next
assert_equal "1 2 3 4" "$( (head -n 2; cat) <"$T/n12" | head -n 4 | tr '\n' ' ' | sed 's/ $//')" "head stops reading where it stops copying"

## errors
head -n x "$T/ab" >/dev/null 2>&1
assert_equal "1" "$?" "a bad line count fails"
head "$T/nonexistent" >/dev/null 2>&1
assert_equal "1" "$?" "a missing file fails"

## a line longer than the read buffer is one line
printf '%3000s\nz\n' | tr ' ' a >"$T/long"
assert_equal "3001" "$(head -n 1 "$T/long" | wc -c | tr -d ' ')" "a long line is copied whole"

rm -rf "$T"
summary
