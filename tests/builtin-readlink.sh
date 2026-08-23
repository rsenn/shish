DIR=$(dirname "${0}")
. "$DIR/common.sh"

## Testing readlink builtin

TESTDIR=$(mktemp -d)
cd "$TESTDIR" || exit 1
## mktemp -d's own result isn't guaranteed absolute here -- pwd's is
TESTDIR=$(pwd)

touch afile
ln -s afile alink

## a symlink's target is printed
X=$(readlink alink)
assert_equal "afile" "$X" "readlink prints a symlink's immediate target"

## a plain file is not a symlink -- an error
readlink afile >/dev/null 2>&1
assert_equal "1" "$?" "readlink fails on a non-symlink"

## a nonexistent path is an error
readlink nosuch >/dev/null 2>&1
assert_equal "1" "$?" "readlink fails on a nonexistent path"

## missing operand is an error
readlink >/dev/null 2>&1
assert_equal "1" "$?" "readlink requires an operand"

## -f canonicalizes through a symlink to an absolute path
X=$(readlink -f alink)
assert_equal "$TESTDIR/afile" "$X" "readlink -f canonicalizes a symlink to its absolute target"

## -f tolerates a missing final component
X=$(readlink -f nosuch)
assert_equal "$TESTDIR/nosuch" "$X" "readlink -f tolerates a missing final component"

## -f still fails when a non-final component is missing
readlink -f nosuchdir/nope >/dev/null 2>&1
assert_equal "1" "$?" "readlink -f fails when a non-final component is missing"

## -e requires the final component to exist too
readlink -e nosuch >/dev/null 2>&1
assert_equal "1" "$?" "readlink -e fails when the final component is missing"

X=$(readlink -e afile)
assert_equal "$TESTDIR/afile" "$X" "readlink -e succeeds when the path fully exists"

## -m tolerates every component being missing
X=$(readlink -m nosuchdir/nope)
assert_equal "$TESTDIR/nosuchdir/nope" "$X" "readlink -m tolerates missing components anywhere"

cd /
rm -rf "$TESTDIR"

summary
