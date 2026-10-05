#!/bin/sh
## Builds the minimal builtin set plus ONE extra builtin at a time and checks that every such
## configuration compiles, links and knows the builtin (`type NAME`).
##
##   sh tests/builtin-matrix.sh [name ...]     default: every builtin that is not tier m
##   WITHOUT="test" sh tests/builtin-matrix.sh '['   also switch these off (a tier m sibling: [ without test)
##
## Not a CTest case (one build per name); run it after touching a builtin's source or the map.
## The build directory is $MATRIX_DIR (default: build/matrix) and is reused between names.

SRC=$(cd "$(dirname "$0")/.." && pwd)
MAP=$SRC/src/builtin/builtins.map
DIR=${MATRIX_DIR:-$SRC/build/matrix}
mkdir -p "$DIR" || exit 1

# rows: "switch name tier"; the switch differs from the name where the map has macro= ("[" -> lbracket)
ROWS=$(awk '/^[^#]/ { sw = $1; for (i = 5; i <= NF; i++) if ($i ~ /^macro=/) sw = tolower(substr($i, 7)); print sw, $1, $3 }' "$MAP")
ALL=$(echo "$ROWS" | awk '{ print $1 }')
MINIMAL=$(echo "$ROWS" | awk '$3 == "m" { print $1 }' | tr "\n" " ")
NAMES=${*:-$(echo "$ROWS" | awk '$3 != "m" { print $2 }')}
BUILD_JOBS=${BUILD_JOBS:-2}

pass=0
fail=0

for name in $NAMES; do
  switch=$(echo "$ROWS" | awk -v n="$name" '$2 == n || $1 == n { print $1; exit }')
  [ -n "$switch" ] || { echo "$name: FAIL (not in builtins.map)"; fail=$((fail + 1)); continue; }
  args=
  for b in $ALL; do
    case " $WITHOUT " in
      *" $b "*) args="$args -DBUILTIN_$(echo "$b" | tr a-z. A-Z_)=OFF"; continue ;;
    esac
    case " $MINIMAL $switch " in
      *" $b "*) args="$args -DBUILTIN_$(echo "$b" | tr a-z. A-Z_)=ON" ;;
      *) args="$args -DBUILTIN_$(echo "$b" | tr a-z. A-Z_)=OFF" ;;
    esac
  done

  # shellcheck disable=SC2086
  if ! cmake -S "$SRC" -B "$DIR" -DCMAKE_BUILD_TYPE=MinSizeRel $args >"$DIR/$name.log" 2>&1 ||
     ! cmake --build "$DIR" -j "$BUILD_JOBS" --target shish >>"$DIR/$name.log" 2>&1; then
    echo "$name: FAIL (configure/build, see $DIR/$name.log)"
    fail=$((fail + 1))
    continue
  fi

  if "$DIR/shish" -c "type '$name'" >"$DIR/$name.out" 2>&1; then
    echo "$name: OK"
    pass=$((pass + 1))
  else
    echo "$name: FAIL (type: $(cat "$DIR/$name.out"))"
    fail=$((fail + 1))
  fi
done

echo "matrix: $pass ok, $fail failed"
[ "$fail" -eq 0 ]
