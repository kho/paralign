AC_PREREQ([2.65])
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
BOOST_REQUIRE([1.44])
BOOST_TEST

LIBJVM=`find "$JAVA_HOME/" -name libjvm.so`
AS_IF([test X$LIBJVM == X], AC_MSG_ERROR([Cannot find libjvm.so! Set proper JAVA_HOME when configure.]))
AC_SUBST([LIBJVM])

# Checks for header files.
AC_CHECK_HEADERS([hdfs.h])
AC_CHECK_HEADERS([stdint.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
