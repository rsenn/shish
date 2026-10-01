DIR=$(dirname "${0}")
. "$DIR/common.sh"

## cp builtin (src/builtin/extra/builtin_cp.c); skipped when cp is external
case $(type cp) in
*builtin*) ;;
*) echo "cp is not a builtin, skipping" 1>&2; summary ;;
esac

T=$(mktemp -d)
cd "$T" || exit 1
mode() { ls -ld "$1" | cut -c1-10; }

printf 'hello\n' >a
chmod 640 a
cp a b
assert_equal "hello" "$(cat b)" "file to file copies the content"
assert_equal "-rw-r-----" "$(mode b)" "a new file gets the mode of the original"

printf 'old and longer\n' >c
ln c c.hard
cp a c
assert_equal "hello" "$(cat c)" "an existing destination is truncated and rewritten"
assert_equal "hello" "$(cat c.hard)" "an existing destination keeps its inode"

mkdir d
cp a b d
assert_equal "a b" "$(ls d | tr '\n' ' ' | sed 's/ $//')" "several files go into a directory"
cp a d/new
assert_equal "hello" "$(cat d/new)" "file to a name inside a directory"
cp -v a d/v >out 2>&1
assert_equal "'a' -> 'd/v'" "$(cat out)" "-v prints each copy"

cp a b nonexistent 2>/dev/null
assert_equal "1" "$?" "several sources need a directory target"
cp nosuch d 2>/dev/null
assert_equal "1" "$?" "a missing source fails"
rm -f d/b
cp nosuch b d 2>/dev/null
assert_equal "1 hello" "$? $(cat d/b)" "the other sources are still copied after an error"

cp a a 2>/dev/null
assert_equal "1" "$?" "a file onto itself is refused"
ln a a.hard
cp a a.hard 2>/dev/null
assert_equal "1" "$?" "a file onto a hard link of itself is refused"
assert_equal "hello" "$(cat a)" "a refused copy leaves the file alone"

cp d x 2>/dev/null
assert_equal "1" "$?" "a directory needs -R"

## -R: trees, symlinks, empty directories, modes
mkdir -p t/sub/deep t/empty
printf 'f\n' >t/sub/deep/f
printf 'g\n' >t/g
chmod 600 t/g
ln -s g t/link
ln -s nowhere t/dangle
cp -R t u
assert_equal "f" "$(cat u/sub/deep/f)" "-R copies nested directories"
assert_equal "1" "$(ls -d u/empty | wc -l | tr -d ' ')" "-R copies an empty directory"
assert_equal "-rw-------" "$(mode u/g)" "-R keeps file modes"
assert_equal "g" "$(readlink u/link)" "-R copies a symbolic link as a link"
assert_equal "nowhere" "$(readlink u/dangle)" "-R copies a dangling link as a link"
cp -r t u2
assert_equal "f" "$(cat u2/sub/deep/f)" "-r is the same as -R"
cp -R t u/
assert_equal "f" "$(cat u/t/sub/deep/f)" "-R into an existing directory puts the tree inside"
cp -R t/ w
assert_equal "g" "$(cat w/g)" "a trailing slash on the source does not matter"
cp -RL t l 2>/dev/null
assert_equal "g" "$(cat l/link)" "-L copies what a link points to"
cp -R t t/sub/inside 2>/dev/null
assert_equal "1" "$?" "-R refuses to copy a directory into itself"

mkdir ro
printf 'x\n' >ro/f
cp -a t arch
assert_equal "g" "$(readlink arch/link)" "-a copies links as links"

## -p keeps the times
touch -t 200102030405 a
cp -p a p
cp a np
assert_equal "1" "$(test p -nt np && echo 0 || echo 1)" "-p keeps the modification time (copy is older than a plain copy)"
assert_equal "1" "$(find p -newer np | wc -l | tr -d ' ' | sed 's/^1$/0/;s/^0$/1/')" "-p copy is not newer than a plain one"

## -i -n -f
printf 'one\n' >n1
printf 'two\n' >n2
cp -n n1 n2
assert_equal "two" "$(cat n2)" "-n keeps an existing destination"
cp -n n1 n2
assert_equal "0" "$?" "-n skipping is not an error"
printf 'y\n' | cp -i n1 n2 2>/dev/null
assert_equal "one" "$(cat n2)" "-i copies after y"
printf 'two\n' >n2
printf 'n\n' | cp -i n1 n2 2>/dev/null
assert_equal "two" "$(cat n2)" "-i leaves the file after n"
cp -i n1 n2 2>/dev/null </dev/null
assert_equal "0 two" "$? $(cat n2)" "-i at end of input answers no, without an error"
chmod 444 n2
cp n1 n2 2>/dev/null
cp -f n1 n2 2>/dev/null
assert_equal "one" "$(cat n2)" "-f replaces an unwritable destination"
chmod 644 n2

## names
printf 'dash\n' >./-x
cp -- ./-x dash 2>/dev/null
assert_equal "dash" "$(cat dash)" "a name starting with - after --"
printf 's\n' >'sp ace'
cp 'sp ace' 'sp ace2'
assert_equal "s" "$(cat 'sp ace2')" "names with spaces"
cp '' x 2>/dev/null
assert_equal "1" "$?" "an empty operand fails"
cp -Z a b 2>/dev/null
assert_equal "1" "$?" "an unknown option fails"
cp a 2>/dev/null
assert_equal "1" "$?" "a missing destination fails"

## sizes
: >zero
cp zero zero2
assert_equal "0" "$(wc -c <zero2 | tr -d ' ')" "an empty file"
i=0
: >big
while [ $i -lt 6000 ]; do echo "line $i xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx" >>big; i=$((i + 1)); done
cp big big2
assert_equal "$(wc -c <big | tr -d ' ')" "$(wc -c <big2 | tr -d ' ')" "a file larger than the copy buffer keeps its size"
assert_equal "$(tail -n 1 big)" "$(tail -n 1 big2)" "a file larger than the copy buffer keeps its end"

cd / && rm -rf "$T"
summary
