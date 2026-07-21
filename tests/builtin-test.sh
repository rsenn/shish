DIR=$(dirname "${0}")
. "$DIR/common.sh"

## Testing test builtin
##
## Each check invokes "test" with literal arguments (not forwarded
## through "$@") to avoid a separate, already-tracked bug where a
## quoted "$@" containing an empty positional parameter is expanded
## wrong (see BUGS).

## numeric comparisons and parenthesized/negated/"-a"-combined groups
( test 3 -lt 10 ); assert_equal "0" "$?" "3 -lt 10"
( test ! \( 3 -lt 10 \) ); assert_equal "1" "$?" "! ( 3 -lt 10 )"
( test \( 3 -lt 10 \) -a \( 10 -gt 1 \) ); assert_equal "0" "$?" "( 3 -lt 10 ) -a ( 10 -gt 1 )"
( test \( 10 -gt 3 \) -a \( 1 -lt 10 \) ); assert_equal "0" "$?" "( 10 -gt 3 ) -a ( 1 -lt 10 )"
( test \( 10 -gt 3 \) -a ! \( 1 -gt 10 \) ); assert_equal "0" "$?" "( 10 -gt 3 ) -a ! ( 1 -gt 10 )"
( test \( 3 -lt 10 -a 10 -gt 1 \) ); assert_equal "0" "$?" "( 3 -lt 10 -a 10 -gt 1 )"
( test 1 -gt 0 ); assert_equal "0" "$?" "1 -gt 0"
( test 0 -gt 0 ); assert_equal "1" "$?" "0 -gt 0"
( test -1 -gt 0 ); assert_equal "1" "$?" "negative-lhs -gt comparison"

## file-attribute operators
( test -e "$0" ); assert_equal "0" "$?" "test -e on this test script itself"
( test -e CMakeLists.txt ); assert_equal "0" "$?" "test -e CMakeLists.txt"
( test -c /dev/null ); assert_equal "0" "$?" "test -c /dev/null is a char device"
( test -c CMakeLists.txt ); assert_equal "1" "$?" "test -c CMakeLists.txt is not a char device"
( test -d . ); assert_equal "0" "$?" "test -d ."
( test -d CMakeLists.txt ); assert_equal "1" "$?" "test -d CMakeLists.txt is not a directory"
( test -f CMakeLists.txt ); assert_equal "0" "$?" "test -f CMakeLists.txt"
( test -f . ); assert_equal "1" "$?" "test -f . is not a regular file"

## string operators
( test "BLAH" ); assert_equal "0" "$?" "a non-empty single-arg string test"
( test "" ); assert_equal "1" "$?" "an empty single-arg string test"
( test -n "BLAH" ); assert_equal "0" "$?" "test -n on a non-empty string"
( test -n "" ); assert_equal "1" "$?" "test -n on an empty string"
( test -z "" ); assert_equal "0" "$?" "test -z on an empty string"
( test -z "BLAH" ); assert_equal "1" "$?" "test -z on a non-empty string"
( test ! -n "BLAH" ); assert_equal "1" "$?" "negated test -n on a non-empty string"
( test ! -n "" ); assert_equal "0" "$?" "negated test -n on an empty string"

summary
