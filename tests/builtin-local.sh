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

summary
