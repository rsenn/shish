
assert_equal() {
  case "$1" in
    "$2") return 0 ;;
    *) echo "${3:-"'$1' != '$2'"}" 1>&2
      return 1
      ;;
  esac
}
