cfg() {
 (if type gcc 2>/dev/null >/dev/null && type g++ 2>/dev/null >/dev/null; then
    : ${CC:=gcc} ${CXX:=g++}
  elif type clang 2>/dev/null >/dev/null && type clang++ 2>/dev/null >/dev/null; then
    : ${CC:=clang} ${CXX:=clang++}
  fi

  : ${build:=`$CC -dumpmachine | sed 's|-pc-|-|g'`}

  if [ -z "$host" -a -z "$builddir" ]; then
    host=$build
    case "$host" in
      x86_64-w64-mingw32) host="$host"; : ${builddir=build/$host}; : ${prefix=/mingw64} ;;
      i686-w64-mingw32) host="$host"; : ${builddir=build/$host}; : ${prefix=/mingw32} ;;
      x86_64-pc-*) host="$host"; : ${builddir=build/$host}; : ${prefix=/usr} ;;
      i686-pc-*) host="$host"; : ${builddir=build/$host}; : ${prefix=/usr} ;;
    esac
  fi

  : ${prefix:=/usr/local}
  : ${libdir:=$prefix/lib}
  [ -d "$libdir/$host" ] && libdir=$libdir/$host

  if [ -e "$TOOLCHAIN" ]; then
    cmakebuild=$(basename "$TOOLCHAIN" .cmake)
    cmakebuild=${cmakebuild%.toolchain}
    cmakebuild=${cmakebuild#toolchain-}
    : ${builddir=build/$cmakebuild}
  else
   : ${builddir=build/$host}
  fi
  case "$host" in
    *msys*) ;;
    *) test -n "$builddir" && builddir=`echo $builddir | sed 's|-pc-|-|g'` ;;
  esac
  
  case $(uname -o) in
   # MSys|MSYS|Msys) SYSTEM="MSYS" ;;
    *) SYSTEM="Unix" ;;
  esac

  case "$STATIC:$TYPE" in
    YES:*|yes:*|y:*|1:*|ON:*|on:* | *:*[Ss]tatic*) set -- "$@" \
      -DENABLE_PIC=OFF ;;
  esac

  [ -n "$PKG_CONFIG_PATH" ] && echo "PKG_CONFIG_PATH=$PKG_CONFIG_PATH" 1>&2
  [ -n "$PKG_CONFIG" ] && case "$PKG_CONFIG" in
     */*) ;;
     *) PKG_CONFIG=$(which "$PKG_CONFIG") ;; 
  esac
  : ${generator:="CodeLite - Unix Makefiles"}

  mkdir -p $builddir
  : ${relsrcdir=`realpath --relative-to "$builddir" .`}
  : set -x
  cd "${builddir:-.}"
  IFS="$IFS "
 set -- -Wno-dev \
    -G "$generator" \
    ${prefix:+-DCMAKE_INSTALL_PREFIX="$prefix"} \
    ${VERBOSE:+-DCMAKE_VERBOSE_MAKEFILE=${VERBOSE:-OFF}} \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DCMAKE_BUILD_TYPE="${TYPE:-Debug}" \
    ${CC:+-DCMAKE_C_COMPILER="$CC"} \
    ${CXX:+-DCMAKE_CXX_COMPILER="$CXX"} \
    ${PKG_CONFIG:+-DPKG_CONFIG_EXECUTABLE="$PKG_CONFIG"} \
    ${TOOLCHAIN:+-DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN"} \
    ${CC:+-DCMAKE_C_COMPILER="$CC"} \
    ${CXX:+-DCMAKE_CXX_COMPILER="$CXX"} \
    ${MAKE:+-DCMAKE_MAKE_PROGRAM="$MAKE"} \
    "$@" \
    $relsrcdir 
  eval "${CMAKE:-cmake} \"\$@\""
 ) 2>&1 |tee "${builddir##*/}.log"
}

cfg-linux32() {
 (build=$(gcc -dumpmachine | sed 's|-pc-|-|g')
  host=${build%%-*}-linux-gnu
  host=i686-${host#*-}
  
  if type i686-linux-gnu-gcc 2>/dev/null >/dev/null; then
    CC=i686-linux-gnu-gcc
    export CC
  elif type i686-pc-linux-gnu-gcc 2>/dev/null >/dev/null; then
    CC=i686-pc-linux-gnu-gcc
    export CC
  else
    : ${CC="gcc"}
    CFLAGS="${CFLAGS:+$CFLAGS }-m32"
    export CC CFLAGS
  fi
  
  export PKG_CONFIG_PATH=/usr/lib/i386-linux-gnu/pkgconfig

  : ${builddir=build/$host}
  cfg \
    -DCMAKE_SYSTEM_LIBRARY_PATH=/usr/lib/i386-linux-gnu \
    "$@")
}


cfg-android ()
{
  (: ${builddir=build/android}
    cfg \
  -DCMAKE_INSTALL_PREFIX=/opt/arm-linux-androideabi/sysroot/usr \
  \
  -DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN:-/opt/android-cmake/android.cmake} \
  -DANDROID_NATIVE_API_LEVEL=21 \
  -DPKG_CONFIG_EXECUTABLE=arm-linux-androideabi-pkg-config \
  -DCMAKE_PREFIX_PATH=/opt/arm-linux-androideabi/sysroot/usr \
  -DCMAKE_MAKE_PROGRAM=/usr/bin/make \
   -DCMAKE_MODULE_PATH="/opt/OpenCV-3.4.1-android-sdk/sdk/native/jni/abi-armeabi-v7a" \
   -DOpenCV_DIR="/opt/OpenCV-3.4.1-android-sdk/sdk/native/jni/abi-armeabi-v7a" \
   "$@"
    )
}

cfg-diet() {
 (: ${build=$(${CC:-gcc} -dumpmachine | sed 's|-pc-|-|g')}
  : ${host=${build/-gnu/-diet}}
  : ${prefix=/opt/diet}
  : ${libdir=/opt/diet/lib-${host%%-*}}
  : ${bindir=/opt/diet/bin-${host%%-*}}
  : ${CC=gcc}

  export CC

  if type pkgconf >/dev/null; then
    export PKG_CONFIG=`type pkgconf 2>&1 |sed 's,.* is ,,'`
  elif type pkg-config >/dev/null; then
    export PKG_CONFIG=`type pkg-config 2>&1 |sed 's,.* is ,,'`
  fi

  : ${PKG_CONFIG_PATH="$libdir/pkgconfig"}; export PKG_CONFIG_PATH
  
  : ${builddir=build/${host%-*}-diet}
  : ${prefix=/opt/diet}

  export builddir prefix
  cfg \
    -DCMAKE_INSTALL_PREFIX="$prefix" \
    -DENABLE_SHARED=OFF \
    -DCMAKE_FIND_ROOT_PATH="$prefix" \
    -DCMAKE_SYSTEM_LIBRARY_PATH="$prefix/lib-${host%%-*}" \
      ${launcher:+-DCMAKE_C_COMPILER_LAUNCHER="$launcher"} \
      ${launcher:+-DCMAKE_C_LINKER_LAUNCHER="$launcher"} \
  -DPKG_CONFIG_EXECUTABLE="$PKG_CONFIG" \
    "$@")
}

cfg-diet64() {
 (build=$(gcc -dumpmachine | sed 's|-pc-|-|g')
  host=${build%%-*}-linux-diet
  host=x86_64-${host#*-}

  PKG_CONFIG_PATH=/opt/diet/lib-x86_64/pkgconfig
  : ${builddir=build/$host}

  if test -e /usr/lib/x86_64-linux-gnu/diet/bin/diet; then
    launcher=/usr/lib/x86_64-linux-gnu/diet/bin/diet
  else
    launcher=diet
  fi
  
  export CFLAGS PKG_CONFIG_PATH launcher builddir

  cfg-diet \
    -DCMAKE_SYSTEM_LIBRARY_PATH=/opt/diet/lib-x86_64 \
  "$@")
}

cfg-diet32() {
 (build=$(gcc -dumpmachine | sed 's|-pc-|-|g')
  host=${build%%-*}-linux-diet
  host=i686-${host#*-}
 
  if type ${host%-diet}-gnu-gcc >/dev/null; then
    : ${CC=${host%-diet}-gnu-gcc}
  else
    CFLAGS="${CFLAGS:+$CFLAGS }-m32"
  fi

  PKG_CONFIG_PATH=/opt/diet/lib-i386/pkgconfig

  if test -e /usr/lib/i386-linux-gnu/diet/bin/diet; then
    launcher=/usr/lib/i386-linux-gnu/diet/bin/diet
  else
    launcher=diet
  fi

  : ${builddir=build/$host}

  export CC CFLAGS PKG_CONFIG_PATH launcher builddir

  cfg-diet \
    -DCMAKE_SYSTEM_LIBRARY_PATH=/opt/diet/lib-i386 \
    "$@")
}

cfg-mingw() {
 (build=$(gcc -dumpmachine)
  : ${host=${build%%-*}-w64-mingw32}
  : ${prefix=/usr/$host/sys-root/mingw}

  case "$host" in
    x86_64-*) : ${TOOLCHAIN=/opt/cmake-toolchains/mingw64.cmake} ;;
    *) : ${TOOLCHAIN=/opt/cmake-toolchains/mingw32.cmake} ;;
  esac

  : ${PKG_CONFIG_PATH=/usr/${host}/sys-root/mingw/lib/pkgconfig}

  export TOOLCHAIN PKG_CONFIG_PATH
  
  : ${builddir=build/$host}
  bindir=$prefix/bin \
  libdir=$prefix/lib \
  cfg \
    "$@")
}
cfg-mingw32() {
  host=i686-w64-mingw32 cfg-mingw "$@"
}
cfg-mingw64() {
  host=x86_64-w64-mingw32 cfg-mingw "$@"
}

cfg-tcc() {
 (build=$(cc -dumpmachine | sed 's|-pc-|-|g')
  host=${build/-gnu/-tcc}
  : ${builddir=build/$host}
  prefix=/usr
  includedir=/usr/lib/$build/tcc/include
  libdir=/usr/lib/$build/tcc/
  bindir=/usr/bin

  CC=${TCC:-tcc} \
  cfg \
    "$@")
}

cfg-musl() {
 (: ${build=$(${CC:-gcc} -dumpmachine | sed 's|-pc-|-|g')}
  host=${build%-*}-musl

 : ${prefix=/opt/musl}
 : ${includedir=$prefix/include/$host}
 : ${libdir=$prefix/lib/$host}
 : ${bindir=$prefix/bin/$host}
 : ${builddir=build/$host}

  CC=musl-gcc \
  PKG_CONFIG=musl-pkg-config \
  cfg \
    -DENABLE_SHARED=OFF \
    "$@")
}


cfg-musl64() {
 (build=$(gcc -dumpmachine | sed 's|-pc-|-|g')
  host=${build%%-*}-linux-musl
  host=x86_64-${host#*-}

  : ${builddir=build/$host}
  
  CC="musl-gcc" \
  cfg-musl \
  "$@")
}

cfg-musl32() {
 (build=$(gcc -dumpmachine | sed 's|-pc-|-|g')
  host=$(echo "$build" | sed "s|x86_64|i686| ; s|-gnu|-musl|")

  : ${builddir=build/$host}

  CC="musl-gcc" \
  CFLAGS="${CFLAGS:+$CFLAGS }-m32" \
  cfg-musl \
  "$@")
}

cfg-msys() {
 (echo "host: $host"
  build=$(gcc -dumpmachine)
  : ${host=${build%%-*}-pc-msys}
  : ${prefix=/usr/$host/sysroot/usr}
echo "host: $host"
  : ${PKG_CONFIG_PATH=/usr/${host}/sysroot/usr/lib/pkgconfig}

  export PKG_CONFIG_PATH

  case "$host" in
    x86_64*) TOOLCHAIN=/opt/cmake-toolchains/msys64.cmake ;;
   *) TOOLCHAIN=/opt/cmake-toolchains/msys32.cmake ;;
  esac
  export TOOLCHAIN
  echo "builddir: $builddir"

  : ${builddir=build/$host}

  bindir=$prefix/bin \
  libdir=$prefix/lib \
  host=$host \
  build=$build \
  cfg \
    "$@")
}

cfg-msys32() {
  host=i686-pc-msys \
    cfg-msys "$@"
}

cfg-msys64() {
  host=x86_64-pc-msys \
    cfg-msys "$@"
}


cfg-termux()
{
  (: ${builddir=build/termux}
    cfg \
  -DCMAKE_INSTALL_PREFIX=/data/data/com.termux/files/usr \
  \
  -DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN:-/opt/android-cmake/android.cmake} \
  -DANDROID_NATIVE_API_LEVEL=21 \
  -DPKG_CONFIG_EXECUTABLE=arm-linux-androideabi-pkg-config \
  -DCMAKE_PREFIX_PATH=/data/data/com.termux/files/usr \
  -DCMAKE_MAKE_PROGRAM=/usr/bin/make \
   -DCMAKE_MODULE_PATH="/data/data/com.termux/files/usr/lib/cmake" \
   "$@"
    )
}

cfg-tcc() {
 (build=$(cc -dumpmachine | sed 's|-pc-|-|g')
  host=${build/-gnu/-tcc}
  : ${builddir=build/$host}
  prefix=/usr
  includedir=/usr/lib/$build/tcc/include
  libdir=/usr/lib/$build/tcc/
  bindir=/usr/bin

  CC=${TCC:-tcc} \
  cfg \
    "$@")
}
  
cfg-android64 () 
{ 
    ( : ${builddir=build/android64};
    cfg -DCMAKE_INSTALL_PREFIX=/opt/aarch64-linux-android64eabi/sysroot/usr -DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN:-/opt/android64-cmake/android64.cmake} -DANDROID_NATIVE_API_LEVEL=21 -DPKG_CONFIG_EXECUTABLE=aarch64-linux-android64eabi-pkg-config -DCMAKE_PREFIX_PATH=/opt/aarch64-linux-android64eabi/sysroot/usr -DCMAKE_MAKE_PROGRAM=/usr/bin/make -DCMAKE_MODULE_PATH="/opt/OpenCV-3.4.1-android64-sdk/sdk/native/jni/abi-armeabi-v7a" -DOpenCV_DIR="/opt/OpenCV-3.4.1-android64-sdk/sdk/native/jni/abi-armeabi-v7a" "$@" )
}

cfg-aarch64() {
 (: ${build=$(cc -dumpmachine | sed 's|-pc-|-|g')}
  : ${host=aarch64-${build#*-}}
  : ${builddir=build/$host}

  : ${prefix=/usr/aarch64-linux-gnu/sysroot/usr}

  : ${TOOLCHAIN=/opt/cmake-toolchains/aarch64-linux-gnu.toolchain.cmake}
  export prefix TOOLCHAIN

  PKG_CONFIG=$(which ${host}-pkg-config) \
  cfg "$@")
}

cfg-emscripten() {
 (: ${builddir=build/emscripten}
  CC="emcc" CXX="em++" \
  LDFLAGS="-sWASM=1 -sLLD_REPORT_UNDEFINED" \
  CFLAGS="-DEMSCRIPTEN=1" \
  CXXFLAGS="-DEMSCRIPTEN=1" \
  TOOLCHAIN="${EMSCRIPTEN:=$(dirname $(which emcc))}/cmake/Modules/Platform/Emscripten.cmake" \
  cfg \
    -DCMAKE_EXE_LINKER_FLAGS="-s WASM=1 -sEXPORTED_RUNTIME_METHODS=['callMain'] -sINVOKE_RUN=0" \
    -DCMAKE_EXECUTABLE_SUFFIX=".html" \
    -DENABLE_SHARED=OFF \
    -DENABLE_PIC=FALSE \
    "$@")
}

cfg-wasm() {
 (builddir=build/wasm32-clang
  CC="clang" CXX="clang++" \
  CFLAGS="--target=wasm32" \
  CXXFLAGS="--target=wasm32" \
  LDFLAGS="--target=wasm32" \
  cfg \
    -DCMAKE_SYSTEM_NAME=Generic \
    -DCMAKE_SYSTEM_PROCESSOR=wasm32 \
    -DENABLE_SHARED=OFF \
    -DENABLE_PIC=FALSE \
    "$@")
}

cfg-wasi() {
 (: ${WASI_SDK_PREFIX:=/opt/wasi-sdk}
  builddir=build/wasi
  CC="${WASI_SDK_PREFIX}/bin/clang"
  CXX="${WASI_SDK_PREFIX}/bin/clang++"
  CFLAGS="-D_WASI_EMULATED_SIGNAL" 
  CXXFLAGS="-D_WASI_EMULATED_SIGNAL" 
  LDFLAGS="-lwasi-emulated-signal"
  export CC CXX CFLAGS CXXFLAGS LDFLAGS
  cfg \
    -DCMAKE_SYSTEM_NAME=WASI \
    -DCMAKE_SYSTEM_PROCESSOR=wasm32 \
    -DCMAKE_SYSROOT="${WASI_SDK_PREFIX}/share/wasi-sysroot" \
    -DENABLE_SHARED=OFF \
    -DENABLE_PIC=FALSE \
    "$@")
}
