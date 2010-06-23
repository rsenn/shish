#!/bin/sh
cat "$@" | grep '^[a-zA-Z].*)$' | grep -v '^static' | sed 's/$/;/'
