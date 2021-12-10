if ! type realpath 2>/dev/null >/dev/null; then
realpath() {
   (DIR=`dirname "$1"`
    BASE=`basename "$1"`
    test -d "$DIR/$BASE" && DIR="$DIR/$BASE" BASE=
    cd -P "$DIR" && echo "$PWD${BASE:+/$BASE}")
  }
  relsrcdir=../..
fi

cfg() { 
 (: ${build=$(gcc -dumpmachine)}

  case "$build" in
    *--*) build=${build%%--*}-${build#*--} ;;
  esac

  : ${host:=$build}
  : ${builddir=build/$build}

  : ${prefix:=/usr/local}

  mkdir -p $builddir;
  : ${relsrcdir=`realpath --relative-to "$builddir" .`}

  if [ "${DEBUG:-0}" -eq 1 ]; then
    debug="--enable-debug"
  else 
    debug="--disable-debug"
  fi

  cd $builddir
  ${CONFIGURE:-"$relsrcdir"/configure} \
    ${build:+--build="$build"} \
    ${host:+--host="$host"} \
        --prefix="$prefix" \
        ${sysconfdir:+--sysconfdir="$sysconfdir"} \
        ${localstatedir:+--localstatedir="$localstatedir"} \
        --enable-{color,dependency-tracking} \
        $debug \
        "$@" &&
  echo "Configured in '$builddir'" 1>&2)
}
 
cfg-android()
{
 (build=$(cc -dumpmachine)
  host=arm-linux-androideabi
  : ${builddir=build/$host}

  CC="$host-gcc" \
  CXX="$host-g++" \
  RANLIB="$host-ranlib" \
  LD="$host-ld" \
  PKG_CONFIG_PATH=/opt/${host}/sysroot/usr/lib/pkgconfig:/opt/${host}/sysroot/usr/share/pkgconfig \
  TOOLCHAIN=/opt/cmake-toolchains/android.cmake \
  prefix=/opt/$host/sysroot/usr \
  CMAKE_PREFIX_PATH=/opt/$host/sysroot/usr \
  cfg "$@" --build="$build" --host="$host" )
}

cfg-android64() 
{
  (: ${builddir=build/android64}
  : ${host="aarch64-linux-android"}
  : ${prefix=/opt/aarch64-linux-android/sysroot/usr}
  
  libdir=$prefix/lib \
  CC="aarch64-linux-android-gcc" \
  CXX="aarch64-linux-android-g++" \
  cfg \
   "$@"
    )
}

cfg-diet() {
 (: ${build=$(${CC:-gcc} -dumpmachine)}
  : ${host=${build/-gnu/-diet}}
  : ${builddir=build/$host}
  prefix=/opt/diet
  
  libdir=/opt/diet/lib-${host%%-*} \
  bindir=/opt/diet/bin-${host%%-*} \
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


cfg-mingw() {
 (: ${build=$(gcc -dumpmachine)}
  : ${host=${build%%-*}-w64-mingw32}
  : ${prefix=/usr/$host/sys-root/mingw}
  : ${builddir=build/$host}
  
  bindir=$prefix/bin \
  libdir=$prefix/lib \
  cfg \
    "$@")
}

cfg-musl() {
 (: ${build=$(${CC:-gcc} -dumpmachine)}
  : ${host=${build/-gnu/-musl}}
  : ${host=${host/-pc-/-}}
  : ${builddir=build/$host}
  prefix=/opt/musl
  : ${includedir=$prefix/include}
  : ${libdir=$prefix/lib}
  : ${bindir=$prefix/bin}
  
  CC=musl-gcc \
  AR=ar \
  RANLIB=ranlib \
  PKG_CONFIG=musl-pkg-config \
  cfg \
    --with-{neon,lib{iconv,intl}-prefix}=/opt/musl \
    --disable-nls \
    --with-ssl=openssl \
    --without-included-gettext \
    --without-libproxy \
    "$@")
}

cfg-tcc() {
 (: ${build=$(cc -dumpmachine)}
  : ${host=${build/-gnu/-tcc}}
  : ${builddir=build/$host}
  : ${prefix=/usr}
  includedir=/usr/lib/$build/tcc/include \
  libdir=/usr/lib/$build/tcc/ \
  bindir=/usr/bin \
  CC=${TCC:-tcc} \
  cfg \
    "$@")
}
