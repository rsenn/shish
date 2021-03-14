if [ -e "$TOOLCHAIN" ]; then
  cmakebuild="TEST"
  cmakebuild=$(basename "$TOOLCHAIN" .cmake)
  cmakebuild=${cmakebuild%.toolchain}
  cmakebuild=${cmakebuild#toolchain-}
  : ${builddir=build/$cmakebuild}
else
  : ${builddir=build/$host}
fi
