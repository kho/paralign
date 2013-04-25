ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = $(BOOST_CPPFLAGS)

noinst_LTLIBRARIES = libparalign.la

libparalign_la_SOURCES = src/io.h src/options.h src/options.cc src/ttable.h src/types.h src/contrib/log.h src/contrib/da.h

bin_PROGRAMS = estimate pa-mapper pa-reducer pa-init-ttable pa-dump-ttable
bin_SCRIPTS = pa-env.sh pa-corpus.py

estimate_SOURCES = src/estimate.cc

pa_mapper_SOURCES = src/mapper.cc
pa_mapper_LDADD = libparalign.la

pa_reducer_SOURCES = src/reducer.cc
pa_reducer_LDADD = libparalign.la
pa_reducer_LDFLAGS = -lhdfs $(LIBJVM)

pa_init_ttable_SOURCES = src/init_ttable.cc
pa_init_ttable_LDADD = libparalign.la
pa_init_ttable_LDFLAGS = -lhdfs $(LIBJVM)

pa_dump_ttable_SOURCES = src/dump_ttable.cc
pa_dump_ttable_LDADD = libparalign.la

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
