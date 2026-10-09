DIR=$(dirname "${0}")
. "$DIR/common.sh"

## Testing realpath builtin

TESTDIR=$(mktemp -d)
cd "$TESTDIR" || exit 1
## mktemp -d's own result isn't guaranteed absolute here -- pwd's is
TESTDIR=$(pwd)

mkdir sub
touch afile
ln -s afile alink

## a relative path resolves to an absolute one, collapsing "." / ".."
X=$(realpath ./sub/../afile)
assert_equal "$TESTDIR/afile" "$X" "realpath resolves a relative path to an absolute, collapsed one"

## a symlink is resolved to what it points at by default
X=$(realpath alink)
assert_equal "$TESTDIR/afile" "$X" "realpath resolves a symlink to its target by default"

## -s leaves symlinks unresolved
X=$(realpath -s alink)
assert_equal "$TESTDIR/alink" "$X" "realpath -s does not resolve symlinks"

## -L is a synonym for -s
X=$(realpath -L alink)
assert_equal "$TESTDIR/alink" "$X" "realpath -L does not resolve symlinks"

## -P (the default) resolves symlinks explicitly
X=$(realpath -P alink)
assert_equal "$TESTDIR/afile" "$X" "realpath -P resolves symlinks"

## a nonexistent path still resolves (no existence requirement)
X=$(realpath nosuch)
assert_equal "$TESTDIR/nosuch" "$X" "realpath resolves a nonexistent path without erroring"

## --relative-to=DIR prints the result relative to DIR
X=$(realpath --relative-to="$TESTDIR" sub/../afile)
assert_equal "afile" "$X" "realpath --relative-to=DIR prints a path relative to DIR"

## --relative-to DIR (separate argument) works the same way
X=$(realpath --relative-to "$TESTDIR/sub" afile)
assert_equal "../afile" "$X" "realpath --relative-to DIR (space form) resolves correctly"

## relative-to the same path yields "."
X=$(realpath --relative-to="$TESTDIR/afile" afile)
assert_equal "." "$X" "realpath --relative-to its own resolved path yields '.'"

## missing operand is an error
realpath >/dev/null 2>&1
assert_equal "1" "$?" "realpath requires an operand"

## lexical edge cases: ".." at the root, repeated separators, empty operand
X=$(realpath /..)
assert_equal "/" "$X" "'..' above the root stays at the root"

X=$(realpath /a/../../b)
assert_equal "/b" "$X" "'..' that would climb above the root is dropped"

X=$(realpath //tmp///)
assert_equal "/tmp" "$X" "repeated and trailing separators collapse"

X=$(realpath "")
assert_equal "$TESTDIR" "$X" "an empty operand resolves to the current directory"

X=$(realpath .)
assert_equal "$TESTDIR" "$X" "'.' resolves to the current directory without a trailing '/.'"

## physical mode resolves a symlink before a following '..'
mkdir -p real/inner
ln -s real/inner deep
X=$(realpath deep/..)
assert_equal "$TESTDIR/real" "$X" "realpath deep/.. goes to the parent of the link's target"

X=$(realpath -s deep/..)
assert_equal "$TESTDIR" "$X" "realpath -s applies '..' lexically, without looking at the link"

## a relative link component followed by '..' above the start
ln -s ../.. real/inner/up
X=$(realpath deep/up/nosuch)
assert_equal "$TESTDIR/nosuch" "$X" "a relative link that climbs is resolved from its own directory"

## a link cycle is an error, not an endless loop
ln -s loop1 loop2
ln -s loop2 loop1
realpath loop1 >/dev/null 2>&1
assert_equal "1" "$?" "a symlink cycle makes realpath fail"

cd /
rm -rf "$TESTDIR"

summary
