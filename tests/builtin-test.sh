DIR=$(dirname "${0}")
. "$DIR/common.sh"

set -e

## Testing test builtin

#test \( \! 3 -gt 10 \) -a \( \! 10 -lt 1 \)
test "(" "!" 3 -gt 10 ")" -a "(" "!" 10 -lt 1 ")"
test '(' '!' 3 -gt 10 ')' -a '(' '!' 10 -lt 1 ')'

test '(' 3 -lt 10 -a 10 -gt 1 ')'
