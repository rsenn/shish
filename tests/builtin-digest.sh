DIR=$(dirname "${0}")
. "$DIR/common.sh"

## Testing the digest builtin (md5sum, sha1sum, ... -- one implementation,
## picked by argv[0]). Off by default: needs -DENABLE_DIGEST=ON.

case $(type md5sum 2>&1) in
  *builtin*) ;;
  *)
    echo "digest builtin not compiled in (-DENABLE_DIGEST=ON), skipping" 1>&2
    summary
    ;;
esac

TESTDIR=$(mktemp -d)
cd "$TESTDIR" || exit 1

## check <command> <input> <expected hex digest>
check() {
  X=$(printf '%s' "$2" | "$1")
  assert_equal "$3  -" "$X" "$1 of '${2:-(empty)}' matches the reference digest"
}

check md5sum '' d41d8cd98f00b204e9800998ecf8427e
check md5sum abc 900150983cd24fb0d6963f7d28e17f72
check sha1sum '' da39a3ee5e6b4b0d3255bfef95601890afd80709
check sha1sum abc a9993e364706816aba3e25717850c26c9cd0d89d
check sha224sum '' d14a028c2a3a2bc9476102bb288234c415a2b01f828ea62ac5b3e42f
check sha224sum abc 23097d223405d8228642a477bda255b32aadbce4bda0b3f7e36c9da7
check sha256sum '' e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
check sha256sum abc ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad
check sha384sum '' 38b060a751ac96384cd9327eb1b1e36a21fdb71114be07434c0cc7bf63f6e1da274edebfe76f65fbd51ad2f14898b95b
check sha384sum abc cb00753f45a35e8bb5a03d699ac65007272c32ab0eded1631a8b605a43ff5bed8086072ba1e7cc2358baeca134c825a7
check sha512sum '' cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e
check sha512sum abc ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f
check sha512-224sum '' 6ed0dd02806fa89e25de060c19d3ac86cabb87d6a0ddd05c333b84f4
check sha512-224sum abc 4634270f707b6a54daae7530460842e20e37ed265ceee9a43e8924aa
check sha512-256sum '' c672b8d1ef56ed28ab87c3622c5114069bdd3ad7b8f9737498d0c01ecef0967a
check sha512-256sum abc 53048e2681941ef99b2e29b76b4c7dabe4c2d0c634fc6d46e0e2f13107e7af23

## padding boundaries: md5 pads to 64-byte blocks (55/56 bytes straddle
## the length field), sha512 to 128-byte blocks (111/112)
rep() { # rep <n>: n times 'a'
  R=
  i=0
  while [ "$i" -lt "$1" ]; do R="${R}a"; i=$((i + 1)); done
}

rep 55
check md5sum "$R" ef1772b6dff9a122358552954ad0df65
rep 56
check md5sum "$R" 3b0c8ac703f828b04c6c197006d17218
rep 111
check sha512sum "$R" fa9121c7b32b9e01733d034cfc78cbf67f926c7ed83e82200ef86818196921760b4beff48404df811b953828274461673c68d04e297b0eb7b2b4d60fc6b566a2
rep 112
check sha512sum "$R" c01d080efd492776a1c43bd23dd99d0a2e626d481e16782e75d54c2503b5dc32bd05f0f1ba33e568b88fd2d970929b719ecbb152f58f130a407c8830604b70ca

## file arguments: "<digest>  <name>" per file, "-" reads stdin
printf abc > f1
: > f2

X=$(md5sum f1 f2)
assert_equal "$(printf '900150983cd24fb0d6963f7d28e17f72  f1\nd41d8cd98f00b204e9800998ecf8427e  f2')" "$X" "one line per file, in argument order"

X=$(printf abc | md5sum f2 - f1)
assert_equal "$(printf 'd41d8cd98f00b204e9800998ecf8427e  f2\n900150983cd24fb0d6963f7d28e17f72  -\n900150983cd24fb0d6963f7d28e17f72  f1')" "$X" "'-' hashes stdin in its place among the files"

## errors: a missing file is reported and gives status 1, the rest still print
X=$(md5sum nosuch f1 2>/dev/null)
assert_equal "900150983cd24fb0d6963f7d28e17f72  f1" "$X" "files after a missing one are still hashed"

md5sum nosuch f1 >/dev/null 2>&1
assert_equal "1" "$?" "a missing file gives exit status 1"

md5sum -x >/dev/null 2>&1
assert_equal "1" "$?" "an unknown option gives exit status 1"

cd /
rm -rf "$TESTDIR"

summary
