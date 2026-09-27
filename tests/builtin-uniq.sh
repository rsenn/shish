DIR=$(dirname "${0}")
. "$DIR/common.sh"

## uniq builtin (src/builtin/extra/builtin_uniq.c); skipped when uniq is external
case $(type uniq) in
*builtin*) ;;
*) echo "uniq is not a builtin, skipping" 1>&2; summary ;;
esac

T=$(mktemp -d)
printf 'a\na\nb\nc\nc\nc\nd\n' >"$T/u1"

assert_filter "uniq collapses adjacent repeats" "a
b
c
d" 'a\na\nb\nc\nc\nc\nd\n' "uniq"
assert_filter "uniq does not merge lines that are apart" "a
b
a" 'a\nb\na\n' "uniq"
assert_filter "uniq -c counts" "      2 a
      1 b" 'a\na\nb\n' "uniq -c"
assert_filter "uniq -d keeps only repeated lines" "a
c" 'a\na\nb\nc\nc\nd\n' "uniq -d"
assert_filter "uniq -u keeps only single lines" "b
d" 'a\na\nb\nc\nc\nd\n' "uniq -u"
assert_filter "uniq -f 1 skips a field" "x 1 foo
z 2 foo" 'x 1 foo\ny 1 foo\nz 2 foo\n' "uniq -f 1"
assert_filter "uniq -s 1 skips a character" "aXb
cYb" 'aXb\nbXb\ncYb\n' "uniq -s 1"
assert_filter "uniq adds the missing newline of a last line" "a" 'a\na' "uniq"

## characters, not bytes, in a UTF-8 locale
assert_equal "2" "$(LC_ALL=C; printf '\303\251a\nxa\n' | uniq -s 1 | wc -l | tr -d ' ')" "-s counts bytes in the C locale"
assert_equal "1" "$(LC_ALL=C.UTF-8; printf '\303\251a\nxa\n' | uniq -s 1 | wc -l | tr -d ' ')" "-s counts characters in a UTF-8 locale"

## the output_file operand
uniq "$T/u1" "$T/out"
assert_equal "a b c d" "$(tr '\n' ' ' <"$T/out" | sed 's/ $//')" "a second operand is the output file"
uniq "$T/u1" "$T/out" extra >/dev/null 2>&1
assert_equal "1" "$?" "a third operand is an error"
uniq -f x "$T/u1" >/dev/null 2>&1
assert_equal "1" "$?" "a bad field count fails"

printf '%3000s\n%3000s\n' x x | tr ' ' a >"$T/long"
assert_equal "1" "$(uniq "$T/long" | wc -l | tr -d ' ')" "long lines compare whole"

rm -rf "$T"
summary
