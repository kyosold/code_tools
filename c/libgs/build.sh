#!/bin/bash

echo "aclocal"
aclocal

echo "libtoolize -f -c"
libtoolize -f -c

echo "autoconf"
autoconf

echo "automake --add-missing"
automake --add-missing
