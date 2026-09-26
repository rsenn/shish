DIR=$(dirname "${0}")
. "$DIR/common.sh"

## cat builtin (src/builtin/extra/builtin_cat.c); skipped when cat is external
case $(type cat) in
*builtin*) ;;
*) echo "cat is not a builtin, skipping" 1>&2; summary ;;
esac

T=$(mktemp -d)
printf 'a\nb\n\nc\n' >"$T/f1"
printf 'x\ny\n' >"$T/f2"
printf 'no newline' >"$T/f3"

assert_equal "a
b

c
x
y" "$(cat "$T/f1" "$T/f2")" "cat concatenates its operands in order"

assert_equal "    1 a
    2 b
    3 
    4 c
    5 x
    6 y" "$(cat -n "$T/f1" "$T/f2")" "-n numbers continuously across files"

assert_equal "    1 a
    2 b

    4 c" "$(cat -b "$T/f1")" "-b numbers only non-empty lines"

assert_equal "hi" "$(echo hi | cat)" "no operand reads stdin"
assert_equal "hi" "$(echo hi | cat -)" "'-' reads stdin"
assert_equal "no newline" "$(cat "$T/f3")" "a last line without newline is passed through"

X=$(cat "$T/nosuch" "$T/f2" 2>/dev/null)
assert_equal "x
y" "$X" "an unreadable operand is skipped and the rest are still printed"

cat "$T/nosuch" 2>/dev/null
assert_equal 1 "$?" "an unreadable operand makes the status 1"

cat -Z 2>/dev/null
assert_equal 1 "$?" "a bad option is an error"

assert_equal "    1 a
    2 b" "$(echo a | cat -n; echo b | cat -n | sed 's/1/2/')" "cat feeds the filter chain"

rm -rf "$T"
summary
