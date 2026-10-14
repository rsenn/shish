DIR=$(dirname "${0}")
. "$DIR/common.sh"

## --help and bad options of every builtin that parses options with shell_getopt

is_builtin() {
  case $(command -V "$1" 2>&1) in
    *"built-in"*) return 0 ;;
  esac
  return 1
}

for b in ls cp mv rm ln mkdir rmdir chmod mktemp readlink realpath basename date touch timeout \
  id uname split tee wc cat grep sed sort uniq head tail cut tr xargs env which hostname; do
  is_builtin "$b" || continue

  "$b" --help >out.$$ 2>err.$$
  assert_equal 0 "$?" "$b --help exits 0"
  assert_equal "" "$(cat err.$$)" "$b --help writes nothing to stderr"
  help "$b" >help.$$ 2>&1
  assert_equal "$(cat help.$$)" "$(cat out.$$)" "$b --help prints what 'help $b' prints"

  "$b" --no-such-option >out.$$ 2>err.$$
  r=$?
  case $b in
    timeout | env) assert_equal 125 "$r" "$b --no-such-option exits 125" ;;
    *) assert_equal 2 "$r" "$b --no-such-option exits 2" ;;
  esac
  assert_match "$(cat err.$$)" '*--no-such-option: invalid option*' "$b names the bad option"
  assert_match "$(cat err.$$)" '*usage: '"$b"' *' "$b prints a usage line"
done

# --help after the command name of a wrapper belongs to the wrapped command
is_builtin env && {
  assert_equal "x" "$(env echo x --help | sed 's/ --help//')" "env passes --help on to the command"
}

rm -f out.$$ err.$$ help.$$
summary
