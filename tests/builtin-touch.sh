DIR=$(dirname "${0}")
. "$DIR/common.sh"

## Testing touch builtin

TESTDIR=$(mktemp -d)
cd "$TESTDIR" || exit 1

## touch creates a missing file
touch newfile
assert_equal "0" "$?" "touch on a missing file succeeds"
X=$(test -f newfile; echo $?)
assert_equal "0" "$X" "the file actually exists afterward"

## -t sets an explicit [[CC]YY]MMDDhhmm[.ss] stamp
touch -t 202301021530.45 t1
X=$(stat -c '%Y' t1)
EXPECT=$(date -d '2023-01-02 15:30:45' +%s 2>/dev/null || date -j -f '%Y-%m-%d %H:%M:%S' '2023-01-02 15:30:45' +%s)
assert_equal "$EXPECT" "$X" "-t sets an exact [[CC]YY]MMDDhhmm[.ss] timestamp"

## -d accepts an ISO date, defaulting the time of day to midnight
touch -d 2020-05-04 d1
X=$(stat -c '%Y' d1)
EXPECT=$(date -d '2020-05-04' +%s 2>/dev/null || date -j -f '%Y-%m-%d' '2020-05-04' +%s)
assert_equal "$EXPECT" "$X" "-d with a bare date sets midnight of that day"

## --date=@N sets an exact epoch time
touch --date=@0 d2
X=$(stat -c '%Y' d2)
assert_equal "0" "$X" "--date=@0 sets the Unix epoch"

## -r copies another file's times
touch -d 2020-05-04 ref
touch -r ref rfile
X=$(stat -c '%Y' rfile)
Y=$(stat -c '%Y' ref)
assert_equal "$Y" "$X" "-r copies the reference file's modification time"

## -a changes only the access time, preserving modification time
touch -d 2000-01-01 base
touch -a -d 2010-01-01 base
X=$(stat -c '%X %Y' base)
EXPECT_A=$(date -d '2010-01-01' +%s 2>/dev/null || date -j -f '%Y-%m-%d' '2010-01-01' +%s)
EXPECT_M=$(date -d '2000-01-01' +%s 2>/dev/null || date -j -f '%Y-%m-%d' '2000-01-01' +%s)
assert_equal "$EXPECT_A $EXPECT_M" "$X" "-a updates only the access time"

## -m changes only the modification time, preserving access time
touch -m -d 2015-01-01 base
X=$(stat -c '%X %Y' base)
EXPECT_M=$(date -d '2015-01-01' +%s 2>/dev/null || date -j -f '%Y-%m-%d' '2015-01-01' +%s)
assert_equal "$EXPECT_A $EXPECT_M" "$X" "-m updates only the modification time"

## --time=WORD is a synonym for -a / -m; also regression-tests
## fixes/211 -- the long-option extraction used to corrupt argv via
## an overlap bug in byte_copyr(), silently dropping the following
## "-d 2011-01-01" and leaving both times at "now".
touch -d 2000-01-01 wbase
touch --time=mtime -d 2011-01-01 wbase
X=$(stat -c '%Y' wbase)
EXPECT_M=$(date -d '2011-01-01' +%s 2>/dev/null || date -j -f '%Y-%m-%d' '2011-01-01' +%s)
assert_equal "$EXPECT_M" "$X" "--time=mtime -d survives a preceding removed --time=WORD"

## -f is accepted and ignored
touch -f newfile
assert_equal "0" "$?" "-f is accepted and ignored"

## specifying more than one time source is an error
touch -d 2020-01-01 -r ref x 2>/dev/null
assert_equal "1" "$?" "specifying both -d and -r is an error"

## an invalid --time value is an error
touch --time=bogus x 2>/dev/null
assert_equal "1" "$?" "an invalid --time value is an error"

## missing operand is an error
touch 2>/dev/null
assert_equal "1" "$?" "touch requires an operand"

cd /
rm -rf "$TESTDIR"

summary
