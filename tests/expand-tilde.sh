DIR=$(dirname "${0}")
. "$DIR/common.sh"

## Testing tilde expansion (~, ~user)

X=$(echo ~)
assert_equal "$HOME" "$X" "a bare ~ expands to \$HOME"

X=$(echo ~/foo)
assert_equal "$HOME/foo" "$X" "~/foo expands to \$HOME/foo"

X=$(echo "~")
assert_equal "~" "$X" "a double-quoted ~ is never expanded"

X=$(echo '~')
assert_equal "~" "$X" "a single-quoted ~ is never expanded"

X=$(echo ~root)
assert_equal "/root" "$X" "~user expands via that user's passwd entry"

X=$(echo ~this_user_almost_certainly_does_not_exist_12345)
assert_equal "~this_user_almost_certainly_does_not_exist_12345" "$X" \
  "~unknownuser is left completely unmodified, not an error"

X=$( (HOME=/tmp/shish-tilde-test; echo ~) )
assert_equal "/tmp/shish-tilde-test" "$X" "~ respects a HOME reassignment"

## tilde in an assignment's value

X=$(X=~/foo; echo "$X")
assert_equal "$HOME/foo" "$X" "an assignment's value (X=~/foo) is tilde-expanded too"

X=$(X=~; echo "$X")
assert_equal "$HOME" "$X" "a bare ~ as a whole assignment value expands the same way"

X=$(X=~root:~this_user_almost_certainly_does_not_exist_12345:literal; echo "$X")
assert_equal "/root:~this_user_almost_certainly_does_not_exist_12345:literal" "$X" \
  "each ~-prefix following an unquoted ':' in an assignment value is expanded independently"

## a tilde-expansion's result is not itself subject to further
## expansions -- POSIX 2.6.1

X=$( (HOME='/path/with  space'; set -- ~; echo "$#:$1") )
assert_equal "1:/path/with  space" "$X" \
  "a tilde-expansion's result is not subject to field splitting, even if it contains IFS whitespace"

X=$( (HOME='$(echo X)`echo Y`'; echo ~) )
assert_equal '$(echo X)`echo Y`' "$X" \
  "a tilde-expansion's result is used as a literal string, not re-expanded even if it looks like a substitution"

## the permanent parse tree must never be mutated by expansion -- the
## same assignment/argument node, evaluated repeatedly (e.g. inside a
## loop), must resolve ~ fresh every time, not reuse a stale result
## baked in on the first pass

X=""
for i in 1 2; do
  X="$X$(echo ~/x)."
done
assert_equal "$HOME/x.$HOME/x." "$X" "~ expands fresh on every loop iteration, not just the first"

Y=""
for i in 1 2; do
  Z=~/y
  Y="$Y$Z."
done
assert_equal "$HOME/y.$HOME/y." "$Y" "same, for tilde inside a repeatedly-evaluated assignment"

summary
