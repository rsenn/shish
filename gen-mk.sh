#!/bin/sh

IFS="
"
IFS=" $IFS"

exec_cmd() {
  (CMD="$*"; eval echo "+ $CMD" 1>&2; eval "eval \"$CMD\"")
}

LIBS="-lws2_32"

if [ "$COMPRESS" = true ]; then
  LIBS="$LIBS -lzlib -lbz2 -llzma"
  DEFS="  DHAVE_{ZLIB,LIBBZ2,LIBLZMA}=1"
fi

[ $# -eq 0 ] && 
set -- lcc32 lcc64 tcc32 tcc64 bcc55 bcc101

echo "MODULES=$MODULES" 1>&2
#set -- 
#set -- "$@" pocc32 pocc64
#set -- "$@" dmc32 occ32

for compiler; do
  for build_type in Debug RelWithDebInfo MinSizeRel Release; do
  output_dir="build/$compiler/$build_type"
  mkdir -p "$output_dir"

  (exec_cmd "genmakefile --create-bins --create-libs -m ninja -t $compiler --$build_type  \$DEFS -Ilib -Isrc lib src -o $output_dir/build.ninja \$LIBS"
    exec_cmd  "genmakefile --create-bins --create-libs -m make -t $compiler --$build_type \$DEFS -Ilib -Isrc lib src -o $output_dir/Makefile"
    exec_cmd  "genmakefile --create-bins --create-libs -m make -t $compiler --$build_type \$DEFS -Ilib -Isrc lib src -o $compiler.mk")
  done
done
