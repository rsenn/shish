cfg () 
{ 
    : ${build=$(gcc -dumpmachine)}


  cfg32

    builddir=build/$host
    mkdir -p $builddir;
    ( set -x; cd $builddir;
    ../../configure \
          --prefix="$prefix" \
          ${sysconfdir+--sysconfdir="$sysconfdir"} \
          ${localstatedir+--localstatedir="$localstatedir"} \
          --disable-{silent-rules,dependency-tracking} \
          --disable-unittests \
          --disable-asciidoc \
          --disable-{systemd,zstd} \
          "$@"
    ) 2>&1 |tee cfg.log

}

dietcfg() {
  unset builddir
  build=$(gcc -dumpmachine)
  host=${build//gnu/dietlibc}

  cfg32

  DIET=/opt/diet/bin-$a/diet
  prefix=${DIET%%/bin*}
  bindir=${prefix}/bin-${host%%-*}
  libdir=${prefix}/lib-${host%%-*}
 

    CC="$DIET -Os gcc $m" CFLAGS="-pipe -Os -falign-functions=1 -falign-jumps=1 -falign-loops=1 -D_GNU_SOURCE=1 -D_BSD_SOURCE=1 -D_ATFILE_SOURCE=1" \
    CXX="$DIET -Os gcc $m" CXXFLAGS="-pipe -Os -falign-functions=1 -falign-jumps=1 -falign-loops=1 -D_GNU_SOURCE=1 -D_BSD_SOURCE=1 -D_ATFILE_SOURCE=1" \
  cfg \
    --prefix="$prefix" \
    --libdir="$libdir" \
    --bindir="$bindir" \
    --sbindir="$bindir" \
    --{build,host}="$host" \
    "$@"
}


cfg32() {
  : ${host=$build}
  case "$CC $*" in
      *-m32*) host=i686-${host#*-} libdir=/usr/lib32 cpu=x86_64 ;;
      *i[3-6]86*) host=i686-${host#*-} cpu=i386 ;;
  esac
  case "$host" in
    i?86*) a=i686 cpu=i386 m="-m32" l="32" ;;
    *) a=x86_64 cpu=x86_64 m="-m64" l="" ;;
  esac
}
