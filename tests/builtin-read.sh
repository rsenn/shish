DIR=$(dirname "${0}")
. "$DIR/common.sh"

## Testing read builtin

TMPFILE=$(mktemp)

## basic single-word read
printf 'hello\n' >"$TMPFILE"
read -r WORD <"$TMPFILE"
assert_equal "hello" "$WORD" "read must capture a single-word line"

## field splitting across multiple variables, remainder goes to the last one
printf 'dev mnt type opt1,opt2 extra field\n' >"$TMPFILE"
read -r DEV MNT TYPE OPTS REST <"$TMPFILE"
assert_equal "dev" "$DEV" "read splits into the 1st variable"
assert_equal "mnt" "$MNT" "read splits into the 2nd variable"
assert_equal "type" "$TYPE" "read splits into the 3rd variable"
assert_equal "opt1,opt2" "$OPTS" "read splits into the 4th variable"
assert_equal "extra field" "$REST" "read puts every remaining field into the last variable"

## -r (raw) must not interpret backslash escapes
printf 'line with \\\\n backslash\n' >"$TMPFILE"
read -r RAWLINE <"$TMPFILE"
assert_equal 'line with \\n backslash' "$RAWLINE" "read -r must leave backslashes untouched"

## -n N reads at most N characters, ignoring further input
printf 'ABCDEFGHIJKLMNOPQRSTUVWXYZ\n' >"$TMPFILE"
read -r -n 10 PARTIAL <"$TMPFILE"
assert_equal "ABCDEFGHIJ" "$PARTIAL" "read -n N must stop after N characters"

## -d sets a custom delimiter instead of newline
printf 'first;second;third' >"$TMPFILE"
read -r -d ';' FIRST <"$TMPFILE"
assert_equal "first" "$FIRST" "read -d must split on the given delimiter instead of newline"

## reading two separate lines in sequence from the same fd
printf 'one\ntwo\n' >"$TMPFILE"
exec 9<"$TMPFILE"
read -r -u 9 LINE1
read -r -u 9 LINE2
exec 9<&-
assert_equal "one" "$LINE1" "read -u must read the 1st line from the given fd"
assert_equal "two" "$LINE2" "read -u must read the 2nd line from the same fd on the next call"

rm -f "$TMPFILE"

summary
