DIR=$(dirname "${0}")
. "$DIR/common.sh"

## nl builtin (src/builtin/extra/builtin_nl.c); skipped when nl is external
case $(type nl) in
*builtin*) ;;
*) echo "nl is not a builtin, skipping" 1>&2; summary ;;
esac

assert_filter "default numbers non-empty lines only" "$(printf '     1\ta\n       \n     2\tb')" 'a\n\nb\n' "nl"
assert_filter "-ba numbers every line" "$(printf '     1\ta\n     2\t\n     3\tb')" 'a\n\nb\n' "nl -ba"
assert_filter "-n ln left-justifies" '1  :a' 'a\n' "nl -n ln -w 3 -s :"
assert_filter "-n rz zero-pads" "$(printf '0001\ta')" 'a\n' "nl -n rz -w 4"
assert_filter "-v and -i" "$(printf '    10\ta\n    15\tb')" 'a\nb\n' "nl -v 10 -i 5"
assert_filter "a negative start" "$(printf '    -1\ta\n     0\tb')" 'a\nb\n' "nl -v -1"
assert_filter "-bp numbers matching lines" "$(printf '     1\ta\n       b')" 'a\nb\n' "nl -bp'^a'"
assert_filter "-l2 numbers every second empty line" "$(printf '     1\ta\n       \n     2\t\n     3\tb')" 'a\n\n\nb\n' "nl -ba -l2"
assert_filter "delimiter lines print empty and restart numbering" "$(printf '     1\ta\n\n     1\tb')" 'a\n\\:\\:\nb\n' "nl"
assert_filter "-p does not restart" "$(printf '     1\ta\n\n     2\tb')" 'a\n\\:\\:\nb\n' "nl -p"
assert_filter "-d sets the delimiter; the footer after it is unnumbered" "$(printf '     1\ta\n\n       b')" 'a\nxy\nb\n' "nl -d xy"
assert_filter "-hn -fa: a footer is numbered when asked" "$(printf '\n     1\tf')" '\\:\nf\n' "nl -fa"

nl -b z </dev/null >/dev/null 2>&1
assert_equal "1" "$?" "a bad type is an error"
nl -n xx </dev/null >/dev/null 2>&1
assert_equal "1" "$?" "a bad format is an error"
nl -w 0 </dev/null >/dev/null 2>&1
assert_equal "1" "$?" "width 0 is an error"

summary
