DIR=$(dirname "${0}")
. "$DIR/common.sh"

set -e

## Testing parameter expansion

V="ABCD"
assert_equal "AB" "${V%CD}"
assert_equal "CD" "${V#AB}"

V="ABCD"
assert_equal "A" "${V%BC*}"
assert_equal "D" "${V#*BC}"

V="AAAAAAAAAAAAAAAABBBBBBBBBBBBBBBBCCCCCCCCCCCCCCCCDDDDDDDDDDDDDDDD"
assert_equal "AAAAAAAAAAAAAAAA" "${V%%B*}"
assert_equal "DDDDDDDDDDDDDDDD" "${V##*C}"

unset V

assert_equal "unset" "${V-unset}"
assert_equal "" "${V+set}"

V=""
assert_equal "" "${V-unset}"
assert_equal "empty" "${V:-empty}"
assert_equal "set" "${V+set}"
assert_equal "" "${V:+nonempty}"

V="..."
assert_equal "..." "${V-unset}"
assert_equal "..." "${V:-empty}"
assert_equal "set" "${V+set}"
assert_equal "nonempty" "${V:+nonempty}"

unset W
: ${W="init"}
assert_equal "$W" "init"

unset X
: ${X:="if-zero"}
assert_equal "$X" "if-zero"

X=""
: ${X:="if-zero"}
assert_equal "$X" "if-zero"

X="..."
: ${X="init"}
assert_equal "$X" "..."
: ${X:="if-zero"}
assert_equal "$X" "..."

#set +e
Y=
unset Y
Y=A
Z=$( (: ${Y?"Y is unset"}) 2>&1)
assert_equal "$?" "1"
assert_equal "${Z##*: }" "Y is unset"
set -e

## Positional parameters

set -- A1 A2 A3 A4 A5 A6 A7 A8 \
	ARG9 ARG10 ARG11 ARG12 ARG13 ARG14 ARG15 ARG16

assert_equal "$#" 16
assert_equal "$3" A3
assert_equal "${3}" A3
assert_equal "$14" A14
assert_equal "${14}" ARG14

## Special parameters

## $* argument concatenating thru IFS

saved_IFS=$IFS
IFS="."
assert_equal "$*" "A1.A2.A3.A4.A5.A6.A7.A8.ARG9.ARG10.ARG11.ARG12.ARG13.ARG14.ARG15.ARG16"
IFS="$saved_IFS"

## $@ remove prefix on every argument

set -- "${@#A}"
assert_equal "$1" 1
assert_equal "$9" RG9

## $- shell options substitution
assert_nomatch "$-" "*f*"
set -f
assert_match "$-" "*f*"
set +f
assert_nomatch "$-" "*f*"

print_stats
echo "Success" 1>&2
exit 0
