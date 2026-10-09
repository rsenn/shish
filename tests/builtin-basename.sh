DIR=$(dirname "${0}")
. "$DIR/common.sh"

## Testing basename builtin

X=$(basename /usr/lib)
assert_equal "lib" "$X" "a plain path strips everything up to the final '/'"

X=$(basename /usr/lib/)
assert_equal "lib" "$X" "a trailing slash on the path doesn't change the result"

X=$(basename usr)
assert_equal "usr" "$X" "a bare name with no '/' at all is returned unchanged"

X=$(basename /)
assert_equal "/" "$X" "a bare '/' is returned unchanged"

## the SUFFIX operand

X=$(basename /path/to/file.txt .txt)
assert_equal "file" "$X" "a matching SUFFIX is stripped after the directory components"

X=$(basename include/stdio.h .h)
assert_equal "stdio" "$X" "SUFFIX stripping works for single-character extensions too"

X=$(basename file.txt .c)
assert_equal "file.txt" "$X" "a non-matching SUFFIX is left alone, not stripped"

X=$(basename .txt .txt)
assert_equal ".txt" "$X" "SUFFIX is not stripped when it equals the entire resulting name"

X=$(basename /usr/lib/libc.a .a)
assert_equal "libc" "$X" "SUFFIX stripping still applies after a trailing slash on the path"

X=$(basename file.txt "")
assert_equal "file.txt" "$X" "an empty SUFFIX operand strips nothing"

## POSIX edge cases; the operand is never modified
X=$(basename //)
assert_equal "/" "$X" "an operand of only separators gives a single '/'"

X=$(basename "")
assert_equal "" "$X" "an empty operand gives an empty result"

X=$(basename a/b//)
assert_equal "b" "$X" "several trailing separators are all ignored"

P=/usr/lib/
basename "$P" >/dev/null
assert_equal "/usr/lib/" "$P" "basename does not write into its operand to strip the trailing slash"

X=$(basename 'a\b')
assert_equal 'a\b' "$X" "a backslash is an ordinary character, not a separator"

X=$(basename -s .c a/b.c/)
assert_equal "b" "$X" "a suffix is stripped after the trailing separators are"

summary
