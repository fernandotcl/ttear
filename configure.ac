### Initialization macros
AC_INIT(ttear, 0.6-pre, fernandotcl@users.berlios.de)
AC_PREREQ(2.59)
AC_LANG(C++)
AC_CONFIG_SRCDIR(src/main.cpp)
AC_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE

# A function to add compiler options
add_compiler_opts() {
    for option in $* ; do
        SAVE_CXXFLAGS="$CXXFLAGS"
        CXXFLAGS="$option $CXXFLAGS"
        AC_MSG_CHECKING([whether $CXX accepts $option])
        AC_COMPILE_IFELSE([AC_LANG_PROGRAM([], [])], has_option=yes, has_option=no)
        AC_MSG_RESULT($has_option)
        if test "$has_option" = "no" ; then
            CXXFLAGS="$SAVE_CXXFLAGS"
        fi
    done
}
add_compiler_opts -Wall -Werror

### Programs
# Compiler/linker
AC_PROG_CC
AC_PROG_CXX

# pkg-config
AC_CHECK_PROG(pkg_config_exists, pkg-config, true)
if test ! "x$pkg_config_exists" = 'xtrue'; then
    AC_MSG_ERROR([Unable to find pkg-config, make sure it's in your \$PATH])
fi

### Compile-time options
# --enable-debug
AC_MSG_CHECKING([if debug mode is enabled])
AC_ARG_ENABLE(debug, [  --enable-debug          enable debug mode ], debug=yes)
if test "x$debug" = 'xyes'; then
    AC_MSG_RESULT(yes)
    AC_DEFINE_UNQUOTED(DEBUG, 1, [Define to compile with debug options enabled])
    add_compiler_opts -ggdb -O0
else
    AC_MSG_RESULT(no)
    add_compiler_opts -fomit-frame-pointer -O3
fi

### Libraries
# SDL
LIBS="$LIBS `pkg-config --libs sdl`"

### Headers
AC_CHECK_HEADERS([sys/time.h getopt.h time.h])

# SDL
sdl_includes=`pkg-config --cflags sdl`
AC_SUBST(sdl_includes)

### Functions
AC_CHECK_FUNCS([getopt_long gettimeofday])
AC_CHECK_FUNCS([bzero memset strchr strdup], [], [AC_MSG_ERROR([Required function is missing])])

### Types, compiler characterists
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T
AC_TYPE_UINT8_T

### Output
AC_OUTPUT([Makefile etc/Makefile src/Makefile])
