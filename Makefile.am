ACLOCAL_AMFLAGS = -I m4

noinst_LTLIBRARIES = libparalign.la

libparalign_la_SOURCES = src/io.h src/options.h src/options.cc src/ttable.h src/types.h src/contrib/log.h src/contrib/da.h
libparalign_la_CPPFLAGS = $(BOOST_CPPFLAGS)

bin_PROGRAMS = estimate

estimate_SOURCES = src/estimate.cc

check_PROGRAMS = options_test io_test

options_test_SOURCES = src/test/options_test.cc
options_test_CPPFLAGS = -I src
options_test_LDADD = libparalign.la

io_test_SOURCES = src/test/io_test.cc
io_test_CPPFLAGS = -I src $(BOOST_CPPFLAGS)
io_test_LDADD = libparalign.la
io_test_LDFLAGS = $(BOOST_UNIT_TEST_FRAMEWORK_LDFLAGS) $(BOOST_UNIT_TEST_FRAMEWORK_LIBS)
