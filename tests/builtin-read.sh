
old_IFS="$IFS"
IFS=" $old_IFS"
while read -r DEV MNT TYPE OPTS REST; do
    case "$DEV" in
        "" | "#"*) continue ;;
    esac
    dump -v
    echo "Device: $DEV Mount point: $MNT Type: $TYPE Mount options: $OPTS" 
done </etc/fstab