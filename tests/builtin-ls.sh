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

## -R: each directory under a "path:" header, names sorted, subdirectories in that order
mkdir -p rt/b/deep rt/a rt/.hid
touch rt/f rt/b/g rt/b/deep/h rt/a/i rt/.hid/j rt/.dot
X=$(ls -R rt)
assert_equal "$(printf 'rt:\na\nb\nf\n\nrt/a:\ni\n\nrt/b:\ndeep\ng\n\nrt/b/deep:\nh')" "$X" "-R lists every directory under its own header, hidden ones skipped"

X=$(ls -RA rt)
assert_equal "$(printf 'rt:\n.dot\n.hid\na\nb\nf\n\nrt/.hid:\nj\n\nrt/a:\ni\n\nrt/b:\ndeep\ng\n\nrt/b/deep:\nh')" "$X" "-RA shows and enters dotfiles and directories"

X=$(ls -Ra rt | grep -c '^\.\.$')
assert_equal "5" "$X" "-Ra shows .. in every directory it lists (not entered)"

X=$(ls -R rt | grep -c ':$')
assert_equal "4" "$X" "-R prints one header per directory"

X=$(ls -Rr rt | grep ':$' | tr '\n' ' ')
assert_equal "rt: rt/b: rt/b/deep: rt/a: " "$X" "-Rr descends in reverse order"

## a symlink to a directory is listed, not entered (without -L)
mkdir -p sl/real
touch sl/real/f
/bin/ln -s real sl/link
X=$(ls -R sl | grep -c ':$')
assert_equal "2" "$X" "-R does not enter a symlink to a directory"
X=$(ls -RL sl | grep -c 'sl/link:')
assert_equal "1" "$X" "-RL enters it"

## the long format totals per directory
X=$(ls -Rl rt | grep -c '^total ')
assert_equal "4" "$X" "-Rl prints a total for each directory"
X=$(ls -Rl rt | grep -c '^d')
assert_equal "3" "$X" "-Rl shows the three subdirectories as d"

## an unreadable subdirectory: reported, the rest is listed, status 1
mkdir -p ur/ok ur/locked
touch ur/ok/f ur/locked/g
chmod 0 ur/locked
if ! test -r ur/locked; then
  ls -R ur >/dev/null 2>&1
  assert_equal "1" "$?" "-R with an unreadable subdirectory fails"
  X=$(ls -R ur 2>/dev/null | tr '\n' ' ')
  assert_equal "ur: locked ok   ur/ok: f " "$X" "-R with an unreadable subdirectory lists the others"
  chmod 755 ur/locked
fi

## 2000 names in one directory
mkdir many
i=0
while [ $i -lt 2000 ]; do : >many/file_$i; i=$((i + 1)); done
X=$(ls many | wc -l | tr -d ' ')
assert_equal "2000" "$X" "a directory with 2000 entries lists completely"
X=$(ls -l many | wc -l | tr -d ' ')
assert_equal "2001" "$X" "and with -l (total line plus 2000 entries)"

cd /
rm -rf "$TESTDIR"

summary
