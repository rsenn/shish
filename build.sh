#!/bin/sh

IFS="
"

mydir=`dirname "$0"`
distrib=`lsb_release -s -i | tr "[[:"{upper,lower}":]]"`
arch=`${CC-cc} -dumpmachine | sed "s|-.*||"`
type=Debug

cd "$mydir"

builddir=build/$distrib-$arch

mkdir -p $builddir

exec_in_builddir() {
 (IFS=" "
  cmd="(cd $builddir; $*)"
	echo "+ $cmd" 1>&2
  eval "$cmd")
}

addprefix() {
 (PREFIX=$1; shift
  CMD='echo "$PREFIX$LINE"'
  [ $# -gt 0 ] && CMD="for LINE; do $CMD; done" || CMD="while read -r LINE; do $CMD; done"
  eval "$CMD")
}

set \
	CMAKE_INSTALL_PREFIX="${prefix-/usr}" \
	CMAKE_VERBOSE_MAKEFILE="TRUE" \
	CMAKE_BUILD_TYPE="${type}" \
	CMAKE_C_COMPILER="${CC-gcc}"
	CMAKE_CXX_COMPILER="${CXX-g++}"

exec_in_builddir "rm -rf *"
exec_in_builddir cmake $(addprefix -D <<<"$*") ../..
exec_in_builddir make
