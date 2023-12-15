ASSERTIONS_SUCCEEDED=0
ASSERTIONS_FAILED=0

success() {
  ASSERTIONS_SUCCEEDED=$((ASSERTIONS_SUCCEEDED + 1))
  #: $((ASSERTIONS_SUCCEEDED++))
  return 0
}

failure() {
  ASSERTIONS_FAILED=$((ASSERTIONS_FAILED + 1))
  #: $((ASSERTIONS_FAILED++))
  echo "FAILURE" 1>&2
  exit 1
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

assert_greater() {
  if test "$1" -gt "$2"; then
    success
  else
    echo "${3:-"'$1' -le '$2'"}" 1>&2
    failure
  fi
  return $?
}

assert_less() {
  if test "$1" -lt "$2"; then
    success
  else
    echo "${3:-"'$1' -ge '$2'"}" 1>&2
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

summary() {
  print_stats
  echo "Success" 1>&2
  exit 0
}

trap 'print_stats' EXIT
