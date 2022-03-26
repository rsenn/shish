DIR=$(dirname "${0}")
. "$DIR/common.sh"

set -e

## Testing command substitution

fn() {
  #fdtable -u 2
  dump -t
  dump -s
  /opt/diet/bin/cat
}

X=1337
HERE=$(fn <<EOF
Here doc content $X
EOF
)
echo "HERE='$HERE'" 1>&2

OUTPUT=$(echo "This is output from an 'echo' command")
echo "OUTPUT='$OUTPUT'" 1>&2

FORMATTED=`printf "0x%08X" $((0xdeadbeef))`
echo "FORMATTED='$FORMATTED'" 1>&2
