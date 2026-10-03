DIR=$(dirname "${0}")
. "$DIR/common.sh"

## id builtin (src/builtin/core/builtin_id.c); skipped when id is external
case $(type id) in
*builtin*) ;;
*) echo "id is not a builtin, skipping" 1>&2; summary ;;
esac

EXT=$(command -v id)
case $EXT in /*) ;; *) EXT=/usr/bin/id ;; esac

for o in "" -u -g -G -un -gn -Gn -ur -gr -Gr -unr; do
  assert_equal "$($EXT $o)" "$(id $o)" "id $o matches the system id"
done

assert_equal "$($EXT -u root)" "$(id -u root)" "id -u root"
assert_equal "$($EXT -Gn root)" "$(id -Gn root)" "id -Gn root"
assert_equal "$($EXT root)" "$(id root)" "id root"
assert_match "$(id -u)" "[0-9]*" "-u is a number"
assert_equal "$(id -u)" "$(id -ur)" "no setuid: effective and real user IDs are equal here"

id nosuchuser_xyz >/dev/null 2>&1
assert_equal "1" "$?" "an unknown user fails"
id -ug >/dev/null 2>&1
assert_equal "1" "$?" "-u and -g together fail"
id -n >/dev/null 2>&1
assert_equal "1" "$?" "-n without -u, -g or -G fails"
id -Z >/dev/null 2>&1
assert_equal "1" "$?" "an unknown option fails"
id a b >/dev/null 2>&1
assert_equal "1" "$?" "two operands fail"

summary
