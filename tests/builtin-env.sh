DIR=$(dirname "${0}")
. "$DIR/common.sh"

## env builtin (src/builtin/core/builtin_env.c); skipped when env is external
case $(type env) in
*builtin*) ;;
*) echo "env is not a builtin, skipping" 1>&2; summary ;;
esac

PRINTENV=$(command -v printenv)

export ENVT_A=1
assert_equal "1" "$(env | grep -c '^ENVT_A=1$')" "without a utility the environment is printed"
assert_equal "0" "$(ENVT_UNEXPORTED=1; env | grep -c '^ENVT_UNEXPORTED=')" "unexported variables are not printed"

assert_equal "1 2" "$(env ENVT_B=1 ENVT_C=2 sh -c 'echo $ENVT_B $ENVT_C')" "name=value operands reach the utility"
assert_equal "1" "$(env ENVT_B=1 | grep -c '^ENVT_B=1$')" "name=value operands show in the printed environment"
assert_equal "[]" "[$ENVT_B]" "the shell's own variables are untouched afterwards"
assert_equal "x=y" "$(env 'ENVT_E=x=y' sh -c 'echo $ENVT_E')" "only the first = splits name from value"
assert_equal "[]" "$(env ENVT_EMPTY= sh -c 'echo [$ENVT_EMPTY]' | sed 's/\[\]/[]/')" "an empty value is set"

assert_equal "0" "$(env -u ENVT_A sh -c 'echo ${ENVT_A+set}' | grep -c set)" "-u removes a variable for the utility"
assert_equal "1" "$ENVT_A" "-u does not touch the shell's variable"
assert_equal "0" "$(env -u ENVT_A env | grep -c '^ENVT_A=')" "-u removes a variable from the printed environment"

assert_equal "ENVT_ONLY=1" "$(env -i ENVT_ONLY=1 $PRINTENV)" "-i starts with an empty environment"
assert_equal "" "$(env -i)" "-i alone prints nothing"
assert_equal "1" "$(env -i ls / >/dev/null; echo 1)" "-i still finds a program by its name"

env nosuchcommand_xyz >/dev/null 2>&1
assert_equal "127" "$?" "a utility that is not found exits 127"
env sh -c 'exit 7'
assert_equal "7" "$?" "the exit status of the utility is returned"
env /dev/null >/dev/null 2>&1
assert_equal "126" "$?" "a utility that cannot be run exits 126"

f() { echo FUNC; }
env f >/dev/null 2>&1
assert_equal "127" "$?" "a function is not a utility"
assert_equal "1" "$(env -- ENVT_D=1 sh -c 'echo $ENVT_D')" "-- ends the options"
env -Z >/dev/null 2>&1
assert_equal "125" "$?" "an unknown option fails with 125"

summary
