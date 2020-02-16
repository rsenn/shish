case "$0" in
  -*) . ./cfg-cmake.sh ;;
  *) . "$(dirname "$0")"/cfg-cmake.sh ;;
esac
do-cfg () 
{ 
    builddir=build/i686-linux-diet prefix=/opt/diet cfg-diet32 -DENABLE_ALL_BUILTINS=TRUE -DDEBUG_{ALLOC,COLOR,OUTPUT}=ON;
    builddir=build/x86_64-linux-diet prefix=/opt/diet cfg-diet64 -DENABLE_ALL_BUILTINS=TRUE -DDEBUG_{ALLOC,COLOR,OUTPUT}=ON;
    builddir=build/i686-linux-gnu CC=i686-linux-gnu-gcc cfg -DENABLE_ALL_BUILTINS=TRUE -DDEBUG_{ALLOC,COLOR,OUTPUT}=ON;
    builddir=build/x86_64-linux-gnu CC=gcc cfg -DENABLE_ALL_BUILTINS=TRUE -DDEBUG_{ALLOC,COLOR,OUTPUT}=ON
}
