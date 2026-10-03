DIR=$(dirname "${0}")
. "$DIR/common.sh"

## split builtin (src/builtin/core/builtin_split.c); skipped when split is external
case $(type split) in
*builtin*) ;;
*) echo "split is not a builtin, skipping" 1>&2; summary ;;
esac

T=$(mktemp -d)
cd "$T" || exit 1
mkdir w
cd w || exit 1

seq 1 2500 >../s2500
printf 'a\nb\nc' >../nonl

split ../s2500
assert_equal "xaa xab xac" "$(ls | tr '\n' ' ' | sed 's/ $//')" "by default 1000 lines go into each of xaa, xab, ..."
assert_equal "1000 1000 500" "$(wc -l <xaa | tr -d ' ') $(wc -l <xab | tr -d ' ') $(wc -l <xac | tr -d ' ')" "the last piece holds the rest"
assert_equal "$(cat ../s2500)" "$(cat xaa xab xac)" "the pieces put together are the input"
rm -f x*

split -l 700 -a 3 ../s2500 out_
assert_equal "out_aaa out_aab out_aac out_aad" "$(ls | tr '\n' ' ' | sed 's/ $//')" "-a 3 and a name prefix"
assert_equal "700 700 700 400" "$(wc -l <out_aaa | tr -d ' ') $(wc -l <out_aab | tr -d ' ') $(wc -l <out_aac | tr -d ' ') $(wc -l <out_aad | tr -d ' ')" "-l 700"
rm -f out_*

split -b 100 ../s2500
assert_equal "114" "$(ls | wc -l | tr -d ' ')" "-b 100 makes 114 pieces of a 10893 byte file"
assert_equal "100" "$(wc -c <xaa | tr -d ' ')" "a piece is 100 bytes"
assert_equal "xej" "$(ls | tail -n 1)" "the last name is xej (base 26)"
assert_equal "$(cat ../s2500)" "$(cat x*)" "the byte pieces put together are the input"
rm -f x*

split -b 1k ../s2500
assert_equal "1024" "$(wc -c <xaa | tr -d ' ')" "-b 1k is 1024 bytes"
rm -f x*
split -b 1m ../s2500
assert_equal "1" "$(ls | wc -l | tr -d ' ')" "-b 1m of a small file is one piece"
rm -f x*

## the last line without a newline goes into the last piece
split -l 2 ../nonl
assert_equal "a
b" "$(cat xaa)" "the first piece"
assert_equal "c" "$(cat xab)" "the unterminated line is the last piece"
rm -f x*

## standard input, '-', and the exact size
seq 1 5 | split -l 2
assert_equal "1 2 3 4 5" "$(cat xaa xab xac | tr '\n' ' ' | sed 's/ $//')" "standard input"
rm -f x*
seq 1 4 | split -l 2 - p
assert_equal "paa pab" "$(ls | tr '\n' ' ' | sed 's/ $//')" "- is standard input; no empty piece after an exact fit"
rm -f p*
: | split
assert_equal "0" "$(ls | wc -l | tr -d ' ')" "empty input creates no file"

## the suffixes run out
seq 1 30 | split -a 1 -l 1 >/dev/null 2>&1
assert_equal "1 26" "$? $(ls | wc -l | tr -d ' ')" "26 one-letter pieces are made, then it fails"
rm -f x*

## errors
split -l 0 ../nonl >/dev/null 2>&1
assert_equal "1" "$?" "a zero line count fails"
split -b 12x ../nonl >/dev/null 2>&1
assert_equal "1" "$?" "a bad byte count fails"
split nosuch >/dev/null 2>&1
assert_equal "1" "$?" "a missing file fails"
split ../nonl p extra >/dev/null 2>&1
assert_equal "1" "$?" "an extra operand fails"
split -a 300 ../nonl >/dev/null 2>&1
assert_equal "1 0" "$? $(ls | wc -l | tr -d ' ')" "a suffix too long for a file name creates nothing"

cd / && rm -rf "$T"
summary
