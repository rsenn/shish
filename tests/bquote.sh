build=`gcc -dumpmachine`
: ${build:=`gcc -dumpmachine`}

cfg() {
  : ${build:=`gcc -dumpmachine`}
}
