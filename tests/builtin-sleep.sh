DIR=$(dirname "${0}")
. "$DIR/common.sh"

## Testing sleep builtin

## sleep 0 returns immediately and succeeds
sleep 0
assert_equal "0" "$?" "sleep 0 succeeds"

## sleep actually suspends execution for roughly the requested time
START=$(date +%s)
sleep 1
END=$(date +%s)
ELAPSED=$((END - START))
assert_greater "$ELAPSED" "0" "sleep 1 lets at least a second pass"

## a fractional value is accepted and actually waits that long
START=$(date +%s%N)
sleep 0.2
END=$(date +%s%N)
ELAPSED_MS=$(( (END - START) / 1000000 ))
assert_greater "$ELAPSED_MS" "100" "sleep 0.2 waits at least 100ms"

## 's', 'm', 'h', 'd' suffixes are accepted and scale the value
sleep 0.2s
assert_equal "0" "$?" "sleep accepts an explicit 's' suffix"

START=$(date +%s%N)
sleep 0.01m
END=$(date +%s%N)
ELAPSED_MS=$(( (END - START) / 1000000 ))
assert_greater "$ELAPSED_MS" "500" "sleep 0.01m (0.6s) waits at least 500ms"

## a non-numeric argument is an error
sleep abc >/dev/null 2>&1
assert_equal "1" "$?" "sleep rejects a non-numeric argument"

## an unknown suffix is an error
sleep 1x >/dev/null 2>&1
assert_equal "1" "$?" "sleep rejects an unknown unit suffix"

## a missing argument is an error
sleep >/dev/null 2>&1
assert_equal "1" "$?" "sleep requires an operand"

## more than one argument is an error
sleep 1 2 >/dev/null 2>&1
assert_equal "1" "$?" "sleep rejects more than one operand"

summary
