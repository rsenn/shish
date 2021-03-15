ask_passwd() {
    read -r -p"Password: " -s PASSWORD
    echo
    echo "Password is '$PASSWORD'"
}

read_partial() {
    echo "ABCDEFGHIJKLMNOPQRSTUVWXYZ" >tmp.txt
    read -r -n 10 ABCD <tmp.txt
    echo "ABCD='$ABCD'" 1>&2
}

while read -r DEV MNT TYPE OPTS REST; do
  case "$DEV" in
  "" | "#"*) continue ;;
  esac
  #dump -v DEV MNT TYPE OPTS REST
  echo "Device: $DEV"
  echo "Type: $TYPE"
  echo "Mount point: $MNT"
  echo "Mount options: $OPTS"
  echo
  #printf "Device: %-42s Mount point: %-20s Type: %-10s Mount options: %-20s\n" "$DEV" "$MNT" "$TYPE" "$OPTS"
done </etc/fstab

(
  echo '\\BL\nAH'
  echo '\\Line\n2'
) >tmp.txt

(
  read -r -p"Test: " TEST
  echo "TEST=$TEST" 1>&2
  read -r TEST2

  echo "TEST2=$TEST2" 1>&2
) <tmp.txt

read_partial