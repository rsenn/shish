#!/bin/sh
# get-msys-packages.sh - download the libarchive stack from the MSYS2 mirror and
# unpack it into the cross sysroots in one go.
#
#   mirror dir      sysroot                            unpacked into
#   mingw/mingw32   /usr/i686-w64-mingw32/sysroot      sysroot/mingw  (the mingw32/ prefix is stripped)
#   mingw/mingw64   /usr/x86_64-w64-mingw32/sysroot    sysroot/mingw  (the mingw64/ prefix is stripped)
#   msys/i686       /usr/i686-pc-msys/sysroot          sysroot/usr
#   msys/x86_64     /usr/x86_64-pc-msys/sysroot        sysroot/usr
#
# Steps: list each mirror dir with lynx -> keep *.pkg.tar.* (no .sig, no -src, no cross)
#        -> one version per package (newest, or oldest with -o)
#        -> the libarchive family ($REGEX) is the seed set
#        -> download, unpack, and pull in the 'depend =' packages of each .PKGINFO
#        -> verify every dll with ntldd.
#
# Writing to /usr/*/sysroot needs root for some of them: sudo -E scripts/get-msys-packages.sh
# POSIX sh; external tools: lynx curl tar(GNU, for zstd/--strip-components) awk sed grep.

MIRROR=${MIRROR:-https://repo.msys2.org}
BASE=${BASE:-/usr}
CACHE=${CACHE:-${XDG_CACHE_HOME:-$HOME/.cache}/shish-msys}
NTLDD=${NTLDD:-/usr/local/bin/ntldd}
SYSROOT_NAME=${SYSROOT_NAME:-sysroot}   # BASE/<host>/<name>; the msys cross compiler uses 'sysroot'

# libarchive and what it links against; msys splits headers into -devel, so allow that suffix.
REGEX=${REGEX:-'(x86_64|i686|64|32).\b(lib|)(zlib|b2|lzma|xz|bzip2|bz2|lzo2|lz4|archive|zstd)-(devel-)?[0-9]'}

# mirror dir, sysroot dir below $BASE, unpack subdir, tar --strip-components, extra seed
# (msys2-runtime is implied by every msys package, no .PKGINFO names it; the devel/w32api
# packages are what a cross compiler needs in its sysroot)
TARGETS='
mingw/mingw32 i686-w64-mingw32 mingw 1 -
mingw/mingw64 x86_64-w64-mingw32 mingw 1 -
msys/i686 i686-pc-msys . 0 msys2-runtime,msys2-runtime-devel,msys2-w32api-headers,msys2-w32api-runtime,windows-default-manifest
msys/x86_64 x86_64-pc-msys . 0 msys2-runtime,msys2-runtime-devel,msys2-w32api-headers,msys2-w32api-runtime,windows-default-manifest
'

usage() {
  echo "Usage: ${0##*/} [-o] [-n] [-V] [-t dir]...

  -o      take the oldest version of each package (default: newest)
  -n      only print the seed package URLs (no dependencies, no download)
  -V      only verify what is installed (ntldd)
  -t dir  mirror dir to handle, e.g. -t msys/x86_64 (repeatable; default: all four)

  environment: MIRROR BASE CACHE REGEX NTLDD SYSROOT_NAME" 1>&2
  exit 1
}

WANT=newest DRY=false VERIFY=false ONLY=
while getopts onVt:h opt; do
  case $opt in
    o) WANT=oldest ;;
    n) DRY=true ;;
    V) VERIFY=true ;;
    t) ONLY="$ONLY $OPTARG " ;;
    *) usage ;;
  esac
done

# list DIR: all package archive URLs of a mirror directory (cached per day)
list() {
  list_f=$CACHE/$(echo "$1" | tr / _).$(date +%Y%m%d).txt
  [ -s "$list_f" ] || { lynx -dump -listonly -nonumbers "$MIRROR/$1/" 2>/dev/null |
    grep -E '\.pkg\.tar\.(zst|xz|gz)$' >"$list_f.tmp" && mv "$list_f.tmp" "$list_f"; }
  cat "$list_f"
}

# pick_all DIR: "name<TAB>url" per package of the mirror dir, one version each
# ('-src' and 'cross' dropped; newest, or oldest with -o). Versions compare field by field,
# numbers numerically (sort -V is not POSIX).
pick_all() {
  list "$1" | grep -v cross |
    awk -F/ -v want="$WANT" '
      function vercmp(a, b,    na, nb, pa, pb, i, x, y) {
        na = split(a, pa, /[^0-9A-Za-z]+/); nb = split(b, pb, /[^0-9A-Za-z]+/)
        for(i = 1; i <= (na > nb ? na : nb); i++) {
          x = pa[i]; y = pb[i]
          if(x == y) continue
          if(x == "") return -1
          if(y == "") return 1
          if(x ~ /^[0-9]+$/ && y ~ /^[0-9]+$/) { if(x + 0 != y + 0) return x + 0 < y + 0 ? -1 : 1; continue }
          return x < y ? -1 : 1
        }
        return 0
      }
      { f = $NF; sub(/\.pkg\.tar\.(zst|xz|gz)$/, "", f)
        n = split(f, a, "-"); if(n < 4) next
        name = a[1]; for(i = 2; i <= n - 3; i++) name = name "-" a[i]
        if(name ~ /-src$/) next
        ver = a[n-2] "-" a[n-1]
        if(!(name in best) || (want == "oldest" ? vercmp(ver, bver[name]) < 0 : vercmp(ver, bver[name]) > 0)) {
          best[name] = $0; bver[name] = ver
        } }
      END { for(name in best) printf "%s\t%s\n", name, best[name] }' | sort
}

# deps FILE: names in the 'depend =' lines of the archive's .PKGINFO, version constraints stripped
deps() {
  tar -xOf "$1" .PKGINFO 2>/dev/null | sed -n 's/^depend = *\([^<>=: ]*\).*/\1/p'
}

# verify SYSROOT: every dll must resolve its imports inside the sysroot (or be a Windows system dll)
verify() {
  v_root=$1 v_bad=0 v_dirs=
  for v_d in "$v_root"/mingw/bin "$v_root"/usr/bin; do [ -d "$v_d" ] && v_dirs="$v_dirs -D $v_d"; done
  [ -n "$v_dirs" ] || { echo "verify: $v_root: no bin dirs" 1>&2; return 1; }
  for v_f in "$v_root"/mingw/bin/*.dll "$v_root"/usr/bin/*.dll; do
    [ -f "$v_f" ] || continue
    v_out=$("$NTLDD" -R $v_dirs "$v_f" 2>&1 | grep -i 'not found' |
            grep -viE '(kernel32|ntdll|msvcrt|user32|advapi32|shell32|ws2_32|bcrypt|crypt32|ole32|oleaut32|api-ms-win|ucrtbase)\.dll')
    [ -n "$v_out" ] && { echo "UNRESOLVED in ${v_f#$v_root/}:"; echo "$v_out" | sed 's/^/  /'; v_bad=1; }
  done
  [ $v_bad = 0 ] && echo "verify: $v_root: all imports resolve"
  return $v_bad
}

mkdir -p "$CACHE" || exit 1
rc=0
while read -r dir host sub strip extra; do
  [ -n "$dir" ] || continue
  [ -n "$ONLY" ] && case "$ONLY" in *" $dir "*) ;; *) continue ;; esac
  root=$BASE/$host/$SYSROOT_NAME

  if $VERIFY; then verify "$root" || rc=1; continue; fi

  all=$(pick_all "$dir")
  urls=$(echo "$all" | cut -f2 | grep -iE "$REGEX")
  if [ "$extra" != - ]; then
    for e in $(echo "$extra" | tr , ' '); do
      urls="$urls
$(echo "$all" | awk -F'\t' -v n="$e" '$1 == n { print $2 }')"
    done
  fi
  [ -n "$urls" ] || { echo "$dir: no packages selected" 1>&2; rc=1; continue; }
  if $DRY; then echo "$urls"; continue; fi

  mkdir -p "$root/$sub" 2>/dev/null && [ -w "$root" ] ||
    { echo "$dir: cannot write $root (dangling symlink or not root?); run with sudo -E" 1>&2; rc=1; continue; }
  receipt=$root/.packages
  touch "$receipt"

  # queue: the seeds first, then the dependencies they name, until nothing new turns up
  q=$CACHE/queue.$$ seen=$CACHE/seen.$$
  echo "$urls" >"$q"; : >"$seen"
  i=1
  while url=$(sed -n "${i}p" "$q") && [ -n "$url" ]; do
    i=$((i + 1))
    file=${url##*/}
    grep -qxF "$file" "$seen" && continue
    echo "$file" >>"$seen"
    [ -s "$CACHE/$file" ] || { curl -sSfL -C - -o "$CACHE/$file.part" "$url" && mv "$CACHE/$file.part" "$CACHE/$file"; } || { echo "FAILED $url" 1>&2; rc=1; continue; }
    for d in $(deps "$CACHE/$file"); do
      u=$(echo "$all" | awk -F'\t' -v n="$d" '$1 == n { print $2 }')
      [ -n "$u" ] && echo "$u" >>"$q"
    done
    grep -qxF "$file" "$receipt" && { echo "have $file"; continue; }
    echo "unpack $file -> $root/$sub"
    tar -xf "$CACHE/$file" -C "$root/$sub" --strip-components="$strip" \
        --exclude='.PKGINFO' --exclude='.BUILDINFO' --exclude='.MTREE' --exclude='.INSTALL' --exclude='.CHANGELOG' &&
      echo "$file" >>"$receipt" || { echo "FAILED unpack $file" 1>&2; rc=1; }
  done
  rm -f "$q" "$seen"
  verify "$root" || rc=1
done <<EOF
$TARGETS
EOF
exit $rc
