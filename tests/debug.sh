
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
