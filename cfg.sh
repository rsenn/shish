cfg () 
{ 
    case $(uname -o) in
        MSys|MSYS|Msys) SYSTEM="MSYS" ;;
        *) SYSTEM="Unix" ;;
    esac


    ( mkdir -p $builddir;
    cd $builddir;
    set -x
    cmake -Wno-dev -DCMAKE_INSTALL_PREFIX=$(get-prefix) \
    -G "${SYSTEM:-MSYS} Makefiles" \
    -DCMAKE_VERBOSE_MAKEFILE=TRUE \
    -DCMAKE_BUILD_TYPE="${TYPE:-RelWithDebInfo}" \
    "$@" \
     ../.. )
}
