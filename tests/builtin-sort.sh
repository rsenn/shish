DIR=$(dirname "${0}")
. "$DIR/common.sh"

## sort builtin (src/builtin/filter/builtin_sort.c); skipped when sort is external
case $(type sort) in
*builtin*) ;;
*) echo "sort is not a builtin, skipping" 1>&2; summary ;;
esac

T=$(mktemp -d)
cd "$T" || exit 1

printf 'banana\napple\ncherry\nApple\nbanana\n10\n9\n100\n-5\n 3\n' >a
printf 'b 2 x\na 10 y\nc 1 z\na 9 w\nb 2 a\n' >f
printf 'x:3:b\ny:10:a\nz:2:c\nx:3:a\n' >c
printf '1.5\n1.25\n-2\n0\n-0\n.5\n10\n' >num

assert_equal " 3|-5|10|100|9|Apple|apple|banana|banana|cherry|" "$(sort a | tr '\n' '|')" "bytes in the C order"
assert_equal "cherry|banana|banana|apple|Apple|9|100|10|-5| 3|" "$(sort -r a | tr '\n' '|')" "-r reverses"
assert_equal " 3|-5|10|100|9|Apple|apple|banana|cherry|" "$(sort -u a | tr '\n' '|')" "-u drops equal lines"
assert_equal " 3|-5|10|100|9|Apple|apple|banana|banana|cherry|" "$(sort -f a | tr '\n' '|')" "-f folds case (ties fall back to the bytes)"
assert_equal " 3|-5|10|100|9|apple|banana|cherry|" "$(sort -fu a | tr '\n' '|')" "-fu keeps one of Apple and apple"
assert_equal "-5|Apple|apple|banana|banana|cherry| 3|9|10|100|" "$(sort -n a | tr '\n' '|')" "-n sorts by value, not-numbers as zero after the negatives"
assert_equal "-2|-0|0|.5|1.25|1.5|10|" "$(sort -n num | tr '\n' '|')" "-n handles signs, fractions and signed zeros"
assert_equal "-2|0|.5|1.25|1.5|10|" "$(sort -nu num | tr '\n' '|')" "-nu treats -0 and 0 as equal"

## keys
assert_equal "c 1 z|a 10 y|b 2 a|b 2 x|a 9 w|" "$(sort -k2 f | tr '\n' '|')" "-k2 compares from field 2 to the end of the line"
assert_equal "c 1 z|b 2 a|b 2 x|a 9 w|a 10 y|" "$(sort -k2n f | tr '\n' '|')" "-k2n numeric"
assert_equal "a 10 y|a 9 w|b 2 a|b 2 x|c 1 z|" "$(sort -k1,1 -k2,2nr f | tr '\n' '|')" "a second key breaks ties of the first"
assert_equal "z:2:c|x:3:a|x:3:b|y:10:a|" "$(sort -t: -k2n -k3 c | tr '\n' '|')" "-t: separates fields"
assert_equal "c 1 z|a 10 y|b 2 x|a 9 w|b 2 a|" "$(sort -k3,3r f | tr '\n' '|')" "-k3,3r reverses just that key"
assert_equal "a 10 y|a 9 w|b 2 a|b 2 x|c 1 z|" "$(sort -k1.1,1.1 f | tr '\n' '|')" "a character range inside a field, ties by the whole line"
assert_equal "c 1 z|a 10 y|b 2 a|b 2 x|a 9 w|" "$(sort -b -k2 f | tr '\n' '|')" "-b ignores leading blanks"
assert_equal "a 10 y|b 2 x|c 1 z|" "$(sort -u -k1,1 f | tr '\n' '|')" "-u compares the keys only and keeps the first of equal ones"

## -c, -m, -o
sort -c f 2>/dev/null
assert_equal "1" "$?" "-c fails on unsorted input"
sort f >fs
sort -c fs
assert_equal "0" "$?" "-c accepts sorted input"
printf 'a\na\n' >dup
sort -c dup
assert_equal "0" "$?" "-c accepts equal neighbours"
sort -cu dup 2>/dev/null
assert_equal "1" "$?" "-cu rejects equal neighbours"
sort -c f fs 2>/dev/null
assert_equal "2" "$?" "-c takes one file"
assert_equal "$(sort f f)" "$(sort -m fs fs)" "-m merges"
sort -o f f
assert_equal "$(cat fs)" "$(cat f)" "-o may name an input file"
assert_equal "" "$(sort -o out fs; cat out | head -0)" "-o writes the file, not standard output"
assert_equal "5" "$(wc -l <out | tr -d ' ')" "-o output has the lines"

## input
assert_equal "b|c|" "$(printf 'c\nb' | sort | tr '\n' '|')" "an unterminated last line is kept"
assert_equal "x|y|" "$(printf 'y\nx\n' | sort - | tr '\n' '|')" "- is standard input"
assert_equal "" "$(: | sort)" "empty input gives empty output"
i=0
: >big
while [ $i -lt 3000 ]; do echo $(( (i * 7919) % 3001 )); i=$((i + 1)); done >big
sort -n big >bigs
assert_equal "3000" "$(wc -l <bigs | tr -d ' ')" "a large input keeps every line"
sort -c -n bigs
assert_equal "0" "$?" "a large sorted output checks out"
assert_equal "$(sort -n big | tail -n 1)" "$(sort -nr big | head -n 1)" "-nr is -n reversed"

## errors
sort nosuch >/dev/null 2>&1
assert_equal "2" "$?" "a missing file is status 2"
sort -k0 a >/dev/null 2>&1
assert_equal "2" "$?" "a bad key is status 2"
sort -t ab a >/dev/null 2>&1
assert_equal "2" "$?" "a multi-character -t is status 2"
sort -Z a >/dev/null 2>&1
assert_equal "2" "$?" "an unknown option is status 2"

cd / && rm -rf "$T"
summary
