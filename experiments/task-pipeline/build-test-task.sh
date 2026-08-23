#!/bin/sh
# build-test-task.sh <gcc|emcc> -- build test-task.c and
# test-task-buffers.c against task.h's two backends. gcc links
# ../../../libaco directly; emcc needs -sASYNCIFY, nothing else.
#
# test-task-buffers.c also needs libowfat's buffer_* functions --
# reused from this repo's own build/ tree (native: build/<triple>,
# picked via `$cc -dumpmachine`; wasm: build/emscripten), so run a
# cfg-cmake.sh build for that target first if it isn't there yet.
#
# Runnable from anywhere -- cd's to its own directory first.
set -e

cd "$(dirname "$0")"

cc=$1

if [ -z "$cc" ]; then
  echo "usage: $0 <gcc|emcc>" >&2
  exit 1
fi

case $(basename "$cc") in
  emcc)
    owfat=../../build/emscripten/libowfat.a

    if [ ! -f "$owfat" ]; then
      echo "$0: $owfat not found -- run cfg-emscripten (see cfg-cmake.sh) first" >&2
      exit 1
    fi

    "$cc" -O2 -Wall -Wextra -sASYNCIFY -o test-task.js test-task.c
    echo "built test-task.js -- run with: node test-task.js"

    "$cc" -O2 -Wall -Wextra -sASYNCIFY -I../../lib -o test-task-buffers.js \
      test-task-buffers.c "$owfat"
    echo "built test-task-buffers.js -- run with: node test-task-buffers.js"
    ;;
  gcc|cc|clang)
    triple=$("$cc" -dumpmachine)
    owfat="../../build/$triple/libowfat.a"

    if [ ! -f "$owfat" ]; then
      echo "$0: $owfat not found -- run cfg (see cfg-cmake.sh) for $triple first" >&2
      exit 1
    fi

    "$cc" -O2 -Wall -Wextra -o test-task \
      test-task.c ../../../libaco/aco.c ../../../libaco/acosw.S -I../../../libaco
    echo "built test-task -- run with: ./test-task"

    "$cc" -O2 -Wall -Wextra -o test-task-buffers \
      test-task-buffers.c ../../../libaco/aco.c ../../../libaco/acosw.S \
      -I../../../libaco -I../../lib "$owfat"
    echo "built test-task-buffers -- run with: ./test-task-buffers"
    ;;
  *)
    echo "$0: unsupported compiler '$cc' (expected gcc or emcc)" >&2
    exit 1
    ;;
esac
