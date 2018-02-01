cfg() { 
  [ "$DEBUG" = 1 ] && TYPE=Debug
  [ "$VERBOSE" = 1 ] && VERBOSE_MAKEFILE=TRUE

  case $(uname -o) in
      MSys|MSYS|Msys) SYSTEM="MSYS" ;;
      *) SYSTEM="Unix" ;;
  esac

  [ -n "$TYPE" ] && build=cmake-$(echo "$TYPE" | tr "[[:upper:]]" "[[:lower:]]")

  libdir=`get-libdir`
  builddir=build/${build:-cmake}
  : ${prefix=/usr}

 (mkdir -p $builddir;
  set -x
  cd $builddir;
  set -x
  cmake -Wno-dev -DCMAKE_INSTALL_PREFIX="$prefix" \
    -G "${SYSTEM:-MSYS} Makefiles" \
    -DCMAKE_C_COMPILER="${CC-gcc}" \
    ${VERBOSE_MAKEFILE:+-DCMAKE_VERBOSE_MAKEFILE="$VERBOSE_MAKEFILE"} \
    -DCMAKE_BUILD_TYPE="${TYPE:-RelWithDebInfo}" \
  "$@" \
   ../..)
}

get-libdir() {
  ${CC:-gcc} -print-search-dirs |sed -n '/libraries/ { s,^libraries: =,, ; s,:,\n,g; p }'|grep -v -E '(/\.\.|/gcc|/[0-9]/|^/lib)' |sed -n '1 { s,/$,,  ; p }'
}

dietcfg() {
  unset builddir
  build=$(gcc -dumpmachine)
  host=${build//gnu/dietlibc}
  case "$CC $*" in
    *-m32*) host=i686-${host#*-} ;;
    *i[3-6]86*) host=i686-${host#*-} ;;
  esac
  case "$host" in
  i?86*) a=i686 m="-m32" ;;
  *) a=x86_64 m="-m64" ;;
  esac
  DIET=/opt/diet/bin-$a/diet
  prefix=${DIET%%/bin*}
  bindir=${prefix}/bin-${host%%-*}
  libdir=${prefix}/lib-${host%%-*}
  
  builddir="build/cmake-diet" 
  
  CC="i686-linux-dietlibc-gcc" \
  CXX="i686-linux-dietlibc-g++" \
  build="$host"  \
  cfg \
  "$@"
}
