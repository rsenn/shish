DIR=$(dirname "${0}")
. "$DIR/common.sh"

## Testing command substitution

## $(...) captures a simple command's output
OUTPUT=$(echo "This is output from an 'echo' command")
assert_equal "This is output from an 'echo' command" "$OUTPUT" "\$(...) must capture a simple command's stdout"

## a here-doc feeding a command whose output is captured by $(...),
## with parameter expansion inside the (unquoted-delimiter) here-doc
## body
X=1337
fn() {
  cat
}
HERE=$(fn <<EOF
Here doc content $X
EOF
)
assert_equal "Here doc content 1337" "$HERE" "a here-doc with an unquoted delimiter must expand parameters in its body"

## backquote form of command substitution
FORMATTED=`printf "%x" 255`
assert_equal "ff" "$FORMATTED" "backquote command substitution must capture printf's output"

## nested $(...) command substitution
NESTED=$(echo nested $(echo value))
assert_equal "nested value" "$NESTED" "\$(...) must nest without needing to escape the inner one"

## $(...) inside an assignment sees the rest of the script's state
## (e.g. a variable set earlier in the same command list)
Y=before
Z=$(echo "$Y-after")
assert_equal "before-after" "$Z" "\$(...) must see variables assigned earlier in the same script"

summary
