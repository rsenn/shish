


OUTPUT=$(echo "This is output from an 'echo' command")
echo "OUTPUT='$OUTPUT'" 1>&2
FORMATTED=`printf "0x%08X" $((0xdeadbeef))`

echo "FORMATTED='$FORMATTED'" 1>&2
