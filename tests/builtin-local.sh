DIR=$(dirname "${0}")
. "$DIR/common.sh"

## Testing local builtin

test_fn1() {
  local LXXXXXX LYYYYYY='b'

  assert_equal "" "$LXXXXXX" "declared-without-value local starts unset"
  assert_equal "b" "$LYYYYYY" "declared-with-value local gets its initializer"

  LXXXXXX="a"
  GWWWWWW='c'

  assert_equal "a" "$LXXXXXX" "local var reflects an assignment made inside its function"
}

test_fn1

assert_equal "unset" "${LXXXXXX-unset}" "local var must not leak out of the function that declared it"
assert_equal "unset" "${LYYYYYY-unset}" "local var must not leak out of the function that declared it"
assert_equal "c" "$GWWWWWW" "a plain (non-local) assignment inside a function must leak to the global scope"

## no arguments: print the function's own locals, ready for re-input
f() {
  local a=1 b="two words"
  local
}
X=$(f)
assert_equal 'local a="1"
local b="two words"' "$X" "local without arguments prints each local as 'local name=\"value\"'"

g() {
  local outer=1
  h() { local inner='q"x'; local; }
  h
}
X=$(g)
assert_equal 'local inner="q\"x"' "$X" "local lists only the current function's variables, with the value escaped"

X=$(local)
assert_equal "" "$X" "local without arguments outside a function prints nothing"

summary
