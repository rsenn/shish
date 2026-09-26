DIR=$(dirname "${0}")
. "$DIR/common.sh"

## Filter chaining (TODO.md Goal 13): a pipeline whose entire non-last
## prefix is filter-capable builtins invoked with literal argv chains
## straight into the true last (lastpipe) stage through in-process
## buffers instead of fork()/pipe(), for any number of stages -- see
## the "filter chaining" comments in src/eval/eval_pipeline.c. Needs
## cat, grep and sed all compiled in (each is an individually
## toggleable EXTRA_BUILTINS entry).

for b in cat grep sed; do
  case $(type "$b" 2>&1) in
  *builtin*) ;;
  *)
    echo "$b builtin not compiled in, skipping" 1>&2
    summary
    ;;
  esac
done

TESTDIR=$(mktemp -d)
cd "$TESTDIR" || exit 1

cat >in.txt <<'EOF'
alpha 1
bravo 2
charlie 3
delta expr1 4
echo expr2 5
foxtrot 6
golf expr1 7
EOF

## ---------------------------------------------------------------------
## a 3-stage all-builtin chain (cat | grep -E | sed) produces the same
## output as running the stages standalone / via a real pipe would
## ---------------------------------------------------------------------
X=$(cat in.txt | grep -E "(expr1|expr2)" | sed -e "s/[0-9]/N/")
EXPECT=$(printf 'delta exprN 4\necho exprN 5\ngolf exprN 7\n')
assert_equal "$EXPECT" "$X" "cat | grep -E | sed chains and produces the right output"

## exit status is still the last stage's, same as any other pipeline
cat in.txt | grep -E "(expr1|expr2)" | sed -e "s/[0-9]/N/" >/dev/null
assert_equal "0" "$?" "a fully-chained pipeline's exit status is still the last stage's"

## ---------------------------------------------------------------------
## a longer chain (4 builtin stages) chains too, not just 2 or 3
## ---------------------------------------------------------------------
Y=$(cat in.txt | grep -E "(expr1|expr2)" | grep -v charlie | sed -e "s/[0-9]/N/")
assert_equal "$EXPECT" "$Y" "a 4-stage all-builtin chain (cat|grep|grep|sed) still works"

## ---------------------------------------------------------------------
## a middle stage that declines at open() time (grep -c: aggregate,
## not streaming) must still fall the whole pipeline back correctly,
## not just drop output or hang
## ---------------------------------------------------------------------
Z=$(cat in.txt | grep -c expr1 | sed -e "s/2/TWO/")
assert_equal "TWO" "$Z" "a declined chain candidate (grep -c) still falls back and runs correctly"

## ---------------------------------------------------------------------
## a non-builtin-only pipeline (external command in the middle) is
## unaffected -- still runs correctly via the ordinary fork()+pipe() path
## ---------------------------------------------------------------------
W=$(cat in.txt | /bin/cat | grep -E "(expr1|expr2)" | sed -e "s/[0-9]/N/")
assert_equal "$EXPECT" "$W" "a pipeline with a real external command in it still runs correctly"

cd /
rm -rf "$TESTDIR"

summary
