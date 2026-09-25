DIR=$(dirname "${0}")
. "$DIR/common.sh"

## Testing the grep builtin (src/builtin/extra/builtin_grep.c).
## Off by default: needs -DBUILTIN_GREP=ON (it's in EXTRA_BUILTINS).

case $(type grep 2>&1) in
  *builtin*) ;;
  *)
    echo "grep builtin not compiled in (-DBUILTIN_GREP=ON), skipping" 1>&2
    summary
    ;;
esac

TESTDIR=$(mktemp -d)
cd "$TESTDIR" || exit 1

printf 'apple\nbanana\ncherry\nbandana\n' > fruit.txt

## ---------------------------------------------------------------------
## basic matching, stdin vs file operand
## ---------------------------------------------------------------------

X=$(grep an fruit.txt)
assert_equal "banana
bandana" "$X" "a plain BRE pattern selects every matching line"

X=$(cat fruit.txt | grep an)
assert_equal "banana
bandana" "$X" "reading from stdin works the same as a file operand"

X=$(grep an - < fruit.txt)
assert_equal "banana
bandana" "$X" "an explicit '-' operand also means stdin"

X=$(grep xyz fruit.txt; echo "status=$?")
assert_equal "status=1" "$X" "no match: nothing printed, exit status 1"

X=$(grep an nosuchfile.txt 2>/dev/null; echo "status=$?")
assert_equal "status=1" "$X" "a file that can't be opened doesn't stop the exit-status-1 (no match) path from firing"

## ---------------------------------------------------------------------
## -v: invert match
## ---------------------------------------------------------------------

X=$(grep -v an fruit.txt)
assert_equal "apple
cherry" "$X" "-v selects only non-matching lines"

## ---------------------------------------------------------------------
## -n: line numbers
## ---------------------------------------------------------------------

X=$(grep -n an fruit.txt)
assert_equal "2:banana
4:bandana" "$X" "-n prefixes each matching line with its 1-based line number"

## ---------------------------------------------------------------------
## -c: count only
## ---------------------------------------------------------------------

X=$(grep -c an fruit.txt)
assert_equal "2" "$X" "-c prints the match count instead of the lines"

X=$(grep -c xyz fruit.txt)
assert_equal "0" "$X" "-c prints 0 when nothing matches"

## ---------------------------------------------------------------------
## -q: quiet
## ---------------------------------------------------------------------

X=$(grep -q an fruit.txt; echo "status=$?")
assert_equal "status=0" "$X" "-q prints nothing and exits 0 on a match"

X=$(grep -q xyz fruit.txt; echo "status=$?")
assert_equal "status=1" "$X" "-q prints nothing and exits 1 on no match"

## ---------------------------------------------------------------------
## -E: extended regular expressions
## ---------------------------------------------------------------------

X=$(printf 'cat\ncot\ncut\ncost\n' | grep 'c.t')
assert_equal "cat
cot
cut" "$X" "a BRE '.' matches any single character"

X=$(printf 'cat\ndog\n' | grep 'cat\|dog')
assert_equal "cat
dog" "$X" "\\| is a BRE alternation extension, so it works without -E too"

X=$(printf 'cat\ndog\nbird\n' | grep -E 'cat|dog')
assert_equal "cat
dog" "$X" "-E enables ERE alternation"

X=$(printf 'a\naa\naaa\n' | grep -E 'a+')
assert_equal "a
aa
aaa" "$X" "-E enables the ERE '+' repetition operator"

## ---------------------------------------------------------------------
## multiple file operands: filename prefix
## ---------------------------------------------------------------------

printf 'banana split\n' > more.txt

X=$(grep an fruit.txt more.txt)
assert_equal "fruit.txt:banana
fruit.txt:bandana
more.txt:banana split" "$X" "matching more than one file operand prefixes each line with its filename"

X=$(grep -c an fruit.txt more.txt)
assert_equal "fruit.txt:2
more.txt:1" "$X" "-c also prefixes the filename when given more than one file operand"

X=$(grep an fruit.txt)
assert_equal "banana
bandana" "$X" "a single file operand does not get a filename prefix"

## ---------------------------------------------------------------------
## anchors and character classes
## ---------------------------------------------------------------------

X=$(printf 'apple\nbanana\n' | grep '^a')
assert_equal "apple" "$X" "^ anchors to the start of the line"

X=$(printf 'apple\nbanana\n' | grep 'a$')
assert_equal "banana" "$X" "\$ anchors to the end of the line"

X=$(printf 'cat\nbat\nrat\nhat\n' | grep '[cb]at')
assert_equal "cat
bat" "$X" "a bracket expression matches any of its members"

summary
