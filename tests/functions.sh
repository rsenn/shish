DIR=$(dirname "${0}")
. "$DIR/common.sh"

set -e

## Testing functions

func1() {
  echo "called func1"
  return 0
}

echo "define func2"

func2() (
  echo "called func2"
)

echo "define func3"

func3() {
  echo "called func3"
}

len() {
  case "$1" in
    ?) return 1 ;;
    ??) return 2 ;;
    ???) return 3 ;;
    ????) return 4 ;;
   *) return ${#1} ;;
  esac
}

func1; func2; func3

len "ABC"; echo $?
len "ABCDEF"; echo $?
