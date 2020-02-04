@echo off

for %%b in (Debug RelWithDebInfo MinSizeRel Release) do (
  for %%t in (lcc32 lcc64 tcc32 tcc64 bcc55 bcc101) do (
    set output_dir=build/%%t/%%b
	mkdir -p %output_dir%

	genmakefile -s windows -m ninja -t %%t --%%b lib src -o %output_dir%/build.ninja
	genmakefile -s windows -m nmake -t %%t --%%b lib src -o %output_dir%/Makefile
  done
done