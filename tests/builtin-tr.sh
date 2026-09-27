DIR=$(dirname "${0}")
. "$DIR/common.sh"

## tr builtin (src/builtin/extra/builtin_tr.c); skipped when tr is external
case $(type tr) in
*builtin*) ;;
*) echo "tr is not a builtin, skipping" 1>&2; summary ;;
esac

assert_filter "ranges translate" "HELLO" 'hello\n' "tr a-z A-Z"
assert_filter "classes translate" "HELLO" 'hello\n' "tr '[:lower:]' '[:upper:]'"
assert_filter "a short string2 repeats its last character" "xxx" 'abc\n' "tr abc x"
assert_filter "[c*] fills string2 up" "xxy" 'abc\n' "tr abc '[x*]y'"
assert_filter "[c*n] repeats n times" "xxy" 'abc\n' "tr abc '[x*2]y'"
assert_filter "-d deletes" "heo" 'hello\n' "tr -d l"
assert_filter "-d with a class" "hello" 'h1e2l3lo\n' "tr -d '[:digit:]'"
assert_filter "-s squeezes runs" "helo" 'hello\n' "tr -s l"
assert_filter "-s with string2 squeezes the translated bytes" "aXb" 'a  b\n' "tr -s ' ' X"
assert_filter "-cd keeps only string1" "abc" 'a1b2c3\n' "tr -cd 'a-c'"
assert_filter "-c translates the complement" "abXXX" 'ab12\n' "tr -c a-b X"
assert_filter "-cs makes one word per line" "$(printf 'a\nb')" 'a, b\n' "tr -cs '[:alpha:]' '\\n'"
assert_filter "-ds deletes string1, squeezes string2" "x y" 'xa  ay\n' "tr -ds a ' '"
assert_filter "octal escapes" "zzz" 'ABC\n' "tr '\\101-\\103' z"
assert_filter "\\n escape" "a b" 'a\nb' "tr '\\n' ' '"
assert_filter "[=c=] is c" "bbc" 'abc\n' "tr '[=a=]' b"
assert_filter "squeeze survives window boundaries" "ab" 'aaaaab\n' "tr -s a"

tr </dev/null >/dev/null 2>&1
assert_equal "1" "$?" "no operand is an error"
tr a </dev/null >/dev/null 2>&1
assert_equal "1" "$?" "translating needs string2"
tr -d a b </dev/null >/dev/null 2>&1
assert_equal "1" "$?" "-d takes one string"
tr z-a x </dev/null >/dev/null 2>&1
assert_equal "1" "$?" "a reversed range is an error"

L=$(mktemp)
printf '%200000s\n' | tr ' ' a >"$L"
assert_equal "200001" "$(tr a b <"$L" | wc -c | tr -d ' ')" "input larger than a window goes through whole"
assert_equal "2" "$(tr -s a <"$L" | wc -c | tr -d ' ')" "a run longer than a window still squeezes to one"
rm -f "$L"

summary
