@echo off


if "%compilers"=="" set compilers=lcc32 lcc64 tcc32 tcc64 bcc55 bcc101
if "%build_types"=="" set build_types=Debug RelWithDebInfo MinSizeRel Release

for %%t in (%compilers%) do for %%b in (%build_types%) do call :gen %%t %%b

goto :end

:gen
set output_dir=build\%1\%2
if not exist %output_dir% mkdir %output_dir%
del /f /q /s %output_dir%\*

@echo on
genmakefile --create-bins --no-create-libs -n shish -m ninja -t %1 --%2 lib src -o %output_dir%\build.ninja
genmakefile --create-bins --no-create-libs -n shish -m make -t %1 --%2 lib src -o %output_dir%\Makefile

@echo off


:end
