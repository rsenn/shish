DIR=$(dirname "${0}")
. "$DIR/common.sh"

set -e

## Testing if-then-else

if [ -e "$TOOLCHAIN" ]; then
  cmakebuild="TEST"
  cmakebuild=$(basename "$TOOLCHAIN" .cmake)
  cmakebuild=${cmakebuild%.toolchain}
  cmakebuild=${cmakebuild#toolchain-}
  : ${builddir=build/$cmakebuild}
else
  : ${builddir=build/$host}
fi
