#!/bin/sh
## Builds the minimal builtin set plus ONE extra builtin at a time and checks that every such
## configuration compiles, links and knows the builtin (`type NAME`).
##
##   sh tests/builtin-matrix.sh [name ...]     default: every builtin that is not tier m
##
## Not a CTest case (one build per name); run it after touching a builtin's source or the map.
## The build directory is $MATRIX_DIR (default: build/matrix) and is reused between names.

SRC=$(cd "$(dirname "$0")/.." && pwd)
MAP=$SRC/src/builtin/builtins.map
DIR=${MATRIX_DIR:-$SRC/build/matrix}
mkdir -p "$DIR" || exit 1

ALL=$(awk '/^[^#]/ { print $1 }' "$MAP")
MINIMAL=$(awk '/^[^#]/ && $3 == "m" { print $1 }' "$MAP" | tr "\n" " ")
NAMES=${*:-$(awk '/^[^#]/ && $3 != "m" { print $1 }' "$MAP")}
BUILD_JOBS=${BUILD_JOBS:-$(nproc 2>/dev/null || echo 2)}

pass=0
fail=0

for name in $NAMES; do
  args=
  for b in $ALL; do
    case " $MINIMAL $name " in
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

  if "$DIR/shish" -c "type $name" >"$DIR/$name.out" 2>&1; then
    echo "$name: OK"
    pass=$((pass + 1))
  else
    echo "$name: FAIL (type: $(cat "$DIR/$name.out"))"
    fail=$((fail + 1))
  fi
done

echo "matrix: $pass ok, $fail failed"
[ "$fail" -eq 0 ]
