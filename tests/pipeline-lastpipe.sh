DIR=$(dirname "${0}")
. "$DIR/common.sh"

## zsh/ksh run a foreground pipeline's last command in the current
## shell instead of forking it too, so its variable/cd/etc. changes
## are visible afterward -- bash needs "shopt -s lastpipe" to opt in,
## but shish (like zsh/ksh) does this unconditionally. See the
## "lastpipe" comments in src/eval/eval_pipeline.c.

echo gone | read -r VAR
assert_equal "gone" "$VAR" "a variable assigned by a pipeline's last stage (read) must persist"

echo hi | cat | read -r VAR2
assert_equal "hi" "$VAR2" "the last stage of a 3+ member pipeline must still run unforked"

echo /tmp | { read -r D; cd "$D"; }
assert_equal "/tmp" "$PWD" "cd in a pipeline's last stage must persist (braces don't add a subshell)"

## non-last stages stay isolated -- only the last one runs unforked
X=1
(X=2; true) | cat
assert_equal "1" "$X" "a non-last pipeline stage's variable change must NOT leak"

## exit status is still the last stage's, same as before this change
true | false
assert_equal "1" "$?" "pipeline exit status must be the last stage's (failure)"

false | true
assert_equal "0" "$?" "pipeline exit status must be the last stage's (success)"

## background pipelines are unaffected -- every member still forks
echo hi | cat >/tmp/pipeline-lastpipe.$$.out &
wait
assert_equal "hi" "$(cat /tmp/pipeline-lastpipe.$$.out)" "a backgrounded pipeline must still run to completion"
rm -f /tmp/pipeline-lastpipe.$$.out

summary
