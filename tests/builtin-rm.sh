DIR=$(dirname "${0}")
. "$DIR/common.sh"

## Testing rm builtin

TESTDIR=$(mktemp -d)
cd "$TESTDIR" || exit 1

## a plain (non-recursive) rm still removes a single file
touch plainfile
rm plainfile
assert_equal "0" "$?" "a plain rm on a single existing file succeeds"
X=$(test -e plainfile; echo $?)
assert_equal "1" "$X" "the file is actually gone afterward"

## a plain (non-recursive) rm on a directory fails, no -r given
mkdir plaindir
rm plaindir >/dev/null 2>&1
assert_equal "1" "$?" "a plain rm refuses to remove a directory without -r"
rmdir plaindir

## symlinks below are made via /bin/ln, not the "ln" builtin -- the
## builtin unconditionally appends a trailing "/" to a single-file
## destination (BUGS: ln-trailing-slash-on-plain-destination), making
## every "ln -s target name" here fail with ENOTDIR/ENOENT. Unrelated
## to rm; worked around here rather than fixed, to keep this file
## testing only rm.

## -r removes a populated directory tree: files, a nested subdirectory,
## and a dangling symlink inside it
mkdir -p tree/sub
touch tree/f1 tree/sub/f2
/bin/ln -s /this/does/not/exist tree/dangling
rm -r tree
assert_equal "0" "$?" "rm -r on a populated directory tree succeeds"
X=$(test -e tree; echo $?)
assert_equal "1" "$X" "the whole tree is gone afterward, including the top-level directory itself"

## -r on a symlink *to* a directory removes only the symlink, not the
## directory it points at -- matching every other shell's rm
mkdir realdir
touch realdir/keepme
/bin/ln -s realdir linktodir
rm -r linktodir
assert_equal "0" "$?" "rm -r on a symlink-to-directory succeeds"
X=$(test -L linktodir; echo $?)
assert_equal "1" "$X" "the symlink itself is gone"
X=$(test -f realdir/keepme; echo $?)
assert_equal "0" "$X" "the real directory's contents survive -- rm -r must not follow the symlink into it"
rm -r realdir

## -rf on a missing path is not an error
rm -rf this-was-never-created
assert_equal "0" "$?" "rm -rf on a nonexistent path succeeds silently"

## -r (no -f) on a missing path is an error
rm -r this-was-never-created >/dev/null 2>&1
assert_equal "1" "$?" "rm -r without -f on a nonexistent path fails"

## -rv reports each removed entry
mkdir -p vtree/sub
touch vtree/sub/f
X=$(rm -rv vtree | grep -c "removed")
assert_equal "3" "$X" "rm -rv reports one \"removed\" line per file and per directory removed (f, sub/, vtree/ itself)"

## -R is accepted as a synonym for -r
mkdir -p Rtree/sub
touch Rtree/sub/f
rm -R Rtree
assert_equal "0" "$?" "rm -R (capital) works the same as rm -r"

cd /
rm -rf "$TESTDIR"

summary
