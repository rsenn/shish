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

cd /
rm -rf "$TESTDIR"

summary
