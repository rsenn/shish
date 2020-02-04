@echo off

for %%t in (lcc32 lcc64 tcc32 tcc64 bcc55 bcc101) do (
  for %%b in (Debug RelWithDebInfo MinSizeRel Release) do (
  
    call :gen %%t %%b
  
  )
)
goto :end

:gen
set output_dir=build\%1\%2
if not exist %output_dir% mkdir %output_dir%
del /f /q /s %output_dir%\*

echo on
genmakefile --create-bins --no-create-libs -m ninja -t %1 --%2 lib src -o %output_dir%\build.ninja
genmakefile --create-bins --no-create-libs -m nmake -t %1 --%2 lib src -o %output_dir%\Makefile

@echo off


:end