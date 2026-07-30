DIR=$(dirname "${0}")
. "$DIR/common.sh"

## Testing brace expansion ({a,b,c})

X=$(echo {a,b,c})
assert_equal "a b c" "$X" "a plain comma-list expands to one word per alternative"

X=$(echo pre{1,2,3}post)
assert_equal "pre1post pre2post pre3post" "$X" "a prefix/suffix around the group is kept on every alternative"

X=$(echo {a,b}{1,2})
assert_equal "a1 a2 b1 b2" "$X" "two groups in the same word combine (Cartesian product)"

X=$(echo {nocomma})
assert_equal "{nocomma}" "$X" "a brace group with no comma at all is not a valid expansion, left literal"

X=$(echo "{a,b,c}")
assert_equal "{a,b,c}" "$X" "a quoted brace group is never expanded"

X=$(set +B; echo {a,b,c})
assert_equal "{a,b,c}" "$X" "set +B turns brace expansion off"

X=$(set +B; set -B; echo {a,b,c})
assert_equal "a b c" "$X" "set -B turns it back on"

FOO=mid
X=$(echo "{a,$FOO,c}")
assert_equal "{a,mid,c}" "$X" \
  "a word mixing a substitution with a brace group is left entirely literal (v1 scope limit)"

## interaction with pathname expansion: each brace-generated
## alternative is independently glob-matched afterward

BDIR=$(mktemp -d)
touch "$BDIR/file1.txt" "$BDIR/file2.log"
X=$(cd "$BDIR" && echo file{1,2}.*)
assert_equal "file1.txt file2.log" "$X" "glob still applies per-alternative after brace expansion"
rm -rf "$BDIR"

## the permanent parse tree must never be mutated -- repeated
## evaluation (e.g. inside a loop) must re-expand every time, not
## reuse stale, already-split results

X=""
for i in 1 2; do
  X="$X$(echo {a,b}).X"
done
assert_equal "a b.Xa b.X" "$X" "brace expansion re-runs fresh on every loop iteration"

summary
