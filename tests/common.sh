ASSERTIONS_SUCCEEDED=0
ASSERTIONS_FAILED=0

success() {
  #ASSERTIONS_SUCCEEDED=$((ASSERTIONS_SUCCEEDED + 1))
  #: $((ASSERTIONS_SUCCEEDED++))
  return 0
}

failure() {
  #ASSERTIONS_FAILED=$((ASSERTIONS_FAILED + 1))
  #: $((ASSERTIONS_FAILED++))
  return 1
}

assert_equal() {
  if test "$1" = "$2"; then
    success
  else
    echo "${3:-"'$1' != '$2'"}" 1>&2
    failure
  fi
  return $?
}

assert_match() {
  case "$1" in
  $2)
    success
    ;;
  *)
    echo "${3:-"'$1' MATCH '$2'"}" 1>&2
    failure
    ;;
  esac
  return $?
}

assert_nomatch() {
  case "$1" in
  $2)
    echo "${3:-"'$1' NOMATCH '$2'"}" 1>&2
    failure
    ;;
  *)
    success
    ;;
  esac
  return $?
}

print_stats() {
  echo "Tests succeeded: ${ASSERTIONS_SUCCEEDED}" 1>&2
  echo "Tests failure: ${ASSERTIONS_FAILED}" 1>&2
}

trap 'print_stats' EXIT
