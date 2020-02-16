#!/bin/sh

gen_builtin_config_header() { 
  for x in $(tokextract.sh src/builtin/builtin_table.c |grep ^BUILTIN|sort -fu); do
    printf "#cmakedefine %-20s @BUILD_%s@\n" "$x" "$x";
  done
}
gen_builtin_config_header | tee src/builtin_config.h.cmake
