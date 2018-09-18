cfg() { 
  case $(uname -o) in
    MSys|MSYS|Msys) SYSTEM="MSYS" ;;
    *) SYSTEM="Unix" ;;
  esac

  for builddir in build/cmake{-debug,,-release,-minsizerel}
  do
    case "$builddir" in
            *debug*) TYPE=Debug ;;
            *reease*) TYPE=Release ;;
            *minsiz*) TYPE=MinSizeRel ;;
            *) TYPE=RelWithDebInfo ;;
    esac
 (mkdir -p $builddir
  set -x
  cd $builddir
  cmake -Wno-dev -DCMAKE_INSTALL_PREFIX=$(get-prefix) \
    -G "${SYSTEM:-MSYS} Makefiles" \
    ${VERBOSE+-DCMAKE_VERBOSE_MAKEFILE=TRUE} \
    -DCMAKE_BUILD_TYPE="${TYPE:-RelWithDebInfo}" \
    "$@" \
  ../..)
  done
}
