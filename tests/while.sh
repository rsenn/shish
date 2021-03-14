set -- A B C D E F G H
echo $#
IFS=","
while [ $# -gt 0 ]; do
  echo "$# : $1"
  shift
done
