DIR=$(dirname "${0}")
. "$DIR/common.sh"

## Testing expr builtin's "STRING : PATTERN" BRE match operator
## (fixes/78). Every case here was cross-checked against real GNU expr
## (both the printed result and the exit status) when this was
## written.

## classic autoconf/libtool idiom: strip a file extension
X=$(expr conftest.o : '.*\.\(.*\)')
assert_equal "o" "$X" "expr STRING : PATTERN strips a file extension via a capture group"

X=$(expr conftest.c : '.*\.\(.*\)')
assert_equal "c" "$X" "expr STRING : PATTERN works for a different extension"

## no capture group: reports the match length, not the matched text
X=$(expr hello : '.*')
assert_equal "5" "$X" "expr STRING : PATTERN with no capture group reports the match length"

X=$(expr hello : 'hel')
assert_equal "3" "$X" "expr STRING : PATTERN matches a literal prefix and reports its length"

## anchors
X=$(expr hello : '^h')
assert_equal "1" "$X" "expr STRING : PATTERN honors a leading ^ anchor"

X=$(expr abc : 'abc$')
assert_equal "3" "$X" "expr STRING : PATTERN honors a trailing \$ anchor on a full match"

( expr abcd : 'abc$' >/dev/null )
assert_equal "1" "$?" "expr STRING : PATTERN's trailing \$ anchor correctly rejects a non-full match"

## bracket expressions: ranges, negation, POSIX character classes
X=$(expr abc123 : '[a-z]*')
assert_equal "3" "$X" "expr STRING : PATTERN matches a bracket-expression range"

X=$(expr abc123 : '[a-z]*\([0-9]*\)')
assert_equal "123" "$X" "expr STRING : PATTERN combines a bracket range with a following capture group"

X=$(expr abc : '[^0-9]*')
assert_equal "3" "$X" "expr STRING : PATTERN honors bracket-expression negation (^)"

X=$(expr a : '[[:alpha:]]*')
assert_equal "1" "$X" "expr STRING : PATTERN matches a POSIX [:alpha:] character class"

## multiple capture groups: only the first is ever reported
X=$(expr abc : '\(a\)\(b\)\(c\)')
assert_equal "a" "$X" "expr STRING : PATTERN with multiple groups reports only the first"

## leading "*" is a literal, not a quantifier, per POSIX BRE
X=$(expr '*abc' : '\*abc')
assert_equal "4" "$X" "expr STRING : PATTERN's \\* still matches a literal star"

( expr abc : '*abc' >/dev/null )
assert_equal "1" "$?" "expr STRING : PATTERN: a bare leading * in the pattern is literal, so it doesn't match a string without one"

## no match: "0" (no group) or "" (has a group), matching POSIX, and
## exit status 1 either way
X=$(expr foo : 'bar')
assert_equal "0" "$X" "expr STRING : PATTERN with no match and no group reports 0"
( expr foo : 'bar' >/dev/null )
assert_equal "1" "$?" "expr STRING : PATTERN with no match exits 1"

X=$(expr abc : '[0-9]*x')
assert_equal "0" "$X" "expr STRING : PATTERN with no match (bracket expression, no group) also reports 0"

summary
