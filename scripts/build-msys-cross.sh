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
# POSIX sh. External: git curl tar(GNU, xz) patch make awk sed sha256sum dpkg-deb
#                     and gcc, gmp/mpfr/mpc/isl/zlib/zstd development files on the host.

TOP=$(cd "$(dirname "$0")/.." && pwd)
PREFIX=/usr
WORK=$TOP/build/msys-cross
REPO=$TOP/third_party/msys2-packages
REPO_URL=${MSYS2_PACKAGES_URL:-https://github.com/msys2/MSYS2-packages}
TARGETS=
FORCE=false KEEP=false
JOBS=$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)
PKGREL=${PKGREL:-1}

usage() {
  echo "Usage: ${0##*/} [-p prefix] [-t target]... [-j jobs] [-w workdir] [-f] [-k]

  -p prefix   install prefix of the packaged toolchain (default /usr; /usr/local works too)
  -t target   i686-pc-msys or x86_64-pc-msys, or just i686 / x86_64 (default: both)
  -j jobs     parallel make jobs (default: number of CPUs)
  -w workdir  sources, build trees and .deb output (default build/msys-cross)
  -f          redo every step
  -k          keep the build trees (default: delete them after a successful install)

  environment: MSYS2_PACKAGES_URL PKGREL" 1>&2
  exit 1
}

while getopts p:t:j:w:fkh opt; do
  case $opt in
    p) PREFIX=$OPTARG ;;
    t) case $OPTARG in
         i686|i686-pc-msys) TARGETS="$TARGETS i686-pc-msys" ;;
         x86_64|x86_64-pc-msys) TARGETS="$TARGETS x86_64-pc-msys" ;;
         *) usage ;;
       esac ;;
    j) JOBS=$OPTARG ;;
    w) WORK=$OPTARG ;;
    f) FORCE=true ;;
    k) KEEP=true ;;
    *) usage ;;
  esac
done
[ -n "$TARGETS" ] || TARGETS="i686-pc-msys x86_64-pc-msys"
case $WORK in /*) ;; *) WORK=$PWD/$WORK ;; esac

DL=$WORK/dl SRC=$WORK/src STAMP=$WORK/stamp
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

# step TARGET NAME: run the function $2 unless its stamp exists
step() {
  [ -f "$STAMP/$1.$2" ] && ! $FORCE && return 0
  log "$1: $2"
  "step_$2" "$1" || die "$1: step $2 failed"
  touch "$STAMP/$1.$2"
}

step_sysroot() {
  BASE=$STAGE$PREFIX SYSROOT_NAME=sysroot "$TOP/scripts/get-msys-packages.sh" -t "msys/${1%%-*}"
}

step_binutils() {
  b_ver=$(pk_var binutils pkgver) b_dir=$WORK/build-$1-binutils
  # an interrupted build tree is reused: configure only when it has no Makefile, make continues
  if $FORCE || [ ! -f "$b_dir/Makefile" ]; then
    rm -rf "$b_dir"; mkdir -p "$b_dir" && cd "$b_dir" || return 1
    "$SRC/binutils-$b_ver/configure" --prefix="$PREFIX" --target="$1" \
        --with-sysroot="$PREFIX/$1/sysroot" --disable-nls --disable-gprofng \
        $(pk_flags binutils "${1%%-*}") || return 1
  fi
  cd "$b_dir" && make -j"$JOBS" && make install DESTDIR="$STAGE" || return 1
  $KEEP || rm -rf "$b_dir"
}

step_gcc() {
  g_ver=$(pk_var gcc pkgver) g_dir=$WORK/build-$1-gcc g_arch=${1%%-*}
  PATH=$STAGE$PREFIX/bin:$PATH
  lt_cv_deplibs_check_method=pass_all
  export PATH lt_cv_deplibs_check_method
  if $FORCE || [ ! -f "$g_dir/Makefile" ]; then
    rm -rf "$g_dir"; mkdir -p "$g_dir" && cd "$g_dir" || return 1
    "$SRC/gcc-$g_ver/configure" --prefix="$PREFIX" --libexecdir="$PREFIX/lib" --target="$1" \
        --disable-bootstrap --disable-nls --with-build-time-tools="$STAGE$PREFIX/$1/bin" \
        --with-sysroot="$PREFIX/$1/sysroot" --with-build-sysroot="$STAGE$PREFIX/$1/sysroot" \
        $(pk_flags gcc "$g_arch") $(arch_conf "$g_arch") || return 1
  fi
  # build_tooldir: the build compiler's -B must find the staged binutils, not an installed $PREFIX/$1/bin
  cd "$g_dir" && make -j"$JOBS" build_tooldir="$STAGE$PREFIX/$1" &&
    make -j1 install DESTDIR="$STAGE" || return 1
  $KEEP || rm -rf "$g_dir"
}

# step_deb: trim what would collide with the host's own binutils/gcc, strip, package
step_deb() {
  d_root=$STAGE d_arch=$(dpkg --print-architecture)
  d_gcc=$(pk_var gcc pkgver) d_pkg=$(echo "$1" | tr _ -) d_name=gcc-$d_pkg
  rm -rf "$d_root/DEBIAN"; mkdir -p "$d_root/DEBIAN" || return 1
  rm -rf "$STAGE$PREFIX/share/info" "$STAGE$PREFIX/share/locale" "$STAGE$PREFIX/share/man/man7"
  rm -f "$STAGE$PREFIX"/lib/libcc1* "$STAGE$PREFIX"/lib/libiberty.a
  find "$STAGE$PREFIX" -type f | while read -r d_f; do
    [ "$(dd if="$d_f" bs=4 count=1 2>/dev/null | tr -d '\177')" = ELF ] && strip "$d_f" 2>/dev/null
  done
  d_deps=$(find "$STAGE$PREFIX/lib/gcc/$1" -name cc1 -type f | head -n 1 | xargs ldd 2>/dev/null |
           awk '$3 ~ /^\// { print $3 }' | sed 's|^/lib|/usr/lib|' | xargs dpkg -S 2>/dev/null | cut -d: -f1 | sort -u | tr '\n' ',' | sed 's/,$//; s/,/, /g')
  cat >"$d_root/DEBIAN/control" <<EOF
Package: $d_name
Version: $d_gcc-$PKGREL
Architecture: $d_arch
Maintainer: $(git config user.name 2>/dev/null) <$(git config user.email 2>/dev/null)>
Installed-Size: $(du -sk "$d_root" | cut -f1)
Depends: ${d_deps:-libc6}
Provides: binutils-$d_pkg
Section: devel
Priority: optional
Description: GCC $d_gcc and binutils cross toolchain for $1 (MSYS2)
 C and C++ cross compiler plus binutils and a sysroot (MSYS2 runtime, w32api, libarchive
 stack) under $PREFIX/$1/sysroot. Built from the MSYS2-packages recipes.
EOF
  dpkg-deb --root-owner-group -b "$d_root" "$WORK/${d_name}_$d_gcc-${PKGREL}_$d_arch.deb" >/dev/null || return 1
  echo "$WORK/${d_name}_$d_gcc-${PKGREL}_$d_arch.deb"
  rm -rf "$d_root/DEBIAN"
}

# --- main --------------------------------------------------------------------

fetch_repo
prepare binutils
prepare gcc

for target in $TARGETS; do
  STAGE=$WORK/stage-$target
  mkdir -p "$STAGE"
  step "$target" sysroot
  step "$target" binutils
  step "$target" gcc
  step "$target" deb
done
