cfg () 
{ 
    build=$(gcc -dumpmachine)

    case "$CC $*" in
       *clang*) build=${build/gnu/clang} ;;
    esac
    case "$*" in
       *debug*) build=${build}-debug ;;
    esac
    builddir=build/$build
    mkdir -p $builddir;
    ( cd $builddir;
    ../../configure \
          --prefix=/usr \
          --sysconfdir=/etc \
          --localstatedir=/var \
          --disable-{silent-rules,maintainer-mode} \
          "$@"
    )

}

