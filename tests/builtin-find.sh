DIR=$(dirname "${0}")
. "$DIR/common.sh"

## Testing find builtin (only meaningful when it is compiled in)

case $(command -V find 2>&1) in
  *"built-in"*) ;;
  *) summary ;;
esac

TESTDIR=$(mktemp -d)
cd "$TESTDIR" || exit 1
mkdir -p a/b c
printf 'echo sourced-a\n' >a/x.sh
printf 'echo sourced-b\n' >a/b/y.sh
printf 'hello' >a/h.txt
: >c/empty.txt
chmod 755 a/x.sh

nl='
'

## no path given: "." is the starting point, even when the first argument is -type
X=$(find -type f -maxdepth 2 | sort)
assert_equal "./a/h.txt${nl}./a/x.sh${nl}./c/empty.txt" "$X" "find -type f -maxdepth 2 without a path searches ."

X=$(find -type f -name '*.sh' | sort)
assert_equal "./a/b/y.sh${nl}./a/x.sh" "$X" "find -name without a path searches ."

X=$(find a -name y.sh)
assert_equal "a/b/y.sh" "$X" "find with a path prints paths below it"

## -exec ;  runs the command once per file, {} is the path (also inside a word)
X=$(find -type f -name '*.txt' -exec echo got {} \; | sort)
assert_equal "got ./a/h.txt${nl}got ./c/empty.txt" "$X" "find -exec runs the command for each file"

X=$(find a -name y.sh -exec echo X{}Y \;)
assert_equal "Xa/b/y.shY" "$X" "find -exec replaces {} inside a word"

## -exec can call builtins, which then run in the shell itself
X=$(find -type f -name '*.sh' -exec . {} \; | sort)
assert_equal "sourced-a${nl}sourced-b" "$X" "find -exec calls the source builtin"

find -name y.sh -exec eval 'FOUND=1' \;
assert_equal "1" "${FOUND-}" "find -exec runs a builtin in the current shell (variable stays set)"

## -exec +  runs the command once with all paths
X=$(find -type f -name '*.txt' -exec echo batch {} + | sort)
assert_equal "batch ./a/h.txt ./c/empty.txt" "$X" "find -exec ... {} + passes all paths to one command"

## -exec as a test
X=$(find -type f -exec test -s {} \; -print | sort)
assert_equal "./a/b/y.sh${nl}./a/h.txt${nl}./a/x.sh" "$X" "find -exec acts as a test on its exit status"

find -type f -exec 2>/dev/null
assert_equal "1" "$?" "find -exec without a terminator is an error"

## tests
X=$(find -type f -empty)
assert_equal "./c/empty.txt" "$X" "find -empty finds the empty file"

X=$(find -type f -size +4c | sort)
assert_equal "./a/b/y.sh${nl}./a/h.txt${nl}./a/x.sh" "$X" "find -size +4c finds files larger than 4 bytes"

X=$(find -type f -size 5c)
assert_equal "./a/h.txt" "$X" "find -size 5c matches exactly 5 bytes (and keeps matching for every file)"

X=$(find -type f -perm 755)
assert_equal "./a/x.sh" "$X" "find -perm 755 matches the mode exactly"

X=$(find -type f -perm -100)
assert_equal "./a/x.sh" "$X" "find -perm -100 matches files with the owner execute bit"

X=$(find -type f -executable)
assert_equal "./a/x.sh" "$X" "find -executable finds the executable file"

X=$(find -type f -mmin -5 | sort | head -1)
assert_equal "./a/b/y.sh" "$X" "find -mmin -5 finds files modified in the last five minutes"

X=$(find -type f -mtime +1)
assert_equal "" "$X" "find -mtime +1 finds nothing in a fresh tree"

## -regex matches the whole path (BRE by default), -iregex ignores case, -regextype selects EREs
X=$(find -regex '.*\.sh' | sort)
assert_equal "./a/b/y.sh${nl}./a/x.sh" "$X" "find -regex matches the whole path"

X=$(find -regex 'a')
assert_equal "" "$X" "find -regex is anchored: a pattern for a part of the path matches nothing"

X=$(find -regex '.*/[xy]\.sh' | sort)
assert_equal "./a/b/y.sh${nl}./a/x.sh" "$X" "find -regex understands bracket expressions"

X=$(find -iregex '.*\.TXT' | sort)
assert_equal "./a/h.txt${nl}./c/empty.txt" "$X" "find -iregex ignores case"

X=$(find -regextype posix-extended -regex '.*/(x|y)\.sh' | sort)
assert_equal "./a/b/y.sh${nl}./a/x.sh" "$X" "find -regextype posix-extended turns on EREs"

find -regex '\(' 2>/dev/null
assert_equal "1" "$?" "find -regex with an invalid expression fails"

## -prune, -depth, -not, -print0, -delete, -quit
X=$(find -type d -name c -prune -o -type f -print | sort)
assert_equal "./a/b/y.sh${nl}./a/h.txt${nl}./a/x.sh" "$X" "find -prune does not descend into the pruned directory"

X=$(find -depth -type d | head -1)
assert_equal "./a/b" "$X" "find -depth lists a directory's contents before the directory"

X=$(find -not -type d | wc -l)
assert_equal "4" "$(echo $X)" "find -not -type d counts the four files"

X=$(find -type f -name h.txt -print0 | od -c | head -1 | tr -s ' ')
assert_equal "0000000 . / a / h . t x t \\0" "$X" "find -print0 ends each path with a NUL"

X=$(find -type f -name '*.sh' -quit)
assert_equal "1" "$(echo $X | wc -w | tr -d ' ')" "find -quit stops after the first match"

find c -delete
test -e c
assert_equal "1" "$?" "find -delete removes the directory and its contents"
## symlinks: -P (default) never follows, -H follows the operands, -L follows all;
## "link/" names the directory behind the link. Nothing here may leave SYM.
SYM=$TESTDIR/sym
mkdir -p "$SYM/tree/real" "$SYM/outside/deep"
touch "$SYM/tree/real/f" "$SYM/outside/precious" "$SYM/outside/deep/p2"
/bin/ln -s ../outside "$SYM/tree/out"
/bin/ln -s real/f "$SYM/tree/filelink"
/bin/ln -s nowhere "$SYM/tree/dangling"
cd "$SYM" || exit 1

X=$(find tree | sort | tr '\n' ' ')
assert_equal "tree tree/dangling tree/filelink tree/out tree/real tree/real/f " "$X" "find -P: a symlink to a directory is listed, not entered"
X=$(find tree -type l | wc -l | tr -d ' ')
assert_equal "3" "$X" "find -P: -type l finds all three links"

X=$(find -H tree/out | sort | tr '\n' ' ')
assert_equal "tree/out tree/out/deep tree/out/deep/p2 tree/out/precious " "$X" "find -H: an operand that is a symlink is followed"
X=$(find -H tree | grep -c 'tree/out/')
assert_equal "0" "$X" "find -H: a symlink met below an operand is not followed"

X=$(find -L tree | grep -c 'tree/out/')
assert_equal "3" "$X" "find -L: symlinks below the operand are followed"
X=$(find -L tree -type f | sort | tr '\n' ' ')
assert_equal "tree/filelink tree/out/deep/p2 tree/out/precious tree/real/f " "$X" "find -L: a link to a file is a file"
X=$(find -L tree -type l | sort | tr '\n' ' ')
assert_equal "tree/dangling " "$X" "find -L: only the dangling link is still a link"

X=$(find tree/out/ | wc -l | tr -d ' ')
assert_equal "4" "$X" "find link/ (trailing slash) lists the directory behind the link"

## a loop is reported and ends; the other entries are still listed
mkdir loop
touch loop/f
/bin/ln -s . loop/self
X=$(find -L loop 2>&1 >/dev/null | grep -c 'file system loop detected')
assert_equal "1" "$X" "find -L: a symlink back to a parent directory is reported as a loop"
find -L loop >/dev/null 2>&1
assert_equal "1" "$?" "find -L: a loop makes the exit status 1"
X=$(find -L loop 2>/dev/null | grep -c 'loop/f')
assert_equal "1" "$X" "find -L: the walk goes on after a loop"

## -delete never reaches through a link, and refuses -H and -L
mkdir -p del/sub
touch del/sub/f
/bin/ln -s "$SYM/outside" del/lnk
find del -delete
assert_equal "0" "$?" "find -delete on a tree containing a symlink to a directory succeeds"
X=$(test -e del; echo $?)
assert_equal "1" "$X" "find -delete removed the tree"
X=$(ls outside/precious outside/deep/p2 2>&1 | tr '\n' ' ')
assert_equal "outside/deep/p2 outside/precious " "$X" "find -delete left what the symlink pointed at"

mkdir -p del2/sub
touch del2/sub/f
find -L del2 -delete >/dev/null 2>&1
assert_equal "1" "$?" "find -L ... -delete is refused"
find -H del2 -delete >/dev/null 2>&1
assert_equal "1" "$?" "find -H ... -delete is refused"
X=$(test -f del2/sub/f; echo $?)
assert_equal "0" "$X" "a refused -delete removed nothing"
rm -r del2
mkdir -p del3
touch del3/f
/bin/ln -s del3 del3link
find del3link/ -delete >/dev/null 2>&1
X=$(test -f del3/f; echo $?)
assert_equal "0" "$X" "find link/ -delete does not reach through the link"
rm -r del3 del3link

## depth limits, -prune and -depth order on a tree with unknown shape
mkdir -p dep/a/b/c
touch dep/a/b/c/f dep/a/f dep/top
X=$(find dep -maxdepth 1 | wc -l | tr -d ' ')
assert_equal "3" "$X" "find -maxdepth 1: the operand and its children"
X=$(find dep -mindepth 2 -maxdepth 3 | sort | tr '\n' ' ')
assert_equal "dep/a/b dep/a/b/c dep/a/f " "$X" "find -mindepth 2 -maxdepth 3"
X=$(find dep -depth | tail -1)
assert_equal "dep" "$X" "find -depth: the operand comes last"
X=$(find dep -name b -prune -o -type f -print | sort | tr '\n' ' ')
assert_equal "dep/a/f dep/top " "$X" "find -prune skips the pruned directory only"

## an unreadable directory is reported, the rest is listed
mkdir -p unr/ok unr/locked
touch unr/ok/f unr/locked/hidden
chmod 0 unr/locked
if ! test -r unr/locked; then
  find unr >/dev/null 2>&1
  assert_equal "1" "$?" "find: an unreadable directory makes the exit status 1"
  X=$(find unr 2>/dev/null | sort | tr '\n' ' ')
  assert_equal "unr unr/locked unr/ok unr/ok/f " "$X" "find: the unreadable directory is listed, its siblings too"
  X=$(find unr 2>&1 >/dev/null | wc -l | tr -d ' ')
  assert_equal "1" "$X" "find: one error line for it"
  chmod 755 unr/locked
fi
rm -rf unr

cd "$TESTDIR" || exit 1

## -empty: a directory is empty when nothing is in it (a dotfile counts); under -depth/-delete it is
## judged when its turn comes, after what was in it has gone
mkdir -p em/e1 em/e2/e3/e4 em/full em/hidden
echo x >em/full/f
echo x >em/hidden/.dot
: >em/zero
echo a >em/nz
X=$(find em -empty | sort | tr '\n' ' ')
assert_equal "em/e1 em/e2/e3/e4 em/zero " "$X" "find -empty: empty directories and files; a directory holding only a dotfile is not empty"
X=$(find em -type d -empty | wc -l | tr -d ' ')
assert_equal "2" "$X" "find -type d -empty"
X=$(find em -empty -prune -o -type f -print | sort | tr '\n' ' ')
assert_equal "em/full/f em/hidden/.dot em/nz " "$X" "find -empty -prune -o ... still sees the other files"
find em -empty -delete
assert_equal "em em/full em/full/f em/hidden em/hidden/.dot em/nz " "$(find em | sort | tr '\n' ' ')" "find -empty -delete: e4, then e3, e2 (empty once their contents are gone), e1 and the empty file"
X=$(find em/nz -empty | wc -l | tr -d ' ')
assert_equal "0" "$X" "find -empty on a non-empty file"
rm -r em

cd /
rm -rf "$TESTDIR"
summary
