


OUTPUT=$(echo "This is output from an 'echo' command")
echo "OUTPUT='$OUTPUT'" 1>&2

FORMATTED=`printf "0x%08X" $((0xdeadbeef))`
echo "FORMATTED='$FORMATTED'" 1>&2

X=1337
HERE=$(cat <<EOF
Here doc content $X
EOF
)
echo "HERE='$HERE'" 1>&2
