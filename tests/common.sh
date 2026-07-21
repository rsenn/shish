ASSERTIONS_SUCCEEDED=0
ASSERTIONS_FAILED=0

## every test reports "<description>: OK"/"<description>: FAIL" (OK in
## green, FAIL in red) as it runs, rather than stopping at the first
## failure -- so a single run always shows the full pass/fail picture
## for the file. Overall pass/fail is decided at the end by summary(),
## which must be the last thing every tests/*.sh file calls.
report() {
  DESC="$1"

  if [ "$2" = 0 ]; then
    ASSERTIONS_SUCCEEDED=$((ASSERTIONS_SUCCEEDED + 1))
    printf '%s: \033[32mOK\033[0m\n' "$DESC" 1>&2
  else
    ASSERTIONS_FAILED=$((ASSERTIONS_FAILED + 1))
    printf '%s: \033[31mFAIL\033[0m\n' "$DESC" 1>&2
  fi
}

assert_equal() {
  if test "$1" = "$2"; then
    report "${3:-"'$1' = '$2'"}" 0
  else
    report "${3:-"'$1' = '$2'"}" 1
  fi
}

assert_greater() {
  if test "$1" -gt "$2"; then
    report "${3:-"'$1' -gt '$2'"}" 0
  else
    report "${3:-"'$1' -gt '$2'"}" 1
  fi
}

assert_less() {
  if test "$1" -lt "$2"; then
    report "${3:-"'$1' -lt '$2'"}" 0
  else
    report "${3:-"'$1' -lt '$2'"}" 1
  fi
}

assert_match() {
  case "$1" in
  $2)
    report "${3:-"'$1' MATCH '$2'"}" 0
    ;;
  *)
    report "${3:-"'$1' MATCH '$2'"}" 1
    ;;
  esac
}

assert_nomatch() {
  case "$1" in
  $2)
    report "${3:-"'$1' NOMATCH '$2'"}" 1
    ;;
  *)
    report "${3:-"'$1' NOMATCH '$2'"}" 0
    ;;
  esac
}

print_stats() {
  echo "Tests succeeded: ${ASSERTIONS_SUCCEEDED}" 1>&2
  echo "Tests failed: ${ASSERTIONS_FAILED}" 1>&2
}

## must be the last statement in every tests/*.sh file: prints the
## final tally and exits non-zero (so ctest sees the failure) if any
## assertion failed.
summary() {
  print_stats

  if [ "$ASSERTIONS_FAILED" -gt 0 ]; then
    echo "Fail: ${ASSERTIONS_FAILED}" 1>&2
    exit 1
  fi

  echo "Success" 1>&2
  exit 0
}
