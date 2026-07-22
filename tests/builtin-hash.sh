DIR=$(dirname "${0}")
. "$DIR/common.sh"

## Testing hash builtin

## a freshly hashed builtin shows up in the plain table
true
X=$(hash | grep -c 'true (builtin)')
assert_equal "1" "$X" "running a builtin once hashes it, and it shows up in the plain table"

## -d forgets a single entry
hash -d true >/dev/null
X=$(hash | grep -c 'true (builtin)')
assert_equal "0" "$X" "hash -d forgets exactly the named entry"

## -d on an entry that was never hashed fails
( hash -d this-name-was-never-hashed )
assert_equal "1" "$?" "hash -d on an unknown name fails with a non-zero exit status"

## -r forgets everything (checked via true/false specifically, not an
## overall empty table -- "hash | grep ..." itself hashes "hash" and
## "grep" as it runs, so the table is never actually empty right after)
true
false
X=$(hash | grep -c 'true (builtin)\|false (builtin)')
assert_equal "2" "$X" "sanity: the table has true and false entries before hash -r"
hash -r
X=$(hash | grep -c 'true (builtin)\|false (builtin)')
assert_equal "0" "$X" "hash -r forgets every remembered entry, including true and false"

## -p pathname name remembers an arbitrary name -> pathname mapping,
## and it's actually used the next time that name runs
hash -p /bin/echo my_hashed_echo
X=$(my_hashed_echo hi-from-hash-p)
assert_equal "hi-from-hash-p" "$X" "hash -p pathname name makes that name runnable via the given pathname"

## -l prints the -p entries above back out in a directly reusable form
hash -r
hash -p /bin/echo my_hashed_echo
X=$(hash -l)
assert_equal "hash -p /bin/echo my_hashed_echo" "$X" "hash -l reprints a hashed pathname entry as a reusable \"hash -p ...\" line"

## a bare name with no options hashes it without running it
hash -r
X=$(hash | grep -c 'true (builtin)')
assert_equal "0" "$X" "sanity: true is not hashed right after hash -r"
hash true
X=$(hash | grep -c 'true (builtin)')
assert_equal "1" "$X" "a bare \"hash name\" (no options) hashes that name without running it"

hash -r

summary
