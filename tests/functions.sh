DIR=$(dirname "${0}")
. "$DIR/common.sh"

## Testing functions

CALLED=""

func1() {
  CALLED="${CALLED}1"
  return 0
}

func2() (
  CALLED="${CALLED}2"
)

func3() {
  CALLED="${CALLED}3"
  return 7
}

func1
assert_equal "0" "$?" "a function's explicit \"return 0\" must be its exit status"
func2
assert_equal "0" "$?" "a function ending on a successful command must exit 0"
func3
assert_equal "7" "$?" "a function's explicit \"return N\" must be its exit status"

## func2's own assignment must NOT be visible here: its body is a
## subshell ("(...)"), which isolates variable assignments from the
## caller exactly like a bare "(...)" does (fixes/62,
## subshell-function-body-isolation) -- so only func1's and func3's
## "1"/"3" show up, not func2's "2".
assert_equal "13" "$CALLED" "functions must run in the order they're called, each seeing the caller's variables, except a subshell-bodied one whose own assignments must stay isolated"

## argument handling: $1/$#/$@ inside a function are its own, not the
## caller's
argtest() {
  assert_equal "3" "$#" "a function's \$# reflects the arguments it was called with"
  assert_equal "b" "$2" "a function's positional params are its call arguments, in order"
}
argtest a b c

## string length via ${#var}, both the short case-matched lengths and
## the generic fallback
len() {
  case "$1" in
  ?) return 1 ;;
  ??) return 2 ;;
  ???) return 3 ;;
  ????) return 4 ;;
  *) return ${#1} ;;
  esac
}

len "A"
assert_equal "1" "$?" "len() case-matched branch for a 1-char string"
len "ABC"
assert_equal "3" "$?" "len() case-matched branch for a 3-char string"
len "ABCDEF"
assert_equal "6" "$?" "len() \${#1} fallback branch for a string longer than the matched cases"

## recursion
fact() {
  if [ "$1" -le 1 ]; then
    echo 1
  else
    PREV=$(fact $(($1 - 1)))
    echo $(($1 * PREV))
  fi
}
assert_equal "120" "$(fact 5)" "a recursive function must compute the right result (5! = 120)"

summary
