func1() {
  echo "called func1"
  return 0
}
echo "define func2"
func2() (
  echo "called func2"
  exit 0
)
echo "define func3"

func3 () {
  echo "called func3"
}
