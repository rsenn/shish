#!/bin/sh -x -e

for build_type in Debug RelWithDebInfo MinSizeRel Release; do
  for compiler in {lcc,tcc}{32,64} bcc{55,101}; do 
	output_dir=build/$compiler/$build_type
	mkdir -p $output_dir

	genmakefile -s windows -m ninja -t $compiler --$build_type lib src -o $output_dir/build.ninja
	genmakefile -s windows -m nmake -t $compiler --$build_type lib src -o $output_dir/Makefile
  done
done