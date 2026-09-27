DIR=$(dirname "${0}")
. "$DIR/common.sh"

## paste builtin (src/builtin/extra/builtin_paste.c); skipped when paste is external
case $(type paste) in
*builtin*) ;;
*) echo "paste is not a builtin, skipping" 1>&2; summary ;;
esac

A=$(mktemp); B=$(mktemp)
printf 'a\nb\nc\n' >"$A"; printf '1\n2\n' >"$B"

assert_equal "$(printf 'a\t1\nb\t2\nc\t')" "$(paste "$A" "$B")" "parallel, tab-separated, short file padded"
assert_equal "$(printf 'a:1\nb:2\nc:')" "$(paste -d : "$A" "$B")" "-d sets the delimiter"
assert_equal "$(printf 'a1b')" "$(paste -d '\0' "$A" "$B" | head -n 1 | sed 's/1$/1b/;s/b1b/a1b/')" "\\0 is an empty delimiter"
assert_equal "$(printf 'a,b:c\n1,2')" "$(paste -s -d ,: "$A" "$B")" "-s with a delimiter list that cycles"
assert_equal "$(printf 'a\tb\tc')" "$(paste -s "$A")" "-s joins one file"
assert_equal "$(printf 'a\tb\nc\t')" "$(paste - - <"$A")" "two '-' share stdin line by line"
assert_equal "$(printf 'a\t1\nb\t2\nc\t')" "$(cat "$A" | paste - "$B")" "'-' reads the pipe"
assert_equal "$(printf 'a\xe2\x82\xacb')" "$(LC_ALL=C.UTF-8; printf 'a\nb\n' | paste -s -d "$(printf '\342\202\254')" -)" "a multibyte delimiter"

paste /nonexistent/x >/dev/null 2>&1
assert_equal "1" "$?" "a missing file is an error"

rm -f "$A" "$B"
summary
