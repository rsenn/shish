DIR=$(dirname "${0}")
. "$DIR/common.sh"

## Which builtins the CMake build enables (cmake/Builtins.cmake): the
## switches -DBUILTIN_<NAME>=ON|OFF and -DENABLE_ALL_BUILTINS=ON must be
## permanent, i.e. survive a later configure of the same build directory
## (cmake starts one by itself whenever a CMake file changes).
## Configures a tiny project that includes the real Builtins.cmake and
## reads the generated builtin_config.h.

CMAKE=$(command -v cmake)

if [ -z "$CMAKE" ]; then
  echo "cmake not found, skipping" 1>&2
  summary
fi

SRC=$(cd "$DIR/.." && pwd)
TESTDIR=$(mktemp -d)
cd "$TESTDIR" || exit 1

cat > CMakeLists.txt <<EOT
cmake_minimum_required(VERSION 3.10)
project(h C)
include($SRC/cmake/Functions.cmake)
include($SRC/cmake/Builtins.cmake)
EOT

## configure <builddir> [cmake args...]
configure() {
  D=$1
  shift
  "$CMAKE" -S . -B "$D" "$@" >"$D.log" 2>&1
}

## state <builddir> <NAME>: 1 or 0, as the build would see it
state() {
  sed -n "s/^#define BUILTIN_$2 //p" "$1/src/builtin_config.h"
}

## everything, and it stays that way
configure all -DENABLE_ALL_BUILTINS=ON
assert_equal "1 1 1" "$(state all CAT) $(state all RM) $(state all UNAME)" \
  "-DENABLE_ALL_BUILTINS=ON enables the extra builtins"
configure all
assert_equal "1 1 1" "$(state all CAT) $(state all RM) $(state all UNAME)" \
  "a later configure without the flag keeps ENABLE_ALL_BUILTINS"
configure all -DENABLE_ALL_BUILTINS=OFF
assert_equal "0 0 0" "$(state all CAT) $(state all RM) $(state all UNAME)" \
  "-DENABLE_ALL_BUILTINS=OFF turns it off again"

## an explicit BUILTIN_<NAME> wins over ENABLE_ALL_BUILTINS, and stays
configure mix -DENABLE_ALL_BUILTINS=ON -DBUILTIN_CAT=OFF
assert_equal "0 1" "$(state mix CAT) $(state mix LS)" \
  "-DBUILTIN_CAT=OFF beats -DENABLE_ALL_BUILTINS=ON"
configure mix
assert_equal "0 1" "$(state mix CAT) $(state mix LS)" \
  "both choices survive a later configure"

## one builtin, no ALL: on, and permanent
configure one -DBUILTIN_LS=ON
assert_equal "1 0" "$(state one LS) $(state one CAT)" \
  "-DBUILTIN_LS=ON enables just that builtin"
configure one
assert_equal "1 0" "$(state one LS) $(state one CAT)" \
  "-DBUILTIN_LS=ON survives a later configure"
configure one -DBUILTIN_LS=OFF
assert_equal "0" "$(state one LS)" "-DBUILTIN_LS=OFF turns it off again"

## defaults are recomputed, not frozen: enabled by default stays enabled,
## an extra stays off, and neither becomes sticky
configure def
assert_equal "1 0" "$(state def PRINTF) $(state def CAT)" \
  "with no flags: the default builtins, not the extra ones"
configure def -DENABLE_ALL_BUILTINS=ON
configure def -DENABLE_ALL_BUILTINS=OFF
assert_equal "0" "$(state def CAT)" \
  "extras are off again after ALL was switched on and off"

## the older spelling still works, and is converted to the new one
configure old -DENABLE_RM=ON
assert_equal "1" "$(state old RM)" "-DENABLE_RM=ON still enables rm"
configure old
assert_equal "1" "$(state old RM)" "and it is permanent now"

## a build directory written by the old scheme cached its computed answers
## (BUILTIN_CAT=ON here, next to the BUILD_BUILTIN_* entries every configure
## writes); those were never choices and must not stick
configure legacy -DBUILD_BUILTIN_ALIAS:INTERNAL=1 -DBUILTIN_CAT:BOOL=ON
assert_equal "0" "$(state legacy CAT)" \
  "a BUILTIN_<NAME> cached by the old scheme is dropped once"

cd / && rm -rf "$TESTDIR"

summary
