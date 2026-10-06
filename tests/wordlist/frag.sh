# Heap fragmentation under a few hundred live variables. Not run by ctest.
#   gcc -shared -fPIC -O2 -w tests/wordlist/mallinfo.c -o mallinfo.so
#   N=200000 LD_PRELOAD=$PWD/mallinfo.so shish tests/wordlist/frag.sh     # prints MALLINFO ... on stderr at exit
# Run it with two binaries and compare heap, in_use, free, free_chunks and fastbin_chunks.
# a few hundred live variables of mixed sizes, then a long mixed workload that keeps changing some of them
i=0
while [ $i -lt 400 ]; do
  pad=$i
  j=0
  while [ $j -lt $((i % 23)) ]; do pad="$pad-xxxxxxxx"; j=$((j+1)); done
  eval "v$i='$pad'"
  i=$((i+1))
done
n=0
while [ $n -lt ${N:-60000} ]; do
  : a b c d e f g h $v1 "$v2 x" $((n+1))
  x=$v7$n
  y="$v9 $v10 $n"
  set -- $v3 $v4 a b c
  for w in $v5 q r; do :; done
  [ "$x" = zz ] && echo never
  case $v11 in 11*) : ;; esac
  if [ $((n % 20)) -eq 0 ]; then
    k=$(echo sub $n)
    eval "v$((n % 400))='changed-$n-$k-$((n*n))-padding-$((n % 7))'"
  fi
  n=$((n+1))
done
echo done $n
