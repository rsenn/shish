DIR=$(dirname "${0}")
. "$DIR/common.sh"

## Testing rmdir builtin

TESTDIR=$(mktemp -d)
cd "$TESTDIR" || exit 1

## a plain rmdir on a single empty directory still works
mkdir plaindir
rmdir plaindir
assert_equal "0" "$?" "a plain rmdir on an empty directory succeeds"
X=$(test -e plaindir; echo $?)
assert_equal "1" "$X" "the directory is actually gone afterward"

## a plain rmdir on a non-empty directory fails
mkdir nonempty
touch nonempty/f
rmdir nonempty >/dev/null 2>&1
assert_equal "1" "$?" "a plain rmdir refuses to remove a non-empty directory"
rm -r nonempty

## -p removes a directory and every now-empty ancestor above it
mkdir -p a/b/c
rmdir -p a/b/c
assert_equal "0" "$?" "rmdir -p on a/b/c succeeds"
X=$(test -e a; echo $?)
assert_equal "1" "$X" "a/b/c, a/b, and a are all gone -- the whole chain was climbed"

## -p stops climbing (but still reports success for what it did remove)
## once it hits an ancestor that isn't empty
mkdir -p x/y
touch x/keepme
rmdir -p x/y >/dev/null 2>&1
assert_equal "1" "$?" "rmdir -p reports failure once an ancestor isn't empty"
X=$(test -d x; echo $?)
assert_equal "0" "$X" "the non-empty ancestor itself survives"
X=$(test -e x/y; echo $?)
assert_equal "1" "$X" "the originally-named (now-empty) directory was still removed before the climb stopped"
rm -r x

## -p tolerates a trailing slash on the operand the same as no slash
mkdir -p p/q
rmdir -p p/q/
assert_equal "0" "$?" "rmdir -p accepts a trailing slash on the operand"
X=$(test -e p; echo $?)
assert_equal "1" "$X" "the trailing slash doesn't stop the ancestor climb from working"

## -p on a single, slash-free component just removes that one
## directory (no ancestor to climb to)
mkdir onlyme
rmdir -p onlyme
assert_equal "0" "$?" "rmdir -p on a bare, slash-free name behaves like a plain rmdir"
X=$(test -e onlyme; echo $?)
assert_equal "1" "$X" "and it actually removed it"

## -pv reports every directory removed while climbing, not just the
## originally-named one
mkdir -p v1/v2
X=$(rmdir -pv v1/v2 | grep -c "removed")
assert_equal "2" "$X" "rmdir -pv reports one line for the named directory and one per ancestor climbed"

cd /
rm -rf "$TESTDIR"

summary
