. "$(dirname "$0")/common.sh"

## POSIX 2.7.4 here-document forms: <<word and <<-word, with word
## unquoted/single-quoted/double-quoted/backslash-escaped/mixed-quoted,
## plus interaction with pipelines, subshells, command substitution,
## multiple here-docs on one command line, and tab-stripping.

## --- unquoted delimiter: body is expanded like a double-quoted string ---
X=hi
Y=$(cat <<EOF
plain $X
EOF
)
assert_equal "plain hi" "$Y" "unquoted delimiter expands parameters"

Y=$(cat <<EOF
arith $((1 + 2))
EOF
)
assert_equal "arith 3" "$Y" "unquoted delimiter expands arithmetic"

Y=$(cat <<EOF
cmdsub $(echo inner)
EOF
)
assert_equal "cmdsub inner" "$Y" "unquoted delimiter expands command substitution"

Y=$(cat <<EOF
line one \
line two
EOF
)
assert_equal "line one line two" "$Y" "backslash-newline in an unquoted here-doc body is a line continuation"

Y=$(cat <<EOF
keep \$ literal-dollar
EOF
)
assert_equal 'keep $ literal-dollar' "$Y" "backslash-escaped \$ in an unquoted here-doc body suppresses expansion"

## --- quoted delimiter forms: body is treated literally, no expansion ---
X=hi

Y=$(cat <<'EOF'
literal $X
EOF
)
assert_equal 'literal $X' "$Y" "single-quoted delimiter suppresses expansion"

Y=$(cat <<"EOF"
literal $X
EOF
)
assert_equal 'literal $X' "$Y" "double-quoted delimiter suppresses expansion"

Y=$(cat <<\EOF
literal $X
EOF
)
assert_equal 'literal $X' "$Y" "backslash-escaped delimiter suppresses expansion"

Y=$(cat <<E"O"F
literal $X
EOF
)
assert_equal 'literal $X' "$Y" "delimiter mixing quoted and unquoted parts suppresses expansion"

Y=$(cat <<'EOF'
a\\b
EOF
)
assert_equal 'a\\b' "$Y" "a quoted-delimiter here-doc body preserves a real backslash verbatim"

## --- <<- strips leading tabs but not leading spaces ---
Y=$(cat <<-EOF
	tabbed line
	EOF
)
assert_equal "tabbed line" "$Y" "<<- strips leading tabs from body and the closing delimiter line"

Y=$(cat <<-EOF
	line with $X
	EOF
)
assert_equal "line with hi" "$Y" "<<- still expands parameters when the delimiter is unquoted"

Y=$(cat <<-'EOF'
	literal $X
	EOF
)
assert_equal 'literal $X' "$Y" "<<- combined with a quoted delimiter both strips tabs and suppresses expansion"

Y=$(cat <<-EOF
   spaced line
	EOF
)
assert_equal "   spaced line" "$Y" "<<- does not strip leading spaces, only tabs"

## --- empty body ---
Y=$(cat <<EOF
EOF
)
assert_equal "" "$Y" "a here-doc with an empty body produces empty output"

## --- multiple here-docs on the same command line, consumed in order ---
Y=$(cat <<A <<B
first
A
second
B
)
assert_equal "second" "$Y" "the later of two here-docs redirected to the same fd wins"

Y=$(cat <<A <<B <<C
one
A
two
B
three
C
)
assert_equal "three" "$Y" "with three here-docs on the same fd, the last one still wins cleanly"

Y=$(
  cat <<A
one
A
  cat <<B
two
B
)
assert_equal "one
two" "$Y" "two here-docs on separate simple commands are each read in source order"

## --- here-doc as part of a pipeline ---
Y=$(cat <<EOF | tr a-z A-Z
lowercase
EOF
)
assert_equal "LOWERCASE" "$Y" "a here-doc attaches to its command before the pipeline runs"

## --- here-doc combined with another redirection on the same command ---
TESTDIR=$(mktemp -d)
cat <<EOF >"$TESTDIR/out"
redirected
EOF
Y=$(cat "$TESTDIR/out")
assert_equal "redirected" "$Y" "a here-doc coexists with an unrelated output redirection on the same command"
rm -rf "$TESTDIR"

## --- here-doc inside a subshell / command substitution ---
Y=$(
  Z=$(cat <<EOF
nested
EOF
)
  echo "$Z"
)
assert_equal "nested" "$Y" "a here-doc inside a command substitution works"

## --- here-doc feeding a loop ---
Y=$(
  while read -r line; do
    echo "got:$line"
  done <<EOF
a
b
EOF
)
assert_equal "got:a
got:b" "$Y" "a here-doc can be attached to a compound command (while loop)"

## --- here-doc whose body itself looks like shell syntax ---
Y=$(cat <<'EOF'
if [ 1 = 1 ]; then echo yes; fi
EOF
)
assert_equal 'if [ 1 = 1 ]; then echo yes; fi' "$Y" "here-doc body is treated as data, not parsed as shell syntax"

summary
