DIR=$(dirname "${0}")
. "$DIR/common.sh"

## pushd/popd/dirs (src/builtin/extra/builtin_dirstack.c); skipped unless all three are builtins
for b in pushd popd dirs; do
  case $(type $b) in
  *builtin*) ;;
  *) echo "$b is not a builtin, skipping" 1>&2; summary ;;
  esac
done

T=$(mktemp -d)
mkdir "$T/a" "$T/b" "$T/c" "$T/d"
HOME=/nonexistent
cd "$T" || exit 1

## the stack, in the order dirs prints it: cwd first
pushd a >/dev/null; pushd ../b >/dev/null; pushd ../c >/dev/null
assert_equal "$T/c $T/b $T/a $T" "$(dirs)" "dirs lists the cwd, then the stack from the top"
assert_equal "$T/b" "$(dirs +1)" "dirs +N counts from the left, starting at 0"
assert_equal "$T" "$(dirs -0)" "dirs -N counts from the right"

## rotation changes the order and the cwd, not the entries
pushd +2 >/dev/null
assert_equal "$T/a $T $T/c $T/b" "$(dirs)" "pushd +2 rotates entry 2 to the front"
assert_equal "$T/a" "$PWD" "pushd +N changes directory to the new first entry"
pushd >/dev/null
assert_equal "$T $T/a $T/c $T/b" "$(dirs)" "pushd without arguments swaps the top two"

## popd +N removes an entry without changing directory
popd +1 >/dev/null
assert_equal "$T $T/c $T/b" "$(dirs)" "popd +1 removes the second entry"
assert_equal "$T" "$PWD" "popd +N (N>0) stays in the current directory"
popd >/dev/null
assert_equal "$T/c $T/b" "$(dirs)" "popd removes the top entry and changes to it"
assert_equal "$T/c" "$PWD" "popd changes directory to the removed entry"

## errors do not disturb the stack
popd +9 >/dev/null 2>&1; assert_equal 1 "$?" "popd with an index out of range fails"
pushd /nonexistent/dir >/dev/null 2>&1; assert_equal 1 "$?" "pushd into a missing directory fails"
assert_equal "$T/c $T/b" "$(dirs)" "a failed pushd leaves the stack alone"

## a subshell's changes never reach the parent, however deep
dirs -c; cd "$T"; pushd a >/dev/null; pushd ../b >/dev/null
BEFORE=$(dirs)
( pushd ../c >/dev/null; ( popd >/dev/null; popd >/dev/null; pushd ../d >/dev/null ); pushd +1 >/dev/null; dirs -c )
assert_equal "$BEFORE" "$(dirs)" "pushd/popd/dirs -c in nested subshells leave the parent's stack unchanged"
assert_equal "$T/b" "$PWD" "and its directory"
X=$(pushd ../c >/dev/null; pushd ../d >/dev/null; dirs)
assert_equal "$T/d $T/c $T/b $T/a $T" "$X" "a command substitution starts from the parent's stack"
assert_equal "$BEFORE" "$(dirs)" "and does not change it"
( popd +1 >/dev/null; dirs -c )
assert_equal "$BEFORE" "$(dirs)" "popd +N in a subshell does not unlink the parent's node"

## a forked pipeline member keeps the stack it inherited, including its parent subshell's changes
dirs -c; cd "$T"
X=$( pushd a >/dev/null; { pushd ../b >/dev/null; dirs; } | cat )
assert_equal "$T/b $T/a $T" "$X" "a pipeline stage inside a subshell sees that subshell's pushd"

cd /
rm -rf "$T"

summary
