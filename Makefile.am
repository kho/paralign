ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = $(BOOST_CPPFLAGS)

noinst_LTLIBRARIES = libparalign.la

libparalign_la_SOURCES = src/io.h src/options.h src/options.cc src/ttable.h src/types.h src/contrib/log.h src/contrib/da.h

bin_PROGRAMS = estimate

estimate_SOURCES = src/estimate.cc

check_PROGRAMS = io_test ttable_test
TESTCPPFLAGS = -I src $(AM_CPPFLAGS)
TESTLDFLAGS = $(BOOST_UNIT_TEST_FRAMEWORK_LDFLAGS) $(BOOST_UNIT_TEST_FRAMEWORK_LIBS)

io_test_SOURCES = src/test/io_test.cc
io_test_LDADD = libparalign.la
io_test_CPPFLAGS = $(TESTCPPFLAGS)
io_test_LDFLAGS = $(TESTLDFLAGS)

ttable_test_SOURCES = src/test/ttable_test.cc
ttable_test_LDADD = libparalign.la
ttable_test_CPPFLAGS = $(TESTCPPFLAGS)
ttable_test_LDFLAGS = $(TESTLDFLAGS)
