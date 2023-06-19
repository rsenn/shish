echo "\$0=$0"
DIR=$(dirname "$0")
echo "\$DIR=$DIR"

#. "$DIR/common.sh"


set -e

## Testing test builtin


#test \( \! 3 -gt 10 \) -a \( \! 10 -lt 1 \)


test_cmd() {
  (case "$-" in
  *x*)  ;;
  *) echo test "$@" ;;
esac
 test "$@"
 echo " = $?");
}

test_cmd '(' 3 -lt 10 ')'
test_cmd '!' '(' 3 -lt 10 ')'
test_cmd '(' 3 -lt 10 ')' -a '(' 10 -gt 1 ')'
test_cmd '(' 10 -gt 3 ')' -a '(' 1 -lt 10 ')'
test_cmd '(' 10 -gt 3 ')' -a '!' '(' 1 -gt 10 ')'

test_cmd '(' 3 -lt 10 -a 10 -gt 1 ')'

test_cmd 1 -gt 0
test_cmd 0 -gt 0
test_cmd -1 -gt 0

if test -e "$0"; then
  echo "I exist!"
fi
