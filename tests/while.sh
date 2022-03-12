DIR=$(dirname "${0}")
. "$DIR/common.sh"

set -e

## Testing while loop

set -- A B C D E F G H
echo $#
IFS=","
while [ $# -gt 0 ]; do
  echo "$# : $1"
  shift
done
