DIR=$(dirname "${0}")
. "$DIR/common.sh"

## Testing chmod builtin, -R in particular (the walk)

TESTDIR=$(mktemp -d)
cd "$TESTDIR" || exit 1

## the permission string of a path, e.g. "-rw-r--r--"; -d so a directory is not listed
perm() { ls -ld "$1" | cut -c1-10; }

mkdir -p tree/sub tree/empty
touch tree/f tree/sub/g
chmod 644 tree/f tree/sub/g
chmod 755 tree tree/sub tree/empty

chmod 600 tree/f
assert_equal "-rw-------" "$(perm tree/f)" "chmod 600 changes a single file"

chmod 700 tree
assert_equal "drwx------" "$(perm tree)" "without -R only the directory itself changes"
assert_equal "-rw-r--r--" "$(perm tree/sub/g)" "without -R nothing below changes"

chmod -R 750 tree
assert_equal "drwxr-x---" "$(perm tree)" "-R octal: the directory"
assert_equal "drwxr-x---" "$(perm tree/sub)" "-R octal: a subdirectory"
assert_equal "drwxr-x---" "$(perm tree/empty)" "-R octal: an empty subdirectory"
assert_equal "-rwxr-x---" "$(perm tree/f)" "-R octal: a file"
assert_equal "-rwxr-x---" "$(perm tree/sub/g)" "-R octal: a file in a subdirectory"

chmod -R u=rwx,g=r,o= tree/f tree/sub
assert_equal "-rwxr-----" "$(perm tree/sub/g)" "-R symbolic mode, several operands"
assert_equal "drwxr-----" "$(perm tree/sub)" "-R symbolic: a directory operand is changed too"
chmod -R u+x tree
chmod -R go-rwx tree
assert_equal "drwx------" "$(perm tree/sub)" "-R symbolic is relative to each path's own mode (dir)"
assert_equal "-rwx------" "$(perm tree/sub/g)" "-R symbolic is relative to each path's own mode (file)"

## symlinks: one met while recursing is left alone, with its referent; one named
## on the command line is dereferenced but not entered
mkdir -p lt/inner outside
touch outside/precious lt/inner/f
chmod 644 outside/precious lt/inner/f
chmod 755 outside lt lt/inner
/bin/ln -s ../outside lt/outlink
/bin/ln -s ../outside/precious lt/filelink
/bin/ln -s nowhere lt/dangling
chmod -R 700 lt
assert_equal "drwx------" "$(perm lt/inner)" "-R: real directories below are changed"
assert_equal "drwxr-xr-x" "$(perm outside)" "-R: a symlink to a directory met in the tree: referent untouched"
assert_equal "-rw-r--r--" "$(perm outside/precious)" "-R: the contents behind that symlink are untouched"
chmod -R 700 lt/filelink
assert_equal "-rwx------" "$(perm outside/precious)" "a symlink operand to a file is dereferenced"
chmod -R 700 lt/outlink
assert_equal "drwx------" "$(perm outside)" "a symlink operand to a directory: the directory itself is changed"
assert_equal "-rwx------" "$(perm outside/precious)" "a symlink operand is not entered (the file keeps its earlier mode)"
chmod 644 outside/precious
chmod -R 700 lt/outlink
assert_equal "-rw-r--r--" "$(perm outside/precious)" "-R on a symlink operand to a directory does not recurse into it"
chmod 755 outside lt

## chmod -R 700 lt/outlink/ (trailing slash) names the directory itself, which is entered
chmod 755 outside
chmod -R 711 lt/outlink/
assert_equal "-rwx--x--x" "$(perm outside/precious)" "operand with a trailing slash names the directory: it is entered"

## -v counts one line per path; -c only the ones that changed
chmod -R 700 tree
assert_equal "5" "$(chmod -Rv 700 tree | wc -l | tr -d ' ')" "-Rv: one line per path (tree, tree/f, tree/empty, tree/sub, tree/sub/g)"
assert_equal "0" "$(chmod -Rc 700 tree | wc -l | tr -d ' ')" "-Rc: nothing changed, nothing printed"
assert_equal "2" "$(chmod -Rc 755 tree/sub | wc -l | tr -d ' ')" "-Rc: two paths changed, two lines"
assert_equal "3" "$(chmod -Rv 700 lt | grep -c neither)" "-Rv: the three symlinks in the tree are reported as left alone"

## errors
chmod -R 700 nonexistent >/dev/null 2>&1
assert_equal "1" "$?" "a missing operand fails"
chmod -Rf 700 nonexistent >/dev/null 2>&1
assert_equal "1" "$?" "-f still fails for a missing operand"
assert_equal "" "$(chmod -Rf 700 nonexistent 2>&1)" "-f prints nothing"
assert_equal "1" "$(chmod -R u+bogus tree 2>&1 | wc -l | tr -d ' ')" "an invalid mode is reported once, not once per file"
chmod -R u+bogus tree >/dev/null 2>&1
assert_equal "1" "$?" "an invalid mode fails"

cd /
rm -rf "$TESTDIR"

summary
