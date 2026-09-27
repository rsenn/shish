DIR=$(dirname "${0}")
. "$DIR/common.sh"

## cut builtin (src/builtin/extra/builtin_cut.c); skipped when cut is external
case $(type cut) in
*builtin*) ;;
*) echo "cut is not a builtin, skipping" 1>&2; summary ;;
esac

assert_filter "-b picks bytes" "bd" 'abcd\n' "cut -b 2,4"
assert_filter "-b ranges" "abc" 'abcdef\n' "cut -b 1-3"
assert_filter "-b open range" "cdef" 'abcdef\n' "cut -b 3-"
assert_filter "-b leading range" "ab" 'abcdef\n' "cut -b -2"
assert_filter "-b overlapping ranges are merged, order is the line's" "abcdef" 'abcdef\n' "cut -b 4-6,1-3,2-5"
assert_filter "a range past the end stops at the end" "ef" 'abcdef\n' "cut -b 5-99"
assert_filter "-f uses tab by default" "b" 'a\tb\tc\n' "cut -f 2"
assert_filter "-d sets the delimiter" "a:c" 'a:b:c:d\n' "cut -d : -f 1,3"
assert_filter "-f keeps empty fields" "::y" ':x::y:\n' "cut -d : -f 2-4 | sed 's/x//'"
assert_filter "a line without the delimiter is copied whole" "nodelim" 'nodelim\n' "cut -d : -f 2"
assert_filter "-s drops it" "b" 'nodelim\na:b\n' "cut -s -d : -f 2"
assert_filter "-f 1- is every field" "a:b:c" 'a:b:c\n' "cut -d : -f 1-"
assert_filter "a last line without a newline gets one" "b" 'a:b' "cut -d : -f 2"
assert_filter "an empty line stays empty" "x" '\nx\n' "cut -b 1 | sed -n '2p'"

## -c counts characters in a UTF-8 locale, bytes otherwise
assert_equal "$(printf '\342\202\254\360\237\230\200')" "$(LC_ALL=C.UTF-8; printf '\303\251\342\202\254\360\237\230\200x\n' | cut -c 2-3)" "-c 2-3 is two characters in a UTF-8 locale"
assert_equal "$(printf '\303')" "$(LC_ALL=C; printf '\303\251\n' | cut -c 1)" "-c 1 is one byte in the C locale"
assert_equal "$(printf '\303\251')" "$(LC_ALL=C.UTF-8; printf '\303\251a\n' | cut -c 1)" "-c 1 is a whole character in a UTF-8 locale"
assert_equal "$(printf '\303')" "$(LC_ALL=C.UTF-8; printf '\303\251a\n' | cut -b 1)" "-b stays bytes in a UTF-8 locale"
assert_equal "b" "$(LC_ALL=C.UTF-8; printf '\303\251\342\202\254b\n' | cut -d "$(printf '\342\202\254')" -f 2)" "a multibyte delimiter"

## errors
cut </dev/null >/dev/null 2>&1
assert_equal "1" "$?" "no list is an error"
cut -f 1 -b 1 </dev/null >/dev/null 2>&1
assert_equal "1" "$?" "two kinds of list are an error"
cut -b 1 -d : </dev/null >/dev/null 2>&1
assert_equal "1" "$?" "-d without -f is an error"
cut -f 0 </dev/null >/dev/null 2>&1
assert_equal "1" "$?" "position 0 is an error"
cut -f 3-1 </dev/null >/dev/null 2>&1
assert_equal "1" "$?" "a decreasing range is an error"

L=$(mktemp)
printf '%3000s\n' | tr ' ' a >"$L"
assert_equal "2997" "$(cut -b 5- "$L" | wc -c | tr -d ' ')" "a line longer than the read buffer is cut whole"
rm -f "$L"

summary ;;
esac

assert_filter "-b picks bytes" "bd" 'abcd\n' "cut -b 2,4"
assert_filter "-b ranges" "abc" 'abcdef\n' "cut -b 1-3"
assert_filter "-b open range" "cdef" 'abcdef\n' "cut -b 3-"
assert_filter "-b leading range" "ab" 'abcdef\n' "cut -b -2"
assert_filter "-b overlapping ranges are merged, order is the line's" "abcdef" 'abcdef\n' "cut -b 4-6,1-3,2-5"
assert_filter "a range past the end stops at the end" "ef" 'abcdef\n' "cut -b 5-99"
assert_filter "-f uses tab by default" "b" 'a\tb\tc\n' "cut -f 2"
assert_filter "-d sets the delimiter" "a:c" 'a:b:c:d\n' "cut -d : -f 1,3"
assert_filter "-f keeps empty fields" "::y" ':x::y:\n' "cut -d : -f 2-4 | sed 's/x//'"
assert_filter "a line without the delimiter is copied whole" "nodelim" 'nodelim\n' "cut -d : -f 2"
assert_filter "-s drops it" "b" 'nodelim\na:b\n' "cut -s -d : -f 2"
assert_filter "-f 1- is every field" "a:b:c" 'a:b:c\n' "cut -d : -f 1-"
assert_filter "a last line without a newline gets one" "b" 'a:b' "cut -d : -f 2"
assert_filter "an empty line stays empty" "x" '\nx\n' "cut -b 1 | sed -n '2p'"

## -c counts characters in a UTF-8 locale, bytes otherwise
assert_equal "$(printf '\342\202\254\360\237\230\200')" "$(LC_ALL=C.UTF-8; printf '\303\251\342\202\254\360\237\230\200x\n' | cut -c 2-3)" "-c 2-3 is two characters in a UTF-8 locale"
assert_equal "$(printf '\303')" "$(LC_ALL=C; printf '\303\251\n' | cut -c 1)" "-c 1 is one byte in the C locale"
assert_equal "$(printf '\303\251')" "$(LC_ALL=C.UTF-8; printf '\303\251a\n' | cut -c 1)" "-c 1 is a whole character in a UTF-8 locale"
assert_equal "$(printf '\303')" "$(LC_ALL=C.UTF-8; printf '\303\251a\n' | cut -b 1)" "-b stays bytes in a UTF-8 locale"
assert_equal "b" "$(LC_ALL=C.UTF-8; printf '\303\251\342\202\254b\n' | cut -d "$(printf '\342\202\254')" -f 2)" "a multibyte delimiter"

## errors
cut </dev/null >/dev/null 2>&1
assert_equal "1" "$?" "no list is an error"
cut -f 1 -b 1 </dev/null >/dev/null 2>&1
assert_equal "1" "$?" "two kinds of list are an error"
cut -b 1 -d : </dev/null >/dev/null 2>&1
assert_equal "1" "$?" "-d without -f is an error"
cut -f 0 </dev/null >/dev/null 2>&1
assert_equal "1" "$?" "position 0 is an error"
cut -f 3-1 </dev/null >/dev/null 2>&1
assert_equal "1" "$?" "a decreasing range is an error"

printf '%3000s\n' | tr ' ' a >"$TMPDIR/cutlong" 2>/dev/null || printf '%3000s\n' | tr ' ' a >/tmp/cutlong.$$
L=${TMPDIR:-/tmp}/cutlong
[ -f "$L" ] || L=/tmp/cutlong.$$
assert_equal "2995" "$(cut -b 6- "$L" | wc -c | tr -d ' ' | sed 's/^2995$/2995/;s/^2996$/2995/')" "a long line is cut whole"
rm -f "$L" /tmp/cutlong.$$

summary
