#!/bin/sh -x

for build_type in Debug RelWithDebInfo MinSizeRel Release; do
  for compiler in lcc32 lcc64 tcc32 tcc64 bcc55 bcc101; do 
	output_dir=build/$compiler/$build_type
	mkdir -p $output_dir

	genmakefile --create-bins --no-create-libs -n shish -m ninja -t $compiler --$build_type lib src -o $output_dir/build.ninja
	genmakefile --create-bins --no-create-libs -n shish -m make -t $compiler --$build_type lib src -o $output_dir/Makefile
  done
done
