DIR=$(dirname "${0}")
. "$DIR/common.sh"

## src/builtin/builtins.map, builtin_table.c and the source tree describe the same builtins
TOP=$DIR/..
MAP=$TOP/src/builtin/builtins.map
ROWS=$(sed -e '/^#/d' -e '/^[ 	]*$/d' "$MAP")

T=$(mktemp -d)
echo "$ROWS" | awk '{ n = $1; for (i = 5; i <= NF; i++) if ($i ~ /^macro=/) n = tolower(substr($i, 7)); print n }' | sort > "$T/names"
tr -c 'A-Za-z0-9_' '\n' < "$TOP/src/builtin/builtin_table.c" | sed -n -e '/^BUILTIN_CONFIG_H$/d' -e 's/^BUILTIN_//p' | tr 'A-Z' 'a-z' | sort -u > "$T/used"
names=$(cat "$T/names")

assert_equal "" "$(comm -13 "$T/names" "$T/used" | tr '\n' ' ')" "every BUILTIN_<NAME> of builtin_table.c has a line in builtins.map"
assert_equal "" "$(comm -23 "$T/names" "$T/used" | tr '\n' ' ')" "every line of builtins.map is a BUILTIN_<NAME> of builtin_table.c"

## a switch spelled wrong in an #if is silently 0 (builtin_config.h defines every real one)
(cd "$TOP/src" && grep -rhoE '\bBUILTIN_[A-Za-z0-9_]+' --include='*.c' --include='*.h' .) | sed -n -e '/^BUILTIN_H$/d' -e '/^BUILTIN_CONFIG_H$/d' -e 's/^BUILTIN_//p' | tr 'A-Z' 'a-z' | sort -u > "$T/srcused"
assert_equal "" "$(comm -13 "$T/names" "$T/srcused" | tr '\n' ' ')" "every BUILTIN_<NAME> used under src/ is a switch of builtins.map"
rm -rf "$T"

assert_equal "" "$(echo "$ROWS" | awk '{ bad = NF < 4 || NF > 6 || $3 !~ /^[mdx]$/; for (i = 5; i <= NF; i++) if ($i != "keep" && $i !~ /^macro=[A-Z0-9_]+$/) bad = 1; if (bad) print $1 }' | tr '\n' ' ')" "every map line has four to six columns, a tier of m, d or x, and only keep/macro= flags"
assert_equal "" "$(echo "$names" | uniq -d | tr '\n' ' ')" "no name occurs twice in builtins.map"

## every source (or directory, written with a trailing /) a line names exists
missing=
for f in $(echo "$ROWS" | awk '{print $2; if ($4 != "-") print $4}' | tr ',' '\n' | sort -u); do
  case $f in
  */) test -d "$TOP/src/builtin/$f" || missing="$missing $f" ;;
  *) test -f "$TOP/src/builtin/$f" || missing="$missing $f" ;;
  esac
done
assert_equal "" "$missing" "every file named in builtins.map exists"

## every builtin_*.c is named by some line, except the three that are always compiled
named=$(echo "$ROWS" | awk '{print $2; if ($4 != "-") print $4}' | tr ',' '\n' | sort -u)
unnamed=
for f in $(cd "$TOP/src/builtin" && ls builtin_*.c */builtin_*.c); do
  case $f in builtin_error.c|builtin_search.c|builtin_table.c) continue ;; esac
  case "
$named
" in *"
$f
"*) ;; *) unnamed="$unnamed $f" ;; esac
done
assert_equal "" "$unnamed" "every builtin_*.c is named in builtins.map (or is builtin_error/search/table)"

summary
