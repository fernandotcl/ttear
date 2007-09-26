#!/bin/sh
aclocal || exit
autoheader || exit
automake --add-missing || exit
autoconf || exit
