DIR=$(dirname "${0}")
. "$DIR/common.sh"

## Testing find builtin (only meaningful when it is compiled in)

case $(command -V find 2>&1) in
  *"built-in"*) ;;
  *) summary ;;
esac

TESTDIR=$(mktemp -d)
cd "$TESTDIR" || exit 1
mkdir -p a/b c
printf 'echo sourced-a\n' >a/x.sh
printf 'echo sourced-b\n' >a/b/y.sh
printf 'hello' >a/h.txt
: >c/empty.txt
chmod 755 a/x.sh

nl='
'

## no path given: "." is the starting point, even when the first argument is -type
X=$(find -type f -maxdepth 2 | sort)
assert_equal "./a/h.txt${nl}./a/x.sh${nl}./c/empty.txt" "$X" "find -type f -maxdepth 2 without a path searches ."

X=$(find -type f -name '*.sh' | sort)
assert_equal "./a/b/y.sh${nl}./a/x.sh" "$X" "find -name without a path searches ."

X=$(find a -name y.sh)
assert_equal "a/b/y.sh" "$X" "find with a path prints paths below it"

## -exec ;  runs the command once per file, {} is the path (also inside a word)
X=$(find -type f -name '*.txt' -exec echo got {} \; | sort)
assert_equal "got ./a/h.txt${nl}got ./c/empty.txt" "$X" "find -exec runs the command for each file"

X=$(find a -name y.sh -exec echo X{}Y \;)
assert_equal "Xa/b/y.shY" "$X" "find -exec replaces {} inside a word"

## -exec can call builtins, which then run in the shell itself
X=$(find -type f -name '*.sh' -exec . {} \; | sort)
assert_equal "sourced-a${nl}sourced-b" "$X" "find -exec calls the source builtin"

find -name y.sh -exec eval 'FOUND=1' \;
assert_equal "1" "${FOUND-}" "find -exec runs a builtin in the current shell (variable stays set)"

## -exec +  runs the command once with all paths
X=$(find -type f -name '*.txt' -exec echo batch {} + | sort)
assert_equal "batch ./a/h.txt ./c/empty.txt" "$X" "find -exec ... {} + passes all paths to one command"

## -exec as a test
X=$(find -type f -exec test -s {} \; -print | sort)
assert_equal "./a/b/y.sh${nl}./a/h.txt${nl}./a/x.sh" "$X" "find -exec acts as a test on its exit status"

find -type f -exec 2>/dev/null
assert_equal "1" "$?" "find -exec without a terminator is an error"

## tests
X=$(find -type f -empty)
assert_equal "./c/empty.txt" "$X" "find -empty finds the empty file"

X=$(find -type f -size +4c | sort)
assert_equal "./a/b/y.sh${nl}./a/h.txt${nl}./a/x.sh" "$X" "find -size +4c finds files larger than 4 bytes"

X=$(find -type f -size 5c)
assert_equal "./a/h.txt" "$X" "find -size 5c matches exactly 5 bytes (and keeps matching for every file)"

X=$(find -type f -perm 755)
assert_equal "./a/x.sh" "$X" "find -perm 755 matches the mode exactly"

X=$(find -type f -perm -100)
assert_equal "./a/x.sh" "$X" "find -perm -100 matches files with the owner execute bit"

X=$(find -type f -executable)
assert_equal "./a/x.sh" "$X" "find -executable finds the executable file"

X=$(find -type f -mmin -5 | sort | head -1)
assert_equal "./a/b/y.sh" "$X" "find -mmin -5 finds files modified in the last five minutes"

X=$(find -type f -mtime +1)
assert_equal "" "$X" "find -mtime +1 finds nothing in a fresh tree"

## -regex matches the whole path (BRE by default), -iregex ignores case, -regextype selects EREs
X=$(find -regex '.*\.sh' | sort)
assert_equal "./a/b/y.sh${nl}./a/x.sh" "$X" "find -regex matches the whole path"

X=$(find -regex 'a')
assert_equal "" "$X" "find -regex is anchored: a pattern for a part of the path matches nothing"

X=$(find -regex '.*/[xy]\.sh' | sort)
assert_equal "./a/b/y.sh${nl}./a/x.sh" "$X" "find -regex understands bracket expressions"

X=$(find -iregex '.*\.TXT' | sort)
assert_equal "./a/h.txt${nl}./c/empty.txt" "$X" "find -iregex ignores case"

X=$(find -regextype posix-extended -regex '.*/(x|y)\.sh' | sort)
assert_equal "./a/b/y.sh${nl}./a/x.sh" "$X" "find -regextype posix-extended turns on EREs"

find -regex '\(' 2>/dev/null
assert_equal "1" "$?" "find -regex with an invalid expression fails"

## -prune, -depth, -not, -print0, -delete, -quit
X=$(find -type d -name c -prune -o -type f -print | sort)
assert_equal "./a/b/y.sh${nl}./a/h.txt${nl}./a/x.sh" "$X" "find -prune does not descend into the pruned directory"

X=$(find -depth -type d | head -1)
assert_equal "./a/b" "$X" "find -depth lists a directory's contents before the directory"

X=$(find -not -type d | wc -l)
assert_equal "4" "$(echo $X)" "find -not -type d counts the four files"

X=$(find -type f -name h.txt -print0 | od -c | head -1 | tr -s ' ')
assert_equal "0000000 . / a / h . t x t \\0" "$X" "find -print0 ends each path with a NUL"

X=$(find -type f -name '*.sh' -quit)
assert_equal "1" "$(echo $X | wc -w | tr -d ' ')" "find -quit stops after the first match"

find c -delete
test -e c
assert_equal "1" "$?" "find -delete removes the directory and its contents"

cd /
rm -rf "$TESTDIR"
summary
