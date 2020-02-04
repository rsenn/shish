rem @echo off

for %%b in (Debug RelWithDebInfo MinSizeRel Release) do (
  for %%t in (lcc32 lcc64 tcc32 tcc64 bcc55 bcc101) do (
    mkdir build\%%t\%%b

    genmakefile --create-bins --no-create-libs -m ninja -t %%t --%%b lib src -o build\%%t\%%b\build.ninja
    genmakefile --create-bins --no-create-libs -m make -t %%t --%%b lib src -o build\%%t\%%b\Makefile
  )
)
