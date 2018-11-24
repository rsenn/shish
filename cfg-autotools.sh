if ! type realpath 2>/dev/null >/dev/null; then
realpath() {
   (DIR=`dirname "$1"`
    BASE=`basename "$1"`
    test -d "$DIR/$BASE" && DIR="$DIR/$BASE" BASE=
    cd -P "$DIR" && echo "$PWD${BASE:+/$BASE}")
  }
  relsrcdir=../..
fi

cfg () 
{ 
    : ${build=$(gcc -dumpmachine)}

    case "$build" in
      *--*) build=${build%%--*}-${build#*--} ;;
    esac

    : ${host:=$build}
    : ${builddir=build/$build}

    : ${prefix:=/usr}

    mkdir -p $builddir;
    : ${relsrcdir=`realpath --relative-to "$builddir" .`}

    ( set -x; cd $builddir;
    "$relsrcdir"/configure \
      ${build:+--build="$build"} \
      ${host:+--host="$host"} \
          --prefix="$prefix" \
          ${sysconfdir:+--sysconfdir="$sysconfdir"} \
          ${localstatedir:+--localstatedir="$localstatedir"} \
          --enable-{silent-rules,color,dependency-tracking} \
          --enable-debug \
          "$@"
    )
#         #grep -r '\-O[0-9]' $builddir -lI |xargs sed -i 's,-O[1-9],-ggdb -O0,'


}

cfg-android () 
{
  (builddir=build/android
  host="arm-linux-androideabi"
  prefix=/opt/arm-linux-androideabi/sysroot/usr
  libdir=$prefix/lib

  CC="arm-linux-androideabi-gcc" \
  CXX="arm-linux-androideabi-g++" \
  cfg \
   "$@"
    )
}

cfg-android64() 
{
  (builddir=build/android64
  host="aarch64-linux-android"
  prefix=/opt/aarch64-linux-android/sysroot/usr
  libdir=$prefix/lib

  CC="aarch64-linux-android-gcc" \
  CXX="aarch64-linux-android-g++" \
  cfg \
   "$@"
    )
}

diet-cfg() {
 (build=$(${CC:-gcc} -dumpmachine)
  host=${build/-gnu/-dietlibc}
  builddir=build/$host
  prefix=/opt/diet
  libdir=/opt/diet/lib-${host%%-*}
  bindir=/opt/diet/bin-${host%%-*}
  
  CC="diet-gcc" \
  PKG_CONFIG="$host-pkg-config" \
  LIBS="${LIBS:+$LIBS }-liconv -lpthread" \
  cfg \
    --with-{neon,lib{iconv,intl}-prefix}=/opt/diet \
    --disable-nls \
    --with-ssl=openssl \
    --without-included-gettext \
    --without-libproxy \
    "$@")
}


mingw-cfg() {
 (build=$(gcc -dumpmachine)
  host=${build%%-*}-w64-mingw32
  prefix=/usr/$host/sys-root/mingw
  
  builddir=build/$host \
  bindir=$prefix/bin \
  libdir=$prefix/lib \
  cfg \
    "$@")
}
musl-cfg() {
 (build=$(${CC:-gcc} -dumpmachine)
  host=${build/-gnu/-musl}
  host=${host/-pc-/-}
  : ${builddir=build/$host}
  : ${prefix=/usr/lib/musl}
  : ${includedir=$prefix/include}
  : ${libdir=$prefix/lib}
  : ${bindir=$prefix/bin}
  
  CC=musl-gcc \
  AR=ar \
  RANLIB=ranlib \
  PKG_CONFIG=musl-pkg-config \
  cfg \
    --with-{neon,lib{iconv,intl}-prefix}=/opt/diet \
    --disable-nls \
    --with-ssl=openssl \
    --without-included-gettext \
    --without-libproxy \
    "$@")
}
