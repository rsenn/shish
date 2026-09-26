DIR=$(dirname "${0}")
. "$DIR/common.sh"

## Testing xargs builtin

TESTDIR=$(mktemp -d)
cd "$TESTDIR" || exit 1

nl='
'

## default utility is echo, all input lines go into one invocation
X=$(printf 'a\nb\nc\n' | xargs)
assert_equal "a b c" "$X" "xargs with no utility runs echo with all input lines"

## fixed arguments come before the input arguments
X=$(printf 'a\nb\n' | xargs echo pre)
assert_equal "pre a b" "$X" "xargs puts utility arguments before input arguments"

## no input and no utility: echo runs once with no arguments
X=$(xargs </dev/null)
assert_equal "" "$X" "xargs with no utility and empty input runs echo once (blank line)"
X=$(xargs </dev/null | wc -l)
assert_equal "1" "$(echo $X)" "xargs with no utility and empty input prints exactly one newline"

## -r: empty input runs nothing
X=$(xargs -r echo none </dev/null)
assert_equal "" "$X" "xargs -r does not run the utility on empty input"

X=$(printf 'a\n' | xargs -r echo run)
assert_equal "run a" "$X" "xargs -r still runs the utility when there is input"

## -n: at most N arguments per invocation
X=$(printf 'a\nb\nc\n' | xargs -n1 echo p)
assert_equal "p a${nl}p b${nl}p c" "$X" "xargs -n1 runs one invocation per argument"

X=$(printf 'a\nb\nc\n' | xargs -n2 echo p)
assert_equal "p a b${nl}p c" "$X" "xargs -n2 groups two arguments and runs the remainder last"

X=$(printf 'a\nb\n' | xargs -n5 echo p)
assert_equal "p a b" "$X" "xargs -n larger than the input uses a single invocation"

X=$(printf 'a\nb\nc\nd\n' | xargs -n2 echo p | wc -l)
assert_equal "2" "$(echo $X)" "xargs -n2 on four arguments makes exactly two invocations"

## -0: NUL-separated input keeps newlines and blanks inside arguments
X=$(printf 'a b\0c\nd\0' | xargs -0 -n1 echo p)
assert_equal "p a b${nl}p c${nl}d" "$X" "xargs -0 splits on NUL only"

## -d: custom separator
X=$(printf 'a,b,c' | xargs -d , -n1 echo p)
assert_equal "p a${nl}p b${nl}p c" "$X" "xargs -d , splits on commas"

X=$(printf 'a,b,c' | xargs -d , echo)
assert_equal "a b c" "$X" "xargs -d , batches all comma-separated items"

## -a: read from a file instead of stdin
printf 'f1\nf2\n' > in.txt
X=$(xargs -a in.txt echo got </dev/null)
assert_equal "got f1 f2" "$X" "xargs -a reads arguments from the file"

X=$(xargs -a nonexistent.txt echo got </dev/null 2>/dev/null)
assert_equal "" "$X" "xargs -a with a missing file runs nothing"
xargs -a nonexistent.txt echo got </dev/null 2>/dev/null
assert_equal "127" "$?" "xargs -a with a missing file exits 127"

## CRLF line endings are stripped
X=$(printf 'a\r\nb\r\n' | xargs -n1 echo p)
assert_equal "p a${nl}p b" "$X" "xargs strips carriage returns from newline-separated input"

## -I: insert mode
X=$(printf 'a\nb\n' | xargs -I X echo "<X>")
assert_equal "<a>${nl}<b>" "$X" "xargs -I runs the utility once per line"

X=$(printf 'a b\n' | xargs -I X echo "<X>")
assert_equal "<a b>" "$X" "xargs -I keeps blanks inside a line as one argument"

X=$(printf 'a\n' | xargs -I X echo X X X)
assert_equal "a a a" "$X" "xargs -I replaces every argument containing the replstr"

X=$(printf 'a\n' | xargs -I X echo "X-X" "pre-X")
assert_equal "a-a pre-a" "$X" "xargs -I replaces multiple occurrences inside one argument"

X=$(printf '  lead\n' | xargs -I X echo "<X>")
assert_equal "<lead>" "$X" "xargs -I skips leading blanks of each line"

X=$(printf 'a\n\n\nb\n' | xargs -I X echo "<X>")
assert_equal "<a>${nl}<b>" "$X" "xargs -I ignores empty lines"

X=$(printf 'a\nb\n' | xargs -I {} echo {} {})
assert_equal "a a${nl}b b" "$X" "xargs -I {} works with the conventional {} string"

X=$(printf 'a\n' | xargs -I XX echo XX_XX)
assert_equal "a_a" "$X" "xargs -I supports multi-character replstr"

X=$(printf 'a\nb\n' | xargs -I X echo fixed)
assert_equal "fixed${nl}fixed" "$X" "xargs -I still runs once per line when replstr is unused"

X=$(printf 'a\n' | xargs -I X)
assert_equal "" "$X" "xargs -I with no utility echoes an empty line"

X=$(xargs -I X echo "<X>" </dev/null)
assert_equal "" "$X" "xargs -I on empty input runs nothing"

X=$(printf 'a\nb\n' | xargs -I X -r echo "<X>" 2>/dev/null)
assert_equal "<a>${nl}<b>" "$X" "xargs -I X -r: options after -I's argument are still parsed"

X=$(printf 'a\nb\nc\n' | xargs -I X -n2 echo X 2>/dev/null)
assert_equal "a${nl}b${nl}c" "$X" "xargs -I X -n2: -n2 is parsed as an option, replstr unused"

## -L / -l: lines per invocation
X=$(printf 'a\nb\nc\nd\ne\n' | xargs -L2 echo p)
assert_equal "p a b${nl}p c d${nl}p e" "$X" "xargs -L2 groups two lines per invocation"

X=$(printf 'a\nb\nc\n' | xargs -L1 echo p)
assert_equal "p a${nl}p b${nl}p c" "$X" "xargs -L1 runs once per line"

X=$(printf 'a\nb\nc\n' | xargs -L10 echo p)
assert_equal "p a b c" "$X" "xargs -L larger than the input uses a single invocation"

X=$(printf 'a\nb\n\nc\nd\ne\n' | xargs -L2 echo p)
assert_equal "p a b${nl}p c d${nl}p e" "$X" "xargs -L skips blank lines without counting them"

X=$(printf 'a\n  \n\t\nb\n' | xargs -L1 echo p)
assert_equal "p a${nl}p b" "$X" "xargs -L skips whitespace-only lines"

X=$(printf 'a \nb\nc\nd\n' | xargs -L2 echo p)
assert_equal "p a  b c${nl}p d" "$X" "xargs -L: trailing blank continues the line onto the next"

X=$(printf 'a\t\nb\nc\n' | xargs -L1 echo p)
assert_equal "p a	 b${nl}p c" "$X" "xargs -L: trailing tab continues the line onto the next"

X=$(printf 'a\\ \nb\nc\n' | xargs -L1 echo p)
assert_equal "p a\\ ${nl}p b${nl}p c" "$X" "xargs -L: an escaped trailing blank does not continue"

X=$(printf 'a\nb\nc\nd\n' | xargs -l 2 echo p)
assert_equal "p a b${nl}p c d" "$X" "xargs -l 2 behaves like -L 2"

X=$(printf 'a\nb\nc\n' | xargs -l 1 echo p)
assert_equal "p a${nl}p b${nl}p c" "$X" "xargs -l 1 runs once per line"

X=$(printf 'a\nb\nc\nd\n' | xargs -L3 -n2 echo p)
assert_equal "p a b${nl}p c d" "$X" "xargs -L3 -n2: -n limit is hit first"

X=$(printf 'a\nb\nc\nd\n' | xargs -L2 -n5 echo p)
assert_equal "p a b${nl}p c d" "$X" "xargs -L2 -n5: -L limit is hit first"

X=$(printf 'a\nb\n' | xargs -r -L1 echo p)
assert_equal "p a${nl}p b" "$X" "xargs -r -L1 combines"

X=$(printf '\n\n' | xargs -L1 | wc -l)
assert_equal "1" "$(echo $X)" "xargs -L1 on blank-only input runs echo once with no arguments"

X=$(printf '\n\n' | xargs -r -L1 echo p)
assert_equal "" "$X" "xargs -r -L1 on blank-only input runs nothing"

## -t: trace to stderr
printf 'a\nb\n' | xargs -t echo p 2>err.txt >/dev/null
X=$(cat err.txt)
assert_equal "echo p a b" "$X" "xargs -t writes the command line to stderr"

X=$(printf 'a\nb\n' | xargs -t echo p 2>/dev/null)
assert_equal "p a b" "$X" "xargs -t leaves stdout untouched"

printf 'a\nb\n' | xargs -t -n1 echo p 2>err.txt >/dev/null
X=$(cat err.txt)
assert_equal "echo p a${nl}echo p b" "$X" "xargs -t traces every invocation"

printf 'a\nb\n' | xargs -t -L1 echo p 2>err.txt >/dev/null
X=$(cat err.txt)
assert_equal "echo p a${nl}echo p b" "$X" "xargs -t traces every -L invocation"

printf 'a\nb\n' | xargs -t -I X echo "<X>" 2>err.txt >/dev/null
X=$(cat err.txt)
assert_equal "echo <a>${nl}echo <b>" "$X" "xargs -t traces after -I substitution"

printf 'a\n' | xargs echo p 2>err.txt >/dev/null
X=$(cat err.txt)
assert_equal "" "$X" "xargs without -t writes nothing to stderr"

xargs -t </dev/null 2>err.txt >/dev/null
X=$(cat err.txt)
assert_equal "echo" "$X" "xargs -t traces the implicit echo on empty input"

printf 'a\n' | xargs -t 2>err.txt >/dev/null
X=$(cat err.txt)
assert_equal "echo a" "$X" "xargs -t traces the implicit echo"

## exit status
printf 'a\n' | xargs true
assert_equal "0" "$?" "xargs exits 0 when the utility succeeds"

printf 'a\n' | xargs false
assert_equal "1" "$?" "xargs propagates a failing utility exit status"

printf 'a\n' | xargs /nonexistent/cmd 2>/dev/null
assert_equal "127" "$?" "xargs exits 127 when the utility is not found"

printf 'a\nb\n' | xargs -n1 sh -c 'exit 3' x 2>/dev/null
assert_equal "3" "$?" "xargs reports a nonzero status from any invocation"

## invalid option
printf 'a\n' | xargs -Z echo 2>/dev/null
assert_equal "1" "$?" "xargs with an invalid option exits 1"

## large input: many invocations and growing argument lists
i=0
while [ $i -lt 300 ]; do
  echo $i
  i=$((i + 1))
done > many.txt

X=$(xargs -a many.txt echo | wc -w)
assert_equal "300" "$(echo $X)" "xargs passes 300 arguments in a single invocation"

X=$(xargs -a many.txt -n50 echo | wc -l)
assert_equal "6" "$(echo $X)" "xargs -n50 splits 300 arguments into 6 invocations"

X=$(xargs -a many.txt -L100 echo | wc -l)
assert_equal "3" "$(echo $X)" "xargs -L100 splits 300 lines into 3 invocations"

X=$(xargs -a many.txt -I X echo X | wc -l)
assert_equal "300" "$(echo $X)" "xargs -I makes 300 invocations for 300 lines"

cd / && rm -rf "$TESTDIR"

summary
