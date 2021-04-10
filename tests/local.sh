test_fn1() {
  local LXXXXXX LYYYYYY='b'
  
  dump -v LXXXXXX LYYYYYY

  LXXXXXX="a"
  GWWWWWW='c'

  dump -v LXXXXXX LYYYYYY GWWWWWW
}

test_fn1

dump -v LXXXXXX LYYYYYY GWWWWWW

echo LXXXXXX=${LXXXXXX-unset}
echo LYYYYYY=${LV2-unset}
echo GWWWWWW=${GWWWWWW-unset}

