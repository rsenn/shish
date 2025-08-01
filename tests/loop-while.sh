DIR=$(dirname "${0}")
. "$DIR/common.sh"

set -e

## Testing while loop

# loop body executed as long as there are arguments
set -- A B C D E F G H
X=""

while [ $# -gt 0 ]; do
  X="$1$X"
  shift
done

assert_equal "$X" "HGFEDCBA" 


# loop body never executed
I=0
while false; do
  I=$(( $I + 1 ))
done

assert_equal "$I" 0

# loop body executed but 'break' on first opportunity
I=0
while true; do
  break
  I=$(( $I + 1 ))
done

assert_equal "$I" 0

# loop body executed and 'break' after first iteration
I=0
while true; do
  I=$(( $I + 1 ))
  break
done

assert_equal "$I" 1
