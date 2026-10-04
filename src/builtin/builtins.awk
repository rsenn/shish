# Reads src/builtin/builtins.map for configure.ac and the Makefile.in files.
#
#   awk -v mode=names                        -f builtins.awk builtins.map   every switch name
#   awk -v mode=tier -v tiers=md             -f builtins.awk builtins.map   names of those tiers
#   awk -v mode=off -v enabled="cd cp ..." -v top=. -f builtins.awk builtins.map   sources no enabled switch names
#
# "off" prints paths relative to the top directory (src/builtin/../source/x.c -> src/source/x.c).
# A needs entry ending in "/" stands for every .c file of that directory (found with ls under top).

function add(name, p) {
  known[p] = 1
  if (index(" " enabled " ", " " name " ")) on[p] = 1
}

function norm(p) {
  p = "src/builtin/" p
  while (sub(/[^\/]+\/\.\.\//, "", p)) ;
  return p
}

/^#/ || NF == 0 { next }

{
  if (mode == "names")
    print $1
  else if (mode == "tier") {
    if (index(tiers, $3)) print $1
  } else {
    n = NF > 4 ? 0 : split($2 "," $4, f, ",")
    for (i = 1; i <= n; i++)
      if (f[i] != "-") {
        p = norm(f[i])
        if (p ~ /\/$/) {
          cmd = "ls " top "/" p "*.c 2>/dev/null"
          while ((cmd | getline q) > 0) {
            sub("^" top "/", "", q)
            add($1, q)
          }
          close(cmd)
        } else
          add($1, p)
      }
  }
}

END {
  if (mode == "off")
    for (p in known)
      if (!(p in on)) print p
}
