echo "\$0=$0"
DIR=$(dirname "$0")
echo "\$DIR=$DIR"

#. "$DIR/common.sh"

set -e

## Testing test builtin

#test \( \! 3 -gt 10 \) -a \( \! 10 -lt 1 \)

test_cmd() {
	(
		case "$-" in
		*x*) ;;
		*) echo -n test "$@" ;;
		esac
		test "$@"
		echo " = $?"
	)
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

test_or_fail() {
	EXPECT=$1
	shift
	CMD="test \"\$@\""
	eval echo -n "$CMD" 1>&2
	eval "$CMD"
	R=$?
	echo " = $R"
	case "$R" in
	$EXPECT) return 0 ;;
	*)
		eval 'echo test "'$CMD'" failed 1>&2; exit 1'
		;;
	esac
}

test_or_fail 0 -e "$0"
test_or_fail 0 -e CMakeLists.txt
test_or_fail 0 -c /dev/null
test_or_fail 1 -c CMakeLists.txt
test_or_fail 0 -d .
test_or_fail 1 -d CMakeLists.txt
test_or_fail 0 -f CMakeLists.txt
test_or_fail 1 -f .
test_or_fail 0 "BLAH"
test_or_fail 1 ""
test_or_fail 0 -n "BLAH"
test_or_fail 1 -n ""
test_or_fail 0 -z ""
test_or_fail 1 -z "BLAH"

test_or_fail 1 ! -n "BLAH"
test_or_fail 0 ! -n ""
