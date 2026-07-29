DIR=$(dirname "${0}")
. "$DIR/common.sh"

## Testing mkdir builtin

TESTDIR=$(mktemp -d)
cd "$TESTDIR" || exit 1
## mktemp -d's own result isn't guaranteed absolute here -- pwd's is
TESTDIR=$(pwd)

## a plain mkdir on a single new directory still works
mkdir plaindir
assert_equal "0" "$?" "a plain mkdir on a not-yet-existing directory succeeds"
X=$(test -d plaindir; echo $?)
assert_equal "0" "$X" "the directory actually exists afterward"

## a plain mkdir with a missing parent still fails (no -p given)
mkdir noparent/child >/dev/null 2>&1
assert_equal "1" "$?" "a plain mkdir refuses to create a directory whose parent is missing"

## -p on a relative path creates every missing component
mkdir -p rel/a/b
assert_equal "0" "$?" "mkdir -p on a relative multi-component path succeeds"
X=$(test -d rel/a/b; echo $?)
assert_equal "0" "$X" "every component in the relative chain was actually created"

## -p on an absolute path also creates every missing component --
## the leading "/" must not be mistaken for an empty first component
## (BUGS: mkdir-p-absolute-path-leading-slash, fixes/91)
ABS="$TESTDIR/abs/a/b"
mkdir -p "$ABS"
assert_equal "0" "$?" "mkdir -p on an absolute multi-component path succeeds"
X=$(test -d "$ABS"; echo $?)
assert_equal "0" "$X" "every component in the absolute chain was actually created, including right after the leading /"

## -p tolerates repeated slashes between components
mkdir -p "dup//x//y"
assert_equal "0" "$?" "mkdir -p tolerates repeated slashes between components"
X=$(test -d dup/x/y; echo $?)
assert_equal "0" "$X" "the directory chain exists despite the repeated slashes in the operand"

## -p tolerates a trailing slash
mkdir -p "trail/x/"
assert_equal "0" "$?" "mkdir -p tolerates a trailing slash on the operand"
X=$(test -d trail/x; echo $?)
assert_equal "0" "$X" "the directory chain exists despite the trailing slash"

## -p on an already-fully-existing path is not an error
mkdir -p rel/a/b
assert_equal "0" "$?" "mkdir -p on an already-fully-existing path succeeds rather than erroring"

cd /
rm -rf "$TESTDIR"

summary
