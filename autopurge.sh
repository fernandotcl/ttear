#!/bin/sh
test -f Makefile && ( make maintainer-clean || exit )
rm -rf autom4te.cache
rm -rf aclocal.m4 config.h.in configure depcomp install-sh Makefile.in missing etc/Makefile.in src/Makefile.in
rm -f autoscan.log configure.scan
