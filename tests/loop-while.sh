DIR=$(dirname "${0}")
. "$DIR/common.sh"

## Testing while/until loops

## loop body executed as long as there are arguments
set -- A B C D E F G H
X=""
while [ $# -gt 0 ]; do
  X="$1$X"
  shift
done
assert_equal "HGFEDCBA" "$X" "while loop must iterate over every positional param"

## loop body never executed
I=0
while false; do
  I=$(( $I + 1 ))
done
assert_equal "0" "$I" "a while loop whose condition starts false must never run its body"

## loop body executed but 'break' on first opportunity
I=0
while true; do
  break
  I=$(( $I + 1 ))
done
assert_equal "0" "$I" "break must exit the loop before running anything after it in the body"

## loop body executed and 'break' after first iteration
I=0
while true; do
  I=$(( $I + 1 ))
  break
done
assert_equal "1" "$I" "break must still let the body run once before exiting"

## 'continue' skips the rest of the body but keeps looping
I=0
SUM=0
while [ "$I" -lt 5 ]; do
  I=$(( $I + 1 ))
  if [ "$I" = 3 ]; then
    continue
  fi
  SUM=$(( $SUM + $I ))
done
assert_equal "12" "$SUM" "continue must skip the rest of the body for that iteration only (1+2+4+5)"

## until loops until the condition becomes true (inverse of while)
I=0
until [ "$I" -ge 3 ]; do
  I=$(( $I + 1 ))
done
assert_equal "3" "$I" "until loop must run until its condition becomes true"

## until with an initially-true condition never runs its body
I=0
until true; do
  I=$(( $I + 1 ))
done
assert_equal "0" "$I" "an until loop whose condition starts true must never run its body"

summary
