DIR=$(dirname "${0}")
. "$DIR/common.sh"

## Testing backquote expansion

str=`echo BLAH`
assert_equal "BLAH" "$str" "backquote expansion must capture the command's output"

str=`echo one; echo two`
assert_equal "one
two" "$str" "backquote expansion assigned straight to a variable must not be field-split (embedded newline preserved)"

empty=`true`
assert_equal "" "$empty" "backquote expansion of a silent command must be empty"

summary
