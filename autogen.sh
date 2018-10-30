#!/bin/sh
#
# 20151015


aclocal --force -I m4 -I .
autoheader --force
autoconf --force
