#!/bin/sh
#
# 20100623

MYDIR=`dirname "$0"`

cd "$MYDIR"

set -e
set -x

aclocal --force -I m4 
autoheader --force
autoconf --force

ESC=''
sed "s|###ESCAPE###|${ESC}\[|g" <configure >configure.tmp
rm -f configure
mv -f configure.tmp configure
chmod 755 configure
