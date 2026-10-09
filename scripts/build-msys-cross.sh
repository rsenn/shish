#!/bin/sh
# build-msys-cross.sh - build binutils + gcc cross compilers from a Linux host to
# i686-pc-msys and x86_64-pc-msys, and wrap each one in a .deb.
#
# Recipe: the MSYS2-packages PKGBUILDs (cloned to third_party/msys2-packages).
#   from binutils/PKGBUILD and gcc/PKGBUILD this script takes
#     pkgver, the tarball URL and sha256, the patches applied in prepare() (in order),
#     the prepare() source hacks, and the ./configure flag list;
#   it drops the flags that only make sense for a native build (--build, --prefix,
#   --libexecdir, --enable-bootstrap, --enable-install-libiberty) and adds the cross ones
#   (--target, --with-sysroot, --with-build-sysroot).
#
# Layout (PREFIX is /usr or /usr/local):
#   $PREFIX/bin/<target>-gcc ...        $PREFIX/<target>/bin/{as,ld,...}
#   $PREFIX/<target>/sysroot/usr/...    headers + libs from the MSYS2 packages
#                                       (scripts/get-msys-packages.sh fills it)
#
# Steps per target, each skipped when its stamp exists (-f redoes them). An interrupted
# binutils/gcc step continues in its build tree; downloads resume (*.part).
# Steps per target:
#   sysroot -> binutils -> gcc -> deb          (output: $WORK/<name>_<ver>_<arch>.deb)
#
# -m builds the same toolchain as statically linked musl binaries (stage-<target>-musl,
# package gcc-<target>-musl, no runtime dependencies). gcc is C++, and musl-tools only has a
# C wrapper, so -m first builds, once, in $WORK:
#   musl-cc     a x86_64-linux-musl gcc (C, C++) on a sysroot made of /usr/include/x86_64-linux-musl,
#               /usr/lib/x86_64-linux-musl and the kernel headers
#   musl-deps   static gmp, mpfr, mpc, isl (versions from gcc's contrib/prerequisites.sha512)
# The musl binutils/gcc have no plugin support (ld cannot dlopen the LTO plugin).
#
# POSIX sh. External: git curl tar(GNU, xz) patch make awk sed sha256sum sha512sum dpkg-deb
#                     and gcc, gmp/mpfr/mpc/isl/zlib/zstd development files on the host;
#                     -m also needs musl-gcc (package musl-tools).

TOP=$(cd "$(dirname "$0")/.." && pwd)
PREFIX=/usr
WORK=$TOP/build/msys-cross
REPO=$TOP/third_party/msys2-packages
REPO_URL=${MSYS2_PACKAGES_URL:-https://github.com/msys2/MSYS2-packages}
TARGETS=
FORCE=false KEEP=false MUSL=false SUFFIX=
MUSL_CC=${MUSL_CC:-musl-gcc}
JOBS=$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)
PKGREL=${PKGREL:-1}

usage() {
  echo "Usage: ${0##*/} [-p prefix] [-t target]... [-j jobs] [-w workdir] [-m] [-f] [-k]

  -p prefix   install prefix of the packaged toolchain (default /usr; /usr/local works too)
  -t target   i686-pc-msys or x86_64-pc-msys, or just i686 / x86_64 (default: both)
  -j jobs     parallel make jobs (default: number of CPUs)
  -w workdir  sources, build trees and .deb output (default build/msys-cross)
  -m          statically linked musl-hosted binutils and gcc (see above)
  -f          redo every step
  -k          keep the build trees (default: delete them after a successful install)

  environment: MSYS2_PACKAGES_URL PKGREL MUSL_CC" 1>&2
  exit 1
}

while getopts p:t:j:w:mfkh opt; do
  case $opt in
    p) PREFIX=$OPTARG ;;
    t) case $OPTARG in
         i686|i686-pc-msys) TARGETS="$TARGETS i686-pc-msys" ;;
         x86_64|x86_64-pc-msys) TARGETS="$TARGETS x86_64-pc-msys" ;;
         *) usage ;;
       esac ;;
    j) JOBS=$OPTARG ;;
    w) WORK=$OPTARG ;;
    m) MUSL=true SUFFIX=-musl ;;
    f) FORCE=true ;;
    k) KEEP=true ;;
    *) usage ;;
  esac
done
[ -n "$TARGETS" ] || TARGETS="i686-pc-msys x86_64-pc-msys"
case $WORK in /*) ;; *) WORK=$PWD/$WORK ;; esac

DL=$WORK/dl SRC=$WORK/src STAMP=$WORK/stamp
MCC=$WORK/musl-cc MS=$WORK/musl-sysroot MDEPS=$WORK/musl-deps
mkdir -p "$DL" "$SRC" "$STAMP" || exit 1

log() { echo "== $*" 1>&2; }
die() { echo "${0##*/}: $*" 1>&2; exit 1; }

# --- PKGBUILD readers --------------------------------------------------------

# pk_var NAME VAR: value of a plain VAR=value line
pk_var() { sed -n "s/^$2=//p" "$REPO/$1/PKGBUILD" | head -n 1 | tr -d "'\""; }

# pk_url NAME: first source URL with ${pkgver} expanded and the {,.sig} brace dropped
pk_url() {
  sed -n 's/^source=(\(https[^ ]*\).*/\1/p' "$REPO/$1/PKGBUILD" | head -n 1 |
    sed -e "s/[\$]{pkgver}/$(pk_var "$1" pkgver)/g" -e 's/{.*//'
}

# pk_sha NAME: first sha256 (the tarball's)
pk_sha() { sed -n "s/^sha256sums=('\([0-9a-f]*\)'.*/\1/p" "$REPO/$1/PKGBUILD" | head -n 1; }

# pk_patches NAME: the *.patch files named in prepare(), in order, comments skipped
pk_patches() {
  awk '/^prepare\(\)/ { on = 1; next }
       on && /^}/ { exit }
       on && !/^[ \t]*#/ {
         while(match($0, /[0-9][0-9][0-9][0-9]-[^ \t\\"\/]*\.patch/)) {
           print substr($0, RSTART, RLENGTH); $0 = substr($0, RSTART + RLENGTH) } }' "$REPO/$1/PKGBUILD"
}

# pk_flags NAME ARCH: one ./configure flag per line, native-only flags removed
pk_flags() {
  awk '/\/configure[ \t]*\\$/ { on = 1; next }
       on { cont = ($0 ~ /\\[ \t]*$/); sub(/\\[ \t]*$/, ""); sub(/^[ \t]+/, ""); if($0 != "") print
            if(!cont) exit }' "$REPO/$1/PKGBUILD" |
    grep -v -e '^--build' -e '^--prefix' -e '^--libexecdir' -e '^--enable-bootstrap' \
            -e '{build,host,target}' -e '^--enable-install-libiberty' |
    sed "s/\\\${_arch_conf}//; s/\\\${_arch}/$(arch_cpu "$2")/" | grep -v '^$'
}

# arch_cpu ARCH, arch_conf ARCH: the 'case ${CARCH}' of gcc/PKGBUILD
arch_cpu() { case $1 in i686) echo pentium4 ;; *) echo nocona ;; esac; }
arch_conf() { case $1 in i686) echo --disable-sjlj-exceptions ;; esac; }

# --- steps -------------------------------------------------------------------

fetch_repo() {
  if [ ! -d "$REPO/.git" ]; then
    log "cloning $REPO_URL -> $REPO"
    mkdir -p "$(dirname "$REPO")" &&
    git clone -q --depth 1 --filter=blob:none --sparse "$REPO_URL" "$REPO" || die "clone failed"
  fi
  git -C "$REPO" sparse-checkout set binutils gcc || die "sparse-checkout failed"
}

# prepare NAME: download, verify, unpack, patch (once; shared by both targets)
prepare() {
  p_ver=$(pk_var "$1" pkgver) p_url=$(pk_url "$1")
  p_tar=$DL/${p_url##*/} p_dir=$SRC/$1-$p_ver
  [ -f "$p_dir/.prepared" ] && ! $FORCE && return 0
  [ -s "$p_tar" ] || { log "download $p_url"; curl -fL -C - -o "$p_tar.part" "$p_url" && mv "$p_tar.part" "$p_tar" || die "download failed: $p_url"; }
  echo "$(pk_sha "$1")  $p_tar" | sha256sum -c - >/dev/null || die "sha256 mismatch: $p_tar"
  rm -rf "$p_dir"; log "unpack $p_tar"
  tar -xf "$p_tar" -C "$SRC" || die "unpack failed"
  for p_patch in $(pk_patches "$1"); do
    log "patch $1: $p_patch"
    (cd "$p_dir" && patch -Np1 -s -i "$REPO/$1/$p_patch") || die "patch failed: $p_patch"
  done
  # the prepare() hacks of the PKGBUILDs
  [ "$1" = gcc ] && { rm -f "$p_dir/libgomp/config/cygwin/plugin-suffix.h"; echo "$p_ver" >"$p_dir/gcc/BASE-VER"; }
  for p_f in libiberty/configure $( [ "$1" = gcc ] && echo gcc/configure ); do
    sed "/ac_cpp=/s/\$CPPFLAGS/\$CPPFLAGS -O2/" "$p_dir/$p_f" >"$p_dir/$p_f.new" &&
      cat "$p_dir/$p_f.new" >"$p_dir/$p_f" && rm -f "$p_dir/$p_f.new"
  done
  # <arch>-pc-msys stays the alias (tool names, $PREFIX/<arch>-pc-msys), but every per-tool
  # target table only knows the triple MSYS2 builds with: config.sub maps it to <arch>-pc-cygwin
  find "$p_dir" -name config.sub | while read -r p_f; do
    [ -f "$p_f.real" ] || mv "$p_f" "$p_f.real"
    cat >"$p_f" <<'EOF'
#!/bin/sh
case $0 in */*) d=${0%/*} ;; *) d=. ;; esac
"$d/config.sub.real" "$@" | sed 's/-msys/-cygwin/'
EOF
    chmod +x "$p_f"
  done
  touch "$p_dir/.prepared"
}

# step TARGET NAME: run the function $2 unless its stamp exists (in a subshell: exports stay inside)
step() {
  [ -f "$STAMP/$KEY.$2" ] && ! $FORCE && return 0
  log "$KEY: $2"
  ( "step_$2" "$1" ) || die "$KEY: step $2 failed"
  touch "$STAMP/$KEY.$2"
}

# gstep NAME: the same for a step shared by all targets
gstep() {
  [ -f "$STAMP/$1" ] && ! $FORCE && return 0
  log "$1"
  ( "step_$1" ) || die "step $1 failed"
  touch "$STAMP/$1"
}

# host_env: the compiler that runs binutils/gcc themselves (-m: static musl)
host_env() {
  $MUSL || return 0
  CC=$MUSL_CC CFLAGS=-O2 CXXFLAGS=-O2 LDFLAGS=-static
  export CC CFLAGS CXXFLAGS LDFLAGS
}

# host_flags NAME ARCH: the PKGBUILD flags, minus the host-library ones that -m replaces
host_flags() {
  if $MUSL; then pk_flags "$@" | grep -v '^--with-system-zlib'; else pk_flags "$@"; fi
}

# step_musl_cc: x86_64-linux-musl gcc (C, C++) in $MCC, only as the C++ compiler for -m
step_musl_cc() {
  g_ver=$(pk_var gcc pkgver) g_dir=$WORK/build-musl-cc
  mkdir -p "$MS/usr/include" "$MS/usr/lib" "$MCC/x86_64-linux-musl/bin" || return 1
  for m_f in /usr/include/x86_64-linux-musl/* /usr/include/linux /usr/include/asm-generic; do
    ln -sf "$m_f" "$MS/usr/include/"
  done
  ln -sf /usr/include/x86_64-linux-gnu/asm "$MS/usr/include/asm"
  for m_f in /usr/lib/x86_64-linux-musl/*; do ln -sf "$m_f" "$MS/usr/lib/"; done
  for m_t in as ld ar nm ranlib strip objdump objcopy; do ln -sf "/usr/bin/$m_t" "$MCC/x86_64-linux-musl/bin/$m_t"; done
  if $FORCE || [ ! -f "$g_dir/Makefile" ]; then
    rm -rf "$g_dir"; mkdir -p "$g_dir" && cd "$g_dir" || return 1
    "$SRC/gcc-$g_ver/configure" --prefix="$MCC" --target=x86_64-linux-musl --with-sysroot="$MS" \
        --with-build-time-tools="$MCC/x86_64-linux-musl/bin" --enable-languages=c,c++ \
        --disable-multilib --disable-shared --disable-nls --disable-bootstrap --disable-lto --disable-plugin \
        --disable-libsanitizer --disable-libssp --disable-libquadmath --disable-libgomp \
        --disable-libatomic --disable-libitm --disable-libvtv --disable-libstdcxx-pch \
        --enable-threads=posix --with-system-zlib || return 1
  fi
  cd "$g_dir" && make -j"$JOBS" && make install || return 1
  # it must produce a static executable that runs here
  printf '#include <iostream>\nint main() { std::cout << "musl c++ ok\\n"; }\n' >"$g_dir/t.cc" &&
    "$MCC/bin/x86_64-linux-musl-g++" -static "$g_dir/t.cc" -o "$g_dir/t" && [ "$("$g_dir/t")" = "musl c++ ok" ] || return 1
  $KEEP || rm -rf "$g_dir"
}

# step_musl_deps: static gmp mpfr mpc isl in $MDEPS, built with musl-gcc
step_musl_deps() {
  g_ver=$(pk_var gcc pkgver) m_sums=$SRC/gcc-$g_ver/contrib/prerequisites.sha512 m_dir=$WORK/build-musl-deps
  rm -rf "$m_dir"; mkdir -p "$m_dir" "$MDEPS" || return 1
  for m_n in gmp mpfr mpc isl; do
    m_line=$(grep "  $m_n-" "$m_sums") || return 1
    m_file=${m_line##*  }
    [ -s "$DL/$m_file" ] || { curl -fL -C - -o "$DL/$m_file.part" "https://gcc.gnu.org/pub/gcc/infrastructure/$m_file" &&
                              mv "$DL/$m_file.part" "$DL/$m_file"; } || return 1
    echo "${m_line%%  *}  $DL/$m_file" | sha512sum -c - >/dev/null || { echo "sha512 mismatch: $m_file" 1>&2; return 1; }
    tar -xf "$DL/$m_file" -C "$m_dir" || return 1
    case $m_n in
      mpfr) m_opt="--with-gmp=$MDEPS" ;;
      mpc) m_opt="--with-gmp=$MDEPS --with-mpfr=$MDEPS" ;;
      isl) m_opt="--with-gmp-prefix=$MDEPS" ;;
      *) m_opt= ;;
    esac
    ( cd "$m_dir/$m_n"-* && CC=$MUSL_CC CFLAGS=-O2 ./configure --prefix="$MDEPS" --disable-shared --enable-static $m_opt &&
      make -j"$JOBS" && make install ) || return 1
  done
  $KEEP || rm -rf "$m_dir"
}

step_sysroot() {
  BASE=$STAGE$PREFIX SYSROOT_NAME=sysroot "$TOP/scripts/get-msys-packages.sh" -t "msys/${1%%-*}"
}

step_binutils() {
  b_ver=$(pk_var binutils pkgver) b_dir=$WORK/build-$KEY-binutils
  # an interrupted build tree is reused: configure only when it has no Makefile, make continues
  host_env
  b_musl=; $MUSL && b_musl="--disable-plugins --without-zstd"
  if $FORCE || [ ! -f "$b_dir/Makefile" ]; then
    rm -rf "$b_dir"; mkdir -p "$b_dir" && cd "$b_dir" || return 1
    "$SRC/binutils-$b_ver/configure" --prefix="$PREFIX" --target="$1" \
        --with-sysroot="$PREFIX/$1/sysroot" --disable-nls --disable-gprofng $b_musl \
        $(host_flags binutils "${1%%-*}") || return 1
  fi
  cd "$b_dir" && make -j"$JOBS" && make install DESTDIR="$STAGE" || return 1
  $KEEP || rm -rf "$b_dir"
}

step_gcc() {
  g_ver=$(pk_var gcc pkgver) g_dir=$WORK/build-$KEY-gcc g_arch=${1%%-*}
  PATH=$STAGE$PREFIX/bin:$PATH
  lt_cv_deplibs_check_method=pass_all
  export PATH lt_cv_deplibs_check_method
  host_env
  g_musl=
  if $MUSL; then
    CXX=$MCC/bin/x86_64-linux-musl-g++; export CXX
    g_musl="--with-gmp=$MDEPS --with-mpfr=$MDEPS --with-mpc=$MDEPS --with-isl=$MDEPS --disable-plugin --without-zstd"
  fi
  if $FORCE || [ ! -f "$g_dir/Makefile" ]; then
    rm -rf "$g_dir"; mkdir -p "$g_dir" && cd "$g_dir" || return 1
    "$SRC/gcc-$g_ver/configure" --prefix="$PREFIX" --libexecdir="$PREFIX/lib" --target="$1" \
        --disable-bootstrap --disable-nls --with-build-time-tools="$STAGE$PREFIX/$1/bin" \
        --with-sysroot="$PREFIX/$1/sysroot" --with-build-sysroot="$STAGE$PREFIX/$1/sysroot" \
        $g_musl $(host_flags gcc "$g_arch") $(arch_conf "$g_arch") || return 1
  fi
  # build_tooldir: the build compiler's -B must find the staged binutils, not an installed $PREFIX/$1/bin
  cd "$g_dir" && make -j"$JOBS" build_tooldir="$STAGE$PREFIX/$1" &&
    make -j1 install DESTDIR="$STAGE" || return 1
  $KEEP || rm -rf "$g_dir"
}

# step_deb: trim what would collide with the host's own binutils/gcc, strip, package
step_deb() {
  d_root=$STAGE d_arch=$(dpkg --print-architecture)
  d_gcc=$(pk_var gcc pkgver) d_pkg=$(echo "$1" | tr _ -) d_name=gcc-$d_pkg$SUFFIX
  rm -rf "$d_root/DEBIAN"; mkdir -p "$d_root/DEBIAN" || return 1
  rm -rf "$STAGE$PREFIX/share/info" "$STAGE$PREFIX/share/locale" "$STAGE$PREFIX/share/man/man7"
  rm -f "$STAGE$PREFIX"/lib/libcc1* "$STAGE$PREFIX"/lib/libiberty.a
  find "$STAGE$PREFIX" -type f | while read -r d_f; do
    [ "$(dd if="$d_f" bs=4 count=1 2>/dev/null | tr -d '\177')" = ELF ] && strip "$d_f" 2>/dev/null
  done
  d_deps=
  $MUSL || d_deps=$(find "$STAGE$PREFIX/lib/gcc/$1" -name cc1 -type f | head -n 1 | xargs ldd 2>/dev/null |
           awk '$3 ~ /^\// { print $3 }' | sed 's|^/lib|/usr/lib|' | xargs dpkg -S 2>/dev/null | cut -d: -f1 | sort -u | tr '\n' ',' | sed 's/,$//; s/,/, /g')
  {
    echo "Package: $d_name"
    echo "Version: $d_gcc-$PKGREL"
    echo "Architecture: $d_arch"
    echo "Maintainer: $(git config user.name 2>/dev/null) <$(git config user.email 2>/dev/null)>"
    echo "Installed-Size: $(du -sk "$d_root" | cut -f1)"
    $MUSL || echo "Depends: ${d_deps:-libc6}"
    $MUSL && echo "Conflicts: gcc-$d_pkg" && echo "Replaces: gcc-$d_pkg"
    echo "Provides: binutils-$d_pkg"
    echo "Section: devel"
    echo "Priority: optional"
    echo "Description: GCC $d_gcc and binutils cross toolchain for $1 (MSYS2)$($MUSL && echo ', static musl')"
    echo " C and C++ cross compiler plus binutils and a sysroot (MSYS2 runtime, w32api, libarchive"
    echo " stack) under $PREFIX/$1/sysroot. Built from the MSYS2-packages recipes."
  } >"$d_root/DEBIAN/control"
  dpkg-deb --root-owner-group -b "$d_root" "$WORK/${d_name}_$d_gcc-${PKGREL}_$d_arch.deb" >/dev/null || return 1
  echo "$WORK/${d_name}_$d_gcc-${PKGREL}_$d_arch.deb"
  rm -rf "$d_root/DEBIAN"
}

# --- main --------------------------------------------------------------------

fetch_repo
prepare binutils
prepare gcc
if $MUSL; then
  command -v "$MUSL_CC" >/dev/null || die "-m needs musl-gcc (package musl-tools)"
  gstep musl_cc
  gstep musl_deps
fi

for target in $TARGETS; do
  KEY=$target$SUFFIX
  STAGE=$WORK/stage-$KEY
  mkdir -p "$STAGE"
  step "$target" sysroot
  step "$target" binutils
  step "$target" gcc
  step "$target" deb
done
