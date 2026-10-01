DIR=$(dirname "${0}")
. "$DIR/common.sh"

## compress and uncompress builtins (src/builtin/filter/builtin_compress.c,
## builtin_uncompress.c); skipped when they are not compiled in
case $(type gzip) in
*builtin*) ;;
*) echo "gzip is not a builtin, skipping" 1>&2; summary ;;
esac

T=$(mktemp -d)
cd "$T" || exit 1

## is a program of that name in PATH (the builtin of the same name does not count)
have_prog() {
  (IFS=:
   for d in $PATH; do
     [ -x "$d/$1" ] && exit 0
   done
   exit 1)
}

printf 'hello world\nsecond line\n' >orig
i=0
: >big
while [ $i -lt 3000 ]; do echo "line $i xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx" >>big; i=$((i + 1)); done

## every compressor: file -> file.suffix -> back, through stdin and stdout, and -k, -c
for a in gzip:gz bzip2:bz2 lbzip2:bz2 xz:xz zstd:zst lz:lz lz4:lz4 lzma:lzma lzop:lzo; do
  n=${a%%:*}
  x=${a##*:}

  ## lzop writes with liblzo2 but reads through the lzop program
  case $n in
  lzop) have_prog $n || { echo "$n: no $n program in PATH, skipping" 1>&2; continue; } ;;
  esac

  cp orig f
  "$n" f 2>/dev/null
  assert_equal "f.$x 0" "$(ls f.* | tr '\n' ' ' | sed 's/ $//') $(ls f 2>/dev/null | wc -l | tr -d ' ')" "$n turns f into f.$x and removes f"
  [ -e "f.$x" ] || continue

  "$n" -d "f.$x"
  assert_equal "hello world" "$(head -n 1 f)" "$n -d restores the file"
  assert_equal "0" "$(ls f.* 2>/dev/null | wc -l | tr -d ' ')" "$n -d removes the compressed file"

  "$n" -k f
  assert_equal "1" "$(ls f f.$x | wc -l | tr -d ' ' | sed 's/^2$/1/')" "$n -k keeps the original"
  rm -f f
  assert_equal "hello world" "$("$n" -dc "f.$x" | head -n 1)" "$n -dc writes to stdout"
  assert_equal "0" "$(ls f 2>/dev/null | wc -l | tr -d ' ')" "$n -dc leaves no file behind"
  rm -f f.*

  assert_equal "$(wc -c <big | tr -d ' ')" "$("$n" <big | "$n" -d | wc -c | tr -d ' ')" "$n round-trips a pipe"
  assert_equal "hello world" "$("$n" -c orig | uncomp=1 zcat | head -n 1)" "zcat detects $n data from a pipe"
  assert_equal "hello world" "$("$n" -c orig >f.$x; zcat f.$x | head -n 1)" "zcat detects $n data from a file"
  rm -f f f.*
done

## the *cat names read any of them; the un* names work on files like <algo> -d
for p in lz4:lz4cat lzma:lzcat xz:lzcat xz:xzcat zstd:zstdcat bzip2:bzcat lbzip2:lbzcat gzip:zcat; do
  n=${p%%:*}
  c=${p##*:}
  assert_equal "hello world" "$("$n" -c orig | "$c" | head -n 1)" "$c reads $n data"
done

for p in gzip:gunzip:gz xz:unxz:xz zstd:unzstd:zst; do
  IFS=: read n u x <<EOT
$p
EOT
  cp orig f
  "$n" f
  "$u" "f.$x"
  assert_equal "hello world 0" "$(head -n 1 f) $(ls f.* 2>/dev/null | wc -l | tr -d ' ')" "$u turns f.$x back into f"
  "$n" f
  "$u" -k "f.$x"
  assert_equal "2" "$(ls f f.$x 2>/dev/null | wc -l | tr -d ' ')" "$u -k keeps the compressed file"
  "$u" "f.$x" 2>/dev/null
  assert_equal "1" "$?" "$u refuses to overwrite f without -f"
  "$u" -f "f.$x"
  assert_equal "0" "$?" "$u -f overwrites"
  rm -f f
  assert_equal "hello world" "$("$n" -c orig | "$u" -c | head -n 1)" "$u -c filters a pipe"
  rm -f f f.*
done

"$n" -c orig >plain
cp orig plain.gz
gunzip plain.gz 2>/dev/null
assert_equal "1" "$?" "gunzip refuses data that is not compressed"
gunzip nosuch.gz 2>/dev/null
assert_equal "1" "$?" "gunzip fails on a missing file"
gunzip orig 2>/dev/null
assert_equal "1" "$?" "gunzip fails on a name without the suffix"
assert_equal "hello world" "$(head -n 1 orig)" "a failed gunzip leaves the file alone"

cd / && rm -rf "$T"
summary
