#!/bin/sh -x

[ $# -gt 0 ] || set -- lcc32 lcc64 tcc32 tcc64 bcc55 bcc101

for compiler; do 
  for build_type in Debug RelWithDebInfo MinSizeRel Release; do
	output_dir=build/$compiler/$build_type
	mkdir -p $output_dir
(set -x
	genmakefile --create-bins --no-create-libs -n shish -m ninja -t $compiler --$build_type lib src -o $output_dir/build.ninja
  genmakefile --create-bins --no-create-libs -n shish -m make -t $compiler --$build_type lib src -o $output_dir/Makefile)
  done
done
