
old_IFS="$IFS"
IFS=" $old_IFS"
while read -r DEV MNT TYPE OPTS DUMP PASS; do
    case "$DEV" in
        "" | "#"*) continue ;;
    esac
    echo "Device: $DEV Mount point: $MNT" 
    echo "Type: $TYPE Mount options: $OPTS" 
done </etc/fstab