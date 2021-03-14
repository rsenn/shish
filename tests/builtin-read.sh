# old_IFS="$IFS"
# IFS=" $old_IFS"
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
