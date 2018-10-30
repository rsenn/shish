#!/bin/sh
#
# 20140328


aclocal --force
autoheader --force
automake --foreign --force --foreign --add-missing
aclocal --force
autoconf --force
