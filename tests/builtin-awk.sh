DIR=$(dirname "${0}")
. "$DIR/common.sh"

## Testing the awk builtin (text/awk + src/builtin/extra/builtin_awk.c).
## Off by default: needs -DBUILTIN_AWK=ON (it's in EXTRA_BUILTINS).

case $(type awk 2>&1) in
  *builtin*) ;;
  *)
    echo "awk builtin not compiled in (-DBUILTIN_AWK=ON), skipping" 1>&2
    summary
    ;;
esac

TESTDIR=$(mktemp -d)
cd "$TESTDIR" || exit 1

printf 'a b c\nd e f\n1 2 3\n' > fields.txt

## ---------------------------------------------------------------------
## fields, NR, NF
## ---------------------------------------------------------------------

X=$(awk '{print $2}' fields.txt | tr '\n' ,)
assert_equal "b,e,2," "$X" "\$2 selects the second field of every record"

X=$(awk '{print NR, NF}' fields.txt | tr '\n' ,)
assert_equal "1 3,2 3,3 3," "$X" "NR/NF are correct for every record"

X=$(awk '{$2="Z"; print}' fields.txt | tr '\n' ,)
assert_equal "a Z c,d Z f,1 Z 3," "$X" "assigning \$2 rebuilds \$0 with OFS"

X=$(awk '{NF=2; print}' fields.txt | tr '\n' ,)
assert_equal "a b,d e,1 2," "$X" "NF=2 truncates the record to two fields"

X=$(printf 'a:b:c\n' | awk -F: '{print $2}')
assert_equal "b" "$X" "-F sets FS before BEGIN"

X=$(awk 'BEGIN{$0="x,,y"; FS=","; print NF, $2}' </dev/null)
assert_equal "3 " "$X" "a single-character FS keeps empty fields"

## ---------------------------------------------------------------------
## BEGIN/END, patterns, ranges
## ---------------------------------------------------------------------

X=$(awk 'BEGIN{print "hi"}' </dev/null)
assert_equal "hi" "$X" "a BEGIN-only program never reads input"

X=$(awk '{s+=$3} END{print s}' fields.txt)
assert_equal "3" "$X" "END sees the accumulator built by every record's action"

X=$(printf '1\n2\n3\n' | awk '/2/{print "matched:" $0}')
assert_equal "matched:2" "$X" "a bare /re/ pattern matches against \$0"

X=$(printf '1\n2\n3\n4\n5\n' | awk '/2/,/4/' | tr '\n' ,)
assert_equal "2,3,4," "$X" "a range pattern selects lines between (and including) both matches"

## ---------------------------------------------------------------------
## arithmetic, comparison, string/numeric coercion
## ---------------------------------------------------------------------

X=$(awk 'BEGIN{print 2+3*4, 2^10, 7%3, -2^2}' </dev/null)
assert_equal "14 1024 1 -4" "$X" "operator precedence: * before +, ^ before unary -"

X=$(awk 'BEGIN{print (1<2), ("10"<"9"), (10<9)}' </dev/null)
assert_equal "1 1 0" "$X" "string \"10\"<\"9\" compares lexically, numeric 10<9 compares numerically"

X=$(awk 'BEGIN{if (x=="") print "empty"; if (x==0) print "zero"}' </dev/null | tr '\n' ,)
assert_equal "empty,zero," "$X" "an uninitialised variable compares equal to both \"\" and 0"

X=$(awk 'BEGIN{print "a" "b" 1+1}' </dev/null)
assert_equal "ab2" "$X" "concatenation binds looser than +"

## ---------------------------------------------------------------------
## string and math built-ins
## ---------------------------------------------------------------------

X=$(awk 'BEGIN{print substr("hello",2,3)}' </dev/null)
assert_equal "ell" "$X" "substr(s,m,n)"

X=$(awk 'BEGIN{print substr("hello",-2,5)}' </dev/null)
assert_equal "he" "$X" "substr clips a start position before 1 without losing the intended end"

X=$(awk 'BEGIN{print index("hello","ll")}' </dev/null)
assert_equal "3" "$X" "index() returns a 1-based position"

X=$(awk 'BEGIN{n=split("a:b:c",p,":"); print n, p[1], p[3]}' </dev/null)
assert_equal "3 a c" "$X" "split() fills an array and returns the field count"

X=$(awk 'BEGIN{s="hello"; n=sub(/l/,"L",s); print n, s}' </dev/null)
assert_equal "1 heLlo" "$X" "sub() replaces the first match and reports the count"

X=$(awk 'BEGIN{s="hello"; n=gsub(/l/,"L",s); print n, s}' </dev/null)
assert_equal "2 heLLo" "$X" "gsub() replaces every match"

X=$(awk 'BEGIN{print match("hello world","wor"), RSTART, RLENGTH}' </dev/null)
assert_equal "7 7 3" "$X" "match() sets RSTART/RLENGTH"

X=$(awk 'BEGIN{print toupper("abc"), tolower("ABC")}' </dev/null)
assert_equal "ABC abc" "$X" "toupper()/tolower()"

X=$(awk 'BEGIN{print int(3.9), int(-3.9), sqrt(16)}' </dev/null)
assert_equal "3 -3 4" "$X" "int() truncates toward zero"

## ---------------------------------------------------------------------
## printf
## ---------------------------------------------------------------------

X=$(awk 'BEGIN{printf "%5d|%-5s|%.2f\n", 42, "hi", 3.14159}' </dev/null)
assert_equal "   42|hi   |3.14" "$X" "printf width/left-justify/precision"

X=$(awk 'BEGIN{printf "%c%c\n", 65, "xyz"}' </dev/null)
assert_equal "Ax" "$X" "printf %c: a number is a byte value, a string its first character"

X=$(awk 'BEGIN{print sprintf("<%3d>", 7)}' </dev/null)
assert_equal "<  7>" "$X" "sprintf() returns the formatted string instead of printing it"

## ---------------------------------------------------------------------
## arrays: for-in, delete, SUBSEP
## ---------------------------------------------------------------------

X=$(awk 'BEGIN{a["x"]=1; a["y"]=2; n=0; for (k in a) n++; print n}' </dev/null)
assert_equal "2" "$X" "for (k in a) visits every element"

X=$(awk 'BEGIN{a[1]=1; a[2]=2; delete a[1]; for (k in a) print k}' </dev/null)
assert_equal "2" "$X" "delete a[i] removes one element"

X=$(awk 'BEGIN{a[1]=1; a[2]=2; delete a; n=0; for (k in a) n++; print n}' </dev/null)
assert_equal "0" "$X" "delete a (no subscript) clears the whole array"

X=$(awk 'BEGIN{a[1,2]="x"; print ((1,2) in a), ((1,3) in a)}' </dev/null)
assert_equal "1 0" "$X" "a multi-subscript index joins with SUBSEP for \"in\""

## ---------------------------------------------------------------------
## user functions: recursion, array pass-by-reference
## ---------------------------------------------------------------------

X=$(awk 'function fact(n){return n<=1?1:n*fact(n-1)} BEGIN{print fact(6)}' </dev/null)
assert_equal "720" "$X" "recursive user functions"

X=$(awk 'function fill(a){a["k"]=99} BEGIN{fill(x); print x["k"]}' </dev/null)
assert_equal "99" "$X" "an array argument is shared with the caller, not copied"

X=$(awk 'function inc(n){n=n+1; return n} BEGIN{y=1; z=inc(y); print y, z}' </dev/null)
assert_equal "1 2" "$X" "a scalar argument is passed by value"

## ---------------------------------------------------------------------
## control flow
## ---------------------------------------------------------------------

X=$(awk 'BEGIN{for(i=0;i<5;i++) s=s i; print s}' </dev/null)
assert_equal "01234" "$X" "a classic for(init;cond;post) loop"

X=$(awk 'BEGIN{i=0; while(1){i++; if(i>3)break}; print i}' </dev/null)
assert_equal "4" "$X" "break leaves the innermost loop"

X=$(awk 'BEGIN{for(i=0;i<5;i++){if(i==2)continue; s=s i}; print s}' </dev/null)
assert_equal "0134" "$X" "continue skips to the next iteration"

X=$(printf '1\n2\n3\n' | awk '{if ($1==2) next} {print}' | tr '\n' ,)
assert_equal "1,3," "$X" "next skips the rest of the rules for this record"

X=$(awk 'BEGIN{exit 3} END{print "end ran"}' </dev/null; echo "status=$?")
assert_match "$X" "end ran
status=3" "exit in BEGIN still runs END, and sets the exit status"

## ---------------------------------------------------------------------
## getline
## ---------------------------------------------------------------------

X=$(awk 'BEGIN{while ((getline line < "fields.txt") > 0) n++; print n}' </dev/null)
assert_equal "3" "$X" "getline var < file reads every line of a file"

summary
