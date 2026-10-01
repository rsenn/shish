DIR=$(dirname "${0}")
. "$DIR/common.sh"

## unlink builtin (src/builtin/extra/builtin_unlink.c); skipped when unlink is external
case $(type unlink) in
*builtin*) ;;
*) echo "unlink is not a builtin, skipping" 1>&2; summary ;;
esac

T=$(mktemp -d)
cd "$T" || exit 1

: >a
unlink a
assert_equal "0 1" "$? $(test -e a; echo $?)" "unlink removes the file"

: >b
ln b b2
unlink b
assert_equal "1 0" "$(test -e b; echo $?) $(test -e b2; echo $?)" "another hard link to the file survives"

: >target
ln -s target sym
unlink sym
assert_equal "1 0" "$(test -L sym; echo $?) $(test -e target; echo $?)" "a symbolic link is removed, not what it points to"
ln -s nowhere dangling
unlink dangling
assert_equal "0" "$?" "a dangling link can be removed"

mkdir d
unlink d 2>/dev/null
assert_equal "1 0" "$? $(test -d d; echo $?)" "a directory is refused and stays"

unlink nosuch 2>/dev/null
assert_equal "1" "$?" "a missing file fails"

: >./-x
unlink -- -x
assert_equal "0 1" "$? $(test -e ./-x; echo $?)" "-- allows a name starting with a dash"

unlink 2>/dev/null
assert_equal "1" "$?" "no operand fails"
: >p
: >q
unlink p q 2>/dev/null
assert_equal "1 0 0" "$? $(test -e p; echo $?) $(test -e q; echo $?)" "two operands fail and remove nothing"
unlink -f p 2>/dev/null
assert_equal "1 0" "$? $(test -e p; echo $?)" "there are no options"
unlink '' 2>/dev/null
assert_equal "1" "$?" "an empty name fails"

cd / && rm -rf "$T"
summary
