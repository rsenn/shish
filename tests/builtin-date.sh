DIR=$(dirname "${0}")
. "$DIR/common.sh"

## date builtin (src/builtin/core/builtin_date.c); skipped when date is external
case $(type date) in
*builtin*) ;;
*) echo "date is not a builtin, skipping" 1>&2; summary ;;
esac

assert_match "$(date +%s)" "[0-9][0-9]*" "+%s is a number"
assert_nomatch "$(date +%s)" "*[!0-9]*" "+%s is only digits"
assert_match "$(date +%Y-%m-%d)" "[0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9]" "+%Y-%m-%d"
assert_equal "UTC" "$(date -u +%Z)" "-u is UTC"
assert_equal "a%b" "$(date +a%%b)" "%% is a literal percent"
assert_equal "x y" "$(date '+x y')" "the format may contain blanks"
assert_equal "" "$(date +)" "an empty format prints an empty line"
assert_equal "1" "$(date | wc -l | tr -d ' ')" "the default format is one line"
assert_match "$(date -u)" "*UTC*" "-u with the default format"

assert_equal "JST" "$(TZ=Asia/Tokyo date +%Z)" "TZ in the command's environment is honoured"
export TZ=America/New_York
assert_match "$(date +%Z)" "E[SD]T" "an exported TZ is honoured"
assert_equal "UTC" "$(TZ=UTC0 date +%Z)" "a TZ prefix overrides the exported one"
unset TZ

date 010100002020 >/dev/null 2>&1
assert_equal "1" "$?" "setting the clock is not supported"
date +%Y extra >/dev/null 2>&1
assert_equal "1" "$?" "an extra operand fails"
date -Z >/dev/null 2>&1
assert_equal "1" "$?" "an unknown option fails"

summary
