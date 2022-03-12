DIR=$(dirname "${0}")
. "$DIR/common.sh"

set -e

## Testing debug flag

set -x

ls -la >/dev/null

type -f cd
type -f set
type -f column

fn() {
  true
  false
  :
  set -- a
}


{ fn blah; }
