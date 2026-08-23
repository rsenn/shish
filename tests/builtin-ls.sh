DIR=$(dirname "${0}")
. "$DIR/common.sh"

## Testing ls builtin

TESTDIR=$(mktemp -d)
cd "$TESTDIR" || exit 1

touch afile
mkdir adir
touch .hidden

## default listing is sorted, one per line, dotfiles hidden
X=$(ls)
assert_equal "$(printf 'adir\nafile')" "$X" "default listing shows non-dotfiles sorted, skipping . and .."

## -a includes dotfiles
X=$(ls -a | grep -c '^\.hidden$')
assert_equal "1" "$X" "-a includes dotfiles"

X=$(ls | grep -c '^\.hidden$')
assert_equal "0" "$X" "without -a dotfiles are omitted"

## -d lists the directory itself, not its contents
X=$(ls -d adir)
assert_equal "adir" "$X" "-d on a directory prints just its own name"

## a missing path is an error
ls /no/such/path >/dev/null 2>&1
assert_equal "1" "$?" "listing a nonexistent path fails"

## -l shows a permissions column for a regular file
X=$(ls -l afile | cut -c1)
assert_equal "-" "$X" "-l on a regular file starts the mode string with '-'"

X=$(ls -ld adir | cut -c1)
assert_equal "d" "$X" "-l -d on a directory starts the mode string with 'd'"

cd /
rm -rf "$TESTDIR"

summary
