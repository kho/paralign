ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = -I src

noinst_LTLIBRARIES = libparalign.la

libparalign_la_SOURCES = src/log.h src/options.h src/options.cc

check_PROGRAMS = options_test
options_test_SOURCES = src/test/options_test.cc
options_test_LDADD = libparalign.la
