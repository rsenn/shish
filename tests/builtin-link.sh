DIR=$(dirname "${0}")
. "$DIR/common.sh"

## Testing link builtin

TESTDIR=$(mktemp -d)
cd "$TESTDIR" || exit 1

touch a

## a plain link creates a second name for the same file
link a b
assert_equal "0" "$?" "link a b succeeds when a exists and b does not"
X=$(test -f b; echo $?)
assert_equal "0" "$X" "b actually exists afterward"

## linking to an already-existing destination fails
link a b >/dev/null 2>&1
assert_equal "1" "$?" "link fails when the destination already exists"

## linking a nonexistent source fails
link nosuch c >/dev/null 2>&1
assert_equal "1" "$?" "link fails when the source does not exist"

## wrong number of operands is an error
link a >/dev/null 2>&1
assert_equal "1" "$?" "link fails with only one operand"

link a b c >/dev/null 2>&1
assert_equal "1" "$?" "link fails with three operands"

cd /
rm -rf "$TESTDIR"

summary
