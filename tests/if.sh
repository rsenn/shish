DIR=$(dirname "${0}")
. "$DIR/common.sh"

## Testing if-then-elif-else

if true; then
  RESULT="then-branch"
else
  RESULT="else-branch"
fi
assert_equal "then-branch" "$RESULT" "if true must take the then branch"

if false; then
  RESULT="then-branch"
else
  RESULT="else-branch"
fi
assert_equal "else-branch" "$RESULT" "if false must take the else branch"

if false; then
  RESULT="then-branch"
elif true; then
  RESULT="elif-branch"
else
  RESULT="else-branch"
fi
assert_equal "elif-branch" "$RESULT" "if false / elif true must take the elif branch"

if false; then
  RESULT="then-branch"
elif false; then
  RESULT="elif-branch"
else
  RESULT="else-branch"
fi
assert_equal "else-branch" "$RESULT" "if false / elif false must take the else branch"

## the if condition's own exit status/output is available, not just
## its truth value
if RESULT=$(basename /a/b/c.txt .txt); then
  :
fi
assert_equal "c" "$RESULT" "the if condition's command substitution output must be visible after the if"

## an if with no else and a false condition takes neither branch and
## the whole construct exits 0 (its own last executed command's status)
if false; then
  RESULT="then-branch"
fi
STATUS=$?
assert_equal "0" "$STATUS" "an if with no matching branch and no else must still exit 0"

summary
