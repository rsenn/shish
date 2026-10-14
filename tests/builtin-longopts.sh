DIR=$(dirname "${0}")
. "$DIR/common.sh"

## long options behave like their short forms (only the builtins that are compiled in)

is_builtin() {
  case $(command -V "$1" 2>&1) in
    *"built-in"*) return 0 ;;
  esac
  return 1
}

TESTDIR=$(mktemp -d)
cd "$TESTDIR" || exit 1
printf 'b\na\nB\na\n' >in
mkdir -p d/sub
: >d/f
: >d/g
chmod +x d/f

# same output and status for the short and the long spelling
same() {
  desc=$1 short=$2 long=$3
  a=$(eval "$short" 2>&1; echo "rc=$?")
  b=$(eval "$long" 2>&1; echo "rc=$?")
  assert_equal "$a" "$b" "$desc"
}

is_builtin sort && {
  same "sort --reverse" 'sort -r in' 'sort --reverse in'
  same "sort --unique --ignore-case" 'sort -u -f in' 'sort --unique --ignore-case in'
  same "sort --key=1 --field-separator=:" 'sort -k1 -t: in' 'sort --key=1 --field-separator=: in'
  same "sort --output=FILE" 'sort -o o1 in; cat o1' 'sort --output=o2 in; cat o2'
}
is_builtin uniq && same "uniq --count" 'sort in | uniq -c' 'sort in | uniq --count'
is_builtin grep && {
  same "grep --ignore-case --line-number" 'grep -i -n a in' 'grep --ignore-case --line-number a in'
  same "grep --invert-match --count" 'grep -v -c a in' 'grep --invert-match --count a in'
  same "grep --regexp=PAT" 'grep -e a in' 'grep --regexp=a in'
  same "grep --regexp PAT (separate word)" 'grep -e a in' 'grep --regexp a in'
}
is_builtin head && same "head --lines=2" 'head -n 2 in' 'head --lines=2 in'
is_builtin tail && same "tail --lines=1" 'tail -n 1 in' 'tail --lines=1 in'
is_builtin wc && same "wc --lines --words" 'wc -l -w in' 'wc --lines --words in'
is_builtin cat && same "cat --number" 'cat -n in' 'cat --number in'
is_builtin cut && same "cut --fields=1 --delimiter=a" 'cut -f1 -da in' 'cut --fields=1 --delimiter=a in'
is_builtin tr && same "tr --delete" 'tr -d a <in' 'tr --delete a <in'
is_builtin basename && same "basename --suffix" 'basename -s .c x/y.c' 'basename --suffix=.c x/y.c'
is_builtin mkdir && {
  same "mkdir --parents --verbose" 'mkdir -pv m1/a/b' 'mkdir --parents --verbose m2/a/b | sed s/m2/m1/'
  same "mkdir --mode" 'mkdir -m 700 m3; ls -ld m3 | cut -c1-10' 'mkdir --mode=700 m4; ls -ld m4 | cut -c1-10'
}
is_builtin rm && {
  : >r1 && : >r2
  rm -f r1 && rm --force r2; assert_equal "no no" "$(test -e r1 || echo no) $(test -e r2 || echo no)" "rm --force"
  mkdir r3 && rm --recursive r3; assert_equal 1 "$(test -e r3; echo $?)" "rm --recursive"
}
is_builtin ln && { ln --symbolic f l1; assert_equal f "$(readlink l1)" "ln --symbolic"; }
is_builtin readlink && same "readlink --canonicalize" 'readlink -f d/sub' 'readlink --canonicalize d/sub'
is_builtin realpath && same "realpath --relative-to" 'realpath --relative-to=d d/sub' 'realpath --relative-to d d/sub'
is_builtin id && same "id --user" 'id -u' 'id --user'
is_builtin uname && same "uname --kernel-name" 'uname -s' 'uname --kernel-name'
is_builtin env && same "env --ignore-environment" 'env -i echo x' 'env --ignore-environment echo x'
is_builtin xargs && same "xargs --max-args" 'echo a b c | xargs -n 1 echo' 'echo a b c | xargs --max-args=1 echo'
is_builtin tee && same "tee --append" 'echo x | tee -a t1' 'echo x | tee --append t1'
is_builtin chmod && { chmod --verbose 600 d/g >/dev/null; assert_equal "-rw-------" "$(ls -l d/g | cut -c1-10)" "chmod --verbose"; }
is_builtin timeout && {
  timeout --signal=KILL --kill-after=1 5 true; assert_equal 0 "$?" "timeout --signal --kill-after"
  timeout --preserve-status 0.2 sleep 5 >/dev/null 2>&1; assert_equal 143 "$?" "timeout --preserve-status"
}
is_builtin mktemp && { t=$(mktemp --directory); assert_equal 0 "$(test -d "$t"; echo $?)" "mktemp --directory"; rmdir "$t"; }

# a long option that wants a value says so, and one that doesn't want one rejects it
is_builtin sort && {
  sort --key 2>/dev/null; assert_equal 2 "$?" "a missing long-option value is a usage error"
  sort --reverse=1 in 2>/dev/null; assert_equal 2 "$?" "a flag with =value is a usage error"
}

# every long name in a table shows up in the help text
for b in sort grep head tail wc cat cut tr rm mkdir touch realpath timeout date ls; do
  is_builtin "$b" || continue
  h=$(help "$b")
  case $b in
    sort) names='reverse unique key output' ;;
    grep) names='ignore-case invert-match regexp' ;;
    head | tail) names='lines bytes' ;;
    wc) names='bytes chars lines words' ;;
    cat) names='number squeeze-blank' ;;
    cut) names='fields delimiter' ;;
    tr) names='delete complement' ;;
    rm) names='force recursive' ;;
    mkdir) names='parents mode' ;;
    touch) names='date reference' ;;
    realpath) names='relative-to' ;;
    timeout) names='signal kill-after' ;;
    date) names='date reference iso-8601 rfc-email' ;;
    ls) names='color all recursive' ;;
  esac
  for n in $names; do
    assert_match "$h" "*--$n*" "help $b lists --$n"
  done
done

# date -d / -r / -I / -R
if is_builtin date; then
  export TZ=UTC
  assert_equal "1970-01-02" "$(date -d @86400 +%F)" "date -d @SECONDS"
  assert_equal "2020-01-02T03:04:05+00:00" "$(date -d '2020-01-02 03:04:05' -Iseconds)" "date -Iseconds"
  assert_equal "2020-01-02T03:04:05+00:00" "$(date --date='2020-01-02T03:04:05' --iso-8601=seconds)" "date --iso-8601=seconds"
  assert_equal "2020-01-02" "$(date -d '2020-01-02 03:04:05' -I)" "date -I is the date"
  assert_equal "Thu, 02 Jan 2020 03:04:05 +0000" "$(date -d '2020-01-02 03:04:05' -R)" "date -R"
  assert_equal "2020-01-02 03:04:05+00:00" "$(date -d '2020-01-02 03:04:05' --rfc-3339=seconds)" "date --rfc-3339=seconds"
  assert_equal "2020-01-01" "$(date -d '2020-01-02 -1 day' +%F)" "date -d relative tail"
  assert_equal "1970-01-03" "$(date -d '@86400 +1 day' +%F)" "date -d @N plus a relative tail"
  date -d 'not a date' >/dev/null 2>&1; assert_equal 1 "$?" "date -d rejects garbage"
  date -Ibogus >/dev/null 2>&1; assert_equal 2 "$?" "date -I with a bad format is a usage error"
  touch -d @1000000000 ref
  assert_equal "2001-09-09" "$(date -r ref +%F)" "date -r FILE"
  assert_equal "$(date -u +%F)" "$(date --utc +%F)" "date --utc"
  touch --date='2001-02-03 04:05:06' tdate
  assert_equal "2001-02-03 04:05:06" "$(date -r tdate '+%F %T')" "touch --date"
  touch --reference=ref tref
  assert_equal "2001-09-09" "$(date -r tref +%F)" "touch --reference"
fi

# ls --color
if is_builtin ls; then
  E=$(printf '\033')
  ln -s f lnk
  assert_equal "${E}[94msub${E}[0m" "$(ls --color=yes d | grep sub)" "ls --color=yes: directory light blue"
  assert_equal "${E}[33mf${E}[0m" "$(ls --color d | sed -n 1p)" "ls --color: executable yellow"
  assert_equal "${E}[36mlnk${E}[0m" "$(ls --color=always lnk)" "ls --color=always: symlink cyan"
  assert_equal "$(ls d)" "$(ls --color=no d)" "ls --color=no equals plain"
  assert_equal "$(ls d)" "$(ls --color=auto d | cat)" "ls --color=auto into a pipe is plain"
  assert_equal "${E}[94msub${E}[0m/" "$(ls --color=yes -F d | grep sub)" "the -F marker follows the reset sequence"
  ls --color=bogus d >/dev/null 2>&1; assert_equal 2 "$?" "ls --color=bogus is a usage error"
  : >d/aaaaaaaaaaaaaaaaaaa; : >d/b; : >d/c
  assert_equal "$(COLUMNS=30 ls -C d)" "$(COLUMNS=30 ls -C --color=yes d | sed "s/${E}\[[0-9;]*m//g")" "colours do not change the column layout"
fi

rm -rf "$TESTDIR"
summary
