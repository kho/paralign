AC_PREREQ([2.63])
AC_INIT([paralign], [0.1])
AM_INIT_AUTOMAKE([foreign -Wall subdir-objects])

AC_CONFIG_SRCDIR([src/io.h])
AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
AC_PROG_LIBTOOL

# Checks for libraries.
LIBJVM=`find "$JAVA_HOME/" -name libjvm.so`
AS_IF([test x$LIBJVM = x], AC_MSG_ERROR([Cannot find libjvm.so! Set proper JAVA_HOME when configure.]))
AC_SUBST([LIBJVM])

BOOST_REQUIRE([1.41])
BOOST_TEST

# Checks for header files.
AC_CHECK_HEADERS([hdfs.h])
AC_CHECK_HEADERS([stdint.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_CONFIG_FILES([Makefile scripts/pa-hadoop.bash scripts/pa-hadoop-test.bash scripts/test-run.bash])
AC_OUTPUT
