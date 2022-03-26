DIR=$(dirname "${0}")
. "$DIR/common.sh"

set -e

## Testing backquote expansion

str=`echo BLAH`

assert_equal "$str" BLAH


