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

summary
