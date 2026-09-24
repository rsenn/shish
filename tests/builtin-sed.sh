DIR=$(dirname "${0}")
. "$DIR/common.sh"

## Testing the sed builtin (text/sed + src/builtin/extra/builtin_sed.c).
## Off by default: needs -DBUILTIN_SED=ON (it's in EXTRA_BUILTINS).

case $(type sed 2>&1) in
  *builtin*) ;;
  *)
    echo "sed builtin not compiled in (-DBUILTIN_SED=ON), skipping" 1>&2
    summary
    ;;
esac

TESTDIR=$(mktemp -d)
cd "$TESTDIR" || exit 1

## ---------------------------------------------------------------------
## basic substitution
## ---------------------------------------------------------------------

X=$(printf 'hello world\n' | sed 's/world/there/')
assert_equal "hello there" "$X" "s/// replaces the first match"

X=$(printf 'aaa\n' | sed 's/a/b/')
assert_equal "baa" "$X" "s/// without g only replaces the first match"

X=$(printf 'aaa\n' | sed 's/a/b/g')
assert_equal "bbb" "$X" "s///g replaces every match"

X=$(printf 'aaaa\n' | sed 's/a/X/2')
assert_equal "aXaa" "$X" "s///N replaces only the Nth match"

X=$(printf 'aaaa\n' | sed 's/a/X/2g')
assert_equal "aXXX" "$X" "s///Ng replaces the Nth match and every one after it"

X=$(printf 'HELLO\n' | sed 's/hello/hi/I')
assert_equal "hi" "$X" "s///I is case-insensitive"

X=$(printf 'abc\n' | sed 's/[abc]/X/g')
assert_equal "XXX" "$X" "s/// pattern uses BRE bracket expressions"

X=$(printf 'a.b.c\n' | sed 's/\./-/g')
assert_equal "a-b-c" "$X" "s/// pattern backslash-escapes a BRE metacharacter"

X=$(printf 'one two\n' | sed -E 's/(one) (two)/\2 \1/')
assert_equal "two one" "$X" "s/// with -E (ERE) supports unescaped groups and \\N backreferences"

X=$(printf 'abc\n' | sed 's/\(a\)\(b\)\(c\)/\3\2\1/')
assert_equal "cba" "$X" "s/// in BRE supports \\( \\) groups and \\N backreferences in the replacement"

X=$(printf 'x\n' | sed 's/x/[&]/')
assert_equal "[x]" "$X" "s/// replacement '&' expands to the whole match"

X=$(printf 'a&b\n' | sed 's/&/AND/')
assert_equal "aANDb" "$X" "s/// pattern '&' is a literal ampersand, only special in the replacement"

X=$(printf 'a&b\n' | sed 's/a/[\&]/')
assert_equal "[&]&b" "$X" "s/// replacement '\\&' is a literal ampersand"

## empty-match rule: an empty match is followed by copying one byte
## before searching again, so it can't loop forever and can't replace
## the same spot twice.
X=$(printf 'abc\n' | sed 's/x*/-/g')
assert_equal "-a-b-c-" "$X" "s///g with a pattern that can match empty follows the empty-match advance rule"

## ---------------------------------------------------------------------
## empty regex // reuses the last RE used at run time
## ---------------------------------------------------------------------

X=$(printf 'foo\n' | sed '/foo/s//bar/')
assert_equal "bar" "$X" "an empty // reuses the address's regex in a following s///"

X=$(printf 'foofoo\n' | sed 's/foo/bar/;s//baz/')
assert_equal "barbaz" "$X" "an empty // in s/// reuses the previous s///'s regex"

## ---------------------------------------------------------------------
## addresses: line number, \$, regex, ranges, negation
## ---------------------------------------------------------------------

X=$(printf '1\n2\n3\n' | sed -n '2p')
assert_equal "2" "$X" "a numeric address selects that line"

X=$(printf '1\n2\n3\n' | sed -n '$p')
assert_equal "3" "$X" "\$ selects the last line"

X=$(printf 'a\nb\na\n' | sed -n '/a/p')
assert_equal "$(printf 'a\na\n')" "$X" "a /regex/ address selects every matching line"

X=$(printf '1\n2\n3\n4\n5\n' | sed -n '2,4p')
assert_equal "$(printf '2\n3\n4\n')" "$X" "addr1,addr2 selects an inclusive numeric range"

X=$(printf 'a\nb\nc\nd\ne\n' | sed -n '/b/,/d/p')
assert_equal "$(printf 'b\nc\nd\n')" "$X" "addr1,addr2 selects a regex-delimited range"

X=$(printf '1\n2\n3\n4\n5\n' | sed -n '2,$p')
assert_equal "$(printf '2\n3\n4\n5\n')" "$X" "addr1,\$ selects through the last line"

X=$(printf '1\n2\n3\n' | sed -n '2!p')
assert_equal "$(printf '1\n3\n')" "$X" "! negates an address"

X=$(printf 'a\nb\nc\nd\n' | sed -n '3,1p')
assert_equal "c" "$X" "addr1,addr2 with addr2 <= addr1's line number selects exactly that one line"

## ---------------------------------------------------------------------
## d D p P q = n N
## ---------------------------------------------------------------------

X=$(printf 'a\nb\nc\n' | sed '2d')
assert_equal "$(printf 'a\nc\n')" "$X" "d deletes the pattern space and starts a new cycle"

X=$(printf 'a\nb\n' | sed -n 'p;p')
assert_equal "$(printf 'a\na\nb\nb\n')" "$X" "p prints the pattern space (auto-print off via -n, so only the explicit p's show)"

X=$(printf 'a\nb\nc\n' | sed '2q')
assert_equal "$(printf 'a\nb\n')" "$X" "q prints (unless -n) then stops reading further input"

( printf 'a\nb\nc\n' | sed '2q5' >/dev/null )
assert_equal "5" "$?" "q's operand becomes the exit status"

X=$(printf '1\n2\n3\n' | sed '=')
assert_equal "$(printf '1\n1\n2\n2\n3\n3\n')" "$X" "= writes the current line number before the (auto-printed) line"

X=$(printf 'a\nb\nc\nd\n' | sed -n 'n;p')
assert_equal "$(printf 'b\nd\n')" "$X" "n loads the next line without printing (under -n) and continues the script"

X=$(printf 'a\nb\nc\nd\n' | sed 'N;s/\n/-/')
assert_equal "$(printf 'a-b\nc-d\n')" "$X" "N appends the next line, joined by an embedded newline matchable as \\\\n"

## N at true end of input with an odd number of lines: POSIX says N
## quits WITHOUT printing the (incomplete) pattern space, unlike n
X=$(printf 'a\nb\nc\n' | sed 'N')
assert_equal "$(printf 'a\nb\n')" "$X" "N at end of input with no next line discards the pattern space (POSIX, unlike GNU sed's default)"

X=$(printf 'a\nb\nc\n' | sed 'n')
assert_equal "$(printf 'a\nb\nc\n')" "$X" "n at end of input still prints the pattern space it already had, then stops"

## P/D: print/delete only through the first embedded newline. The
## classic idiom guards N with '$!' (don't call N on the last line):
## bare N on the last line hits N's POSIX "no next line: discard,
## don't print" rule and would silently drop the final line.
X=$(printf 'a\nb\nc\nd\n' | sed -n '$!N;P;D' 2>&1)
assert_equal "$(printf 'a\nb\nc\nd\n')" "$X" "\$!N;P;D processes a file one line at a time via the pattern-space-restart idiom"

## classic squeeze-consecutive-blank-lines idiom (cat -s)
X=$(printf 'a\n\n\n\nb\n\nc\n' | sed '/^$/{N;/^\n$/D}')
assert_equal "$(printf 'a\n\nb\n\nc\n')" "$X" "the N/D blank-line-squeeze idiom collapses runs of blank lines to one"

## ---------------------------------------------------------------------
## hold space: g G h H x
## ---------------------------------------------------------------------

X=$(printf 'a\nb\n' | sed -n '1h;2{G;p}')
assert_equal "$(printf 'b\na\n')" "$X" "h copies pattern to hold, G appends hold to pattern"

X=$(printf 'a\nb\n' | sed -n '1{h;d};2{x;p}')
assert_equal "a" "$X" "h then x round-trips the pattern space through the hold space"

## classic tac (reverse) idiom via the hold space
X=$(printf '1\n2\n3\n' | sed -n '1!G;h;$p')
assert_equal "$(printf '3\n2\n1\n')" "$X" "the 1!G;h;\$p idiom reverses the input like tac"

X=$(printf 'a\nb\n' | sed -n '1H;2{H;x;p}')
assert_equal "$(printf '\na\nb\n')" "$X" "H appends the pattern space to the hold space with a newline separator"

## ---------------------------------------------------------------------
## a i c
## ---------------------------------------------------------------------

X=$(printf 'x\n' | sed 'a\
appended text')
assert_equal "$(printf 'x\nappended text\n')" "$X" "a\\ (POSIX form) queues text after the current line"

X=$(printf 'x\n' | sed 'a appended text')
assert_equal "$(printf 'x\nappended text\n')" "$X" "a text (GNU one-line form) is accepted the same way"

X=$(printf 'x\n' | sed 'i inserted text')
assert_equal "$(printf 'inserted text\nx\n')" "$X" "i writes text immediately, before the current line"

X=$(printf 'x\ny\n' | sed '1c changed')
assert_equal "$(printf 'changed\ny\n')" "$X" "c (single address) replaces the pattern space with text, once per matching line"

X=$(printf '1\n2\n3\n4\n' | sed '2,3c changed')
assert_equal "$(printf '1\nchanged\n4\n')" "$X" "c (2-address range) replaces the whole range with text exactly once"

X=$(printf 'one\ntwo\nthree\n' | sed '2a\
line one\
line two')
assert_equal "$(printf 'one\ntwo\nline one\nline two\nthree\n')" "$X" "a\\ text spanning multiple backslash-continued lines"

## POSIX only defines backslash-escaping for a/i/c text, not leading-
## blank stripping -- indentation the script author wrote is text
## content and must survive verbatim.
X=$(printf 'x\n' | sed 'a\
   indented')
assert_equal "$(printf 'x\n   indented\n')" "$X" "a\\ text preserves a line's own leading blanks (not stripped)"

## ---------------------------------------------------------------------
## y (transliterate)
## ---------------------------------------------------------------------

X=$(printf 'abcabc\n' | sed 'y/abc/xyz/')
assert_equal "xyzxyz" "$X" "y transliterates every listed character"

X=$(printf 'Hello\n' | sed 'y/abcdefghijklmnopqrstuvwxyz/ABCDEFGHIJKLMNOPQRSTUVWXYZ/')
assert_equal "HELLO" "$X" "y with a full (non-range) from/to character list"

## ---------------------------------------------------------------------
## labels: b t
## ---------------------------------------------------------------------

X=$(printf 'aaa\n' | sed ':x;s/a/b/;tx')
assert_equal "bbb" "$X" "t branches back while the last s/// succeeded, looping until it doesn't"

X=$(printf 'skip\n' | sed 'b end;s/skip/no/;:end')
assert_equal "skip" "$X" "b unconditionally branches, skipping the s/// in between"

X=$(printf 'a\n' | sed '/a/bx;s/a/no/;:x')
assert_equal "a" "$X" "an address on b makes the branch conditional on the address, not on tflag"

## ---------------------------------------------------------------------
## multiple -e, -f, #n
## ---------------------------------------------------------------------

X=$(printf 'abc\n' | sed -e 's/a/1/' -e 's/b/2/')
assert_equal "12c" "$X" "multiple -e fragments run as one script, in order"

cat > script.sed <<'EOF'
s/foo/bar/
EOF
X=$(printf 'foo\n' | sed -f script.sed)
assert_equal "bar" "$X" "-f reads the script from a file"

printf '#n\np\n' > autoprint.sed
X=$(printf 'once\n' | sed -f autoprint.sed)
assert_equal "once" "$X" "a leading #n in the script suppresses auto-print, same as -n"

## ---------------------------------------------------------------------
## w / r (to files and to /dev/stdout)
## ---------------------------------------------------------------------

X=$(printf 'x\ny\n' | sed -n '1w /dev/stdout')
assert_equal "x" "$X" "w /dev/stdout writes to standard output"

rm -f out.txt
printf 'a\nb\nc\n' | sed -n '2w out.txt' >/dev/null
assert_equal "b" "$(cat out.txt)" "w file writes the matching line(s) to that file"

echo "inserted" > r.txt
X=$(printf 'a\nb\n' | sed '1r r.txt')
assert_equal "$(printf 'a\ninserted\nb\n')" "$X" "r queues a file's content right after the current line"

X=$(printf 'a\n' | sed '1r /no/such/file' 2>/dev/null)
assert_equal "a" "$X" "r on a missing file is silently ignored (no error, no output)"

## ---------------------------------------------------------------------
## l (unambiguous listing)
## ---------------------------------------------------------------------

X=$(printf 'a\tb\n' | sed -n 'l')
assert_equal 'a\tb$' "$X" "l escapes a tab as \\\\t and marks the line end with \$"

X=$(printf 'a\\b\n' | sed -n 'l')
assert_equal 'a\\b$' "$X" "l escapes a literal backslash as \\\\"

## ---------------------------------------------------------------------
## comments, semicolons, blank lines between commands
## ---------------------------------------------------------------------

X=$(printf 'a\n' | sed '
# this is a comment
s/a/b/  # not a trailing comment (only recognized between commands)
')
assert_match "$X" "*b*" "a '#' comment line between commands is skipped"

X=$(printf 'a\n' | sed 's/a/b/;s/b/c/;s/c/d/')
assert_equal "d" "$X" "';' separates several commands on one line"

## ---------------------------------------------------------------------
## no-trailing-newline round-trip
## ---------------------------------------------------------------------

## wc -c pads its count, hence the "echo $X" to collapse it back to a
## bare number via field splitting.
X=$(printf 'noeol' | sed 's/noeol/yes/' | wc -c)
X=$(echo $X)
assert_equal "3" "$X" "a final line with no trailing newline round-trips without adding one"

X=$(printf 'a\nb' | sed -n '$p' | wc -c)
X=$(echo $X)
assert_equal "1" "$X" "\$ still selects the true last line even when it lacks a trailing newline"

## ---------------------------------------------------------------------
## multiple input files, concatenated with one line numbering / \$
## ---------------------------------------------------------------------

printf '1\n2\n' > f1.txt
printf '3\n4\n' > f2.txt
X=$(sed -n '$p' f1.txt f2.txt)
assert_equal "4" "$X" "\$ means the last line of the LAST file, not of each file"

X=$(sed -n '3p' f1.txt f2.txt)
assert_equal "3" "$X" "line numbers count across concatenated file operands"

## ---------------------------------------------------------------------
## exit status
## ---------------------------------------------------------------------

( printf 'a\n' | sed 's/a/b/' >/dev/null )
assert_equal "0" "$?" "a normal run without q exits 0"

summary
