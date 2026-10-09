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

## -f through a link chain and a following '..'
mkdir -p d/e
ln -s d/e chain
X=$(readlink -f chain/..)
assert_equal "$TESTDIR/d" "$X" "readlink -f resolves the link before applying '..'"

ln -s selfloop selfloop
readlink -f selfloop >/dev/null 2>&1
assert_equal "1" "$?" "readlink -f on a symlink that points at itself fails"

## a target longer than the first read buffer comes back whole
L=$(printf 'x%.0s' 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 48 49 50 51 52 53 54 55 56 57 58 59 60 61 62 63 64 65 66 67 68 69 70 71 72 73 74 75 76 77 78 79 80 81 82 83 84 85 86 87 88 89 90 91 92 93 94 95 96 97 98 99 100)
ln -s "$L" longlink
X=$(readlink longlink)
assert_equal "$L" "$X" "a symlink target of 100 bytes is read in full"

cd /
rm -rf "$TESTDIR"

summary
