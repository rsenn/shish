#!/bin/sh
#
# 20151015


aclocal --force -I m4 -I .
autoheader --force
autoconf --force

sed -i 's|###ESCAPE###|\x1b[|' configure
