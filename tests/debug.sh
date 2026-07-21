DIR=$(dirname "${0}")
. "$DIR/common.sh"

## Testing the "set -x" debug/trace flag and the "type" builtin

## "type" must identify shell builtins as such
##
## (via a temp file + "IFS= read -r" rather than "$(type cd)" --
## quoted command substitution currently doesn't suppress field
## splitting in shish for multi-word output, a separate, pre-existing
## bug logged in BUGS)
TYPEFILE=$(mktemp)
type cd >"$TYPEFILE"
IFS= read -r TYPELINE <"$TYPEFILE"
assert_equal "cd is a shell builtin" "$TYPELINE" "type must identify cd as a shell builtin"
type set >"$TYPEFILE"
IFS= read -r TYPELINE <"$TYPEFILE"
assert_equal "set is a shell builtin" "$TYPELINE" "type must identify set as a shell builtin"
rm -f "$TYPEFILE"

## "set -x" must not change program behavior, only trace it
set -x
RESULT=$( (true; false; :; echo ok) )
set +x
assert_equal "ok" "$RESULT" "set -x must not change what a command list actually does"

## a function's "set --" only changes its own positional params, not
## the caller's
set -- outer1 outer2
fn() {
  set -- a b c
  assert_equal "3" "$#" "a function's own \"set --\" changes its own \$#"
  assert_equal "a" "$1" "a function's own \"set --\" changes its own \$1"
}
fn
assert_equal "2" "$#" "the caller's \$# must survive a callee's \"set --\""
assert_equal "outer1" "$1" "the caller's \$1 must survive a callee's \"set --\""

## a brace group runs in the current shell (no subshell), with the
## same "does not leak positional params in" semantics as a function
{ fn; }
assert_equal "2" "$#" "a brace group must not leak a called function's positional params either"

summary
