DIR=$(dirname "${0}")
. "$DIR/common.sh"

## mv builtin (src/builtin/extra/builtin_cp.c); skipped when mv is external
case $(type mv) in
*builtin*) ;;
*) echo "mv is not a builtin, skipping" 1>&2; summary ;;
esac

T=$(mktemp -d)
cd "$T" || exit 1

printf 'hello\n' >a
mv a b
assert_equal "hello 1" "$(cat b) $(test -e a; echo $?)" "rename in place"

mkdir d
mv b d
assert_equal "hello" "$(cat d/b)" "move into a directory"
printf '1\n' >x1
printf '2\n' >x2
mv x1 x2 d
assert_equal "1 2" "$(cat d/x1 d/x2 | tr '\n' ' ' | sed 's/ $//')" "several files into a directory"
mv x1 x2 nonexistent 2>/dev/null
assert_equal "1" "$?" "several sources need a directory target"

printf 'new\n' >n1
printf 'old\n' >n2
mv -n n1 n2
assert_equal "old new" "$(cat n2) $(cat n1)" "-n keeps an existing destination and the source"
printf 'y\n' | mv -i n1 n2 2>/dev/null
assert_equal "new" "$(cat n2)" "-i replaces after y"
printf 'again\n' >n1
printf 'n\n' | mv -i n1 n2 2>/dev/null
assert_equal "new again" "$(cat n2) $(cat n1)" "-i leaves both after n"
mv -f n1 n2
assert_equal "again" "$(cat n2)" "-f replaces without asking"
mv -v n2 n3 >out
assert_equal "'n2' -> 'n3'" "$(cat out)" "-v prints each move"

mv n3 n3 2>/dev/null
assert_equal "1" "$?" "a file onto itself is refused"
mv nosuch x 2>/dev/null
assert_equal "1" "$?" "a missing source fails"

mkdir -p m/sub
printf 'f\n' >m/sub/f
mv m m/sub/inside 2>/dev/null
assert_equal "1" "$?" "a directory into itself is refused"
assert_equal "f" "$(cat m/sub/f)" "a refused move leaves the tree alone"
mv m m2
assert_equal "f" "$(cat m2/sub/f)" "a directory is renamed with its contents"
mkdir e
printf 'x\n' >file
mv e file 2>/dev/null
assert_equal "1" "$?" "a directory over a file is refused"
mv file e 2>/dev/null
assert_equal "x" "$(cat e/file)" "a file into a directory of the same name works"

## across file systems: /dev/shm is a tmpfs on Linux
SHM=/dev/shm
if [ -d "$SHM" ] && [ -w "$SHM" ] && [ "$(df -P "$SHM" | tail -1 | cut -d' ' -f1)" != "$(df -P "$T" | tail -1 | cut -d' ' -f1)" ]; then
  S=$(mktemp -d "$SHM/mv.XXXXXX")
  mkdir -p "$S/tree/sub"
  printf 'data\n' >"$S/tree/sub/f"
  printf 'top\n' >"$S/tree/g"
  chmod 600 "$S/tree/g"
  ln -s g "$S/tree/l"
  printf 'one\n' >"$S/one"
  mv "$S/one" one
  assert_equal "one 1" "$(cat one) $(test -e "$S/one"; echo $?)" "a file moves to another file system and leaves the source"
  mv "$S/tree" tree2
  assert_equal "data top" "$(cat tree2/sub/f tree2/g | tr '\n' ' ' | sed 's/ $//')" "a tree moves to another file system"
  assert_equal "g" "$(readlink tree2/l)" "a symbolic link moves as a link"
  assert_equal "-rw-------" "$(ls -l tree2/g | cut -c1-10)" "modes survive the move"
  assert_equal "1" "$(test -e "$S/tree"; echo $?)" "the source tree is gone"
  assert_equal "0" "$(ls -a tree2/.. | grep -c '\.mv\.')" "no temporary copy is left behind"
  rm -rf "$S"
else
  echo "no second file system for the cross-device cases, skipping them" 1>&2
fi

mv -Z a b 2>/dev/null
assert_equal "1" "$?" "an unknown option fails"

cd / && rm -rf "$T"
summary
