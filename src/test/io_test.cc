#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE io_test
#include <boost/test/unit_test.hpp>

#include <string>
#include <sstream>
#include <vector>

#include "io.h"
#include "types.h"

using namespace std;
using namespace paralign;

BOOST_AUTO_TEST_CASE( MapperSourceEmptyFile ) {
  istringstream strm("");
  MapperSource in(strm);
  BOOST_CHECK_EQUAL(in.Done(), true);
}

BOOST_AUTO_TEST_CASE( MapperSourceSingleLineNoBreak ) {
  istringstream strm("1\t2 3");
  MapperSource in(strm);
  BOOST_CHECK_EQUAL(in.Done(), false);
  vector<WordId> v, w;
  in.Read(&v, &w);
  BOOST_CHECK_EQUAL(v.size(), 1);
  BOOST_CHECK_EQUAL(v[0], 1);
  BOOST_CHECK_EQUAL(w.size(), 2);
  BOOST_CHECK_EQUAL(w[0], 2);
  BOOST_CHECK_EQUAL(w[1], 3);
  in.Next();
  BOOST_CHECK_EQUAL(in.Done(), true);
}

BOOST_AUTO_TEST_CASE( MapperSourceMultiLine ) {
  istringstream strm(
      "1\t2\n"
      "2\t3\n"
      "4\t5\n");
  MapperSource in(strm);
  int read = 0;
  for (; !in.Done(); in.Next())
    ++read;
  BOOST_CHECK_EQUAL(read, 3);
}

BOOST_AUTO_TEST_CASE( ReducerSourceEmptyFile ) {
  istringstream strm("");
  ReducerSource in(strm);
  BOOST_CHECK_EQUAL(in.Done(), true);
}

BOOST_AUTO_TEST_CASE( ReducerSourceSingleLineNoBreak ) {
  istringstream strm("1\t2 3");
  ReducerSource in(strm);
  BOOST_CHECK_EQUAL(in.Done(), false);
  BOOST_CHECK_EQUAL(in.Key(), 1);
  BOOST_CHECK_EQUAL(in.Value(), string("2 3"));
  in.Next();
  BOOST_CHECK_EQUAL(in.Done(), true);
}

BOOST_AUTO_TEST_CASE( ReducerSourceMultiLine ) {
  istringstream strm(
      "1\t\n"
      "2\thaha\n"
      "-2\thoho\n");
  ReducerSource in(strm);
  // First
  BOOST_CHECK_EQUAL(in.Done(), false);
  BOOST_CHECK_EQUAL(in.Key(), 1);
  BOOST_CHECK_EQUAL(in.Value(), string(""));
  // Second
  in.Next();
  BOOST_CHECK_EQUAL(in.Done(), false);
  BOOST_CHECK_EQUAL(in.Key(), 2);
  BOOST_CHECK_EQUAL(in.Value(), string("haha"));
  // Third
  in.Next();
  BOOST_CHECK_EQUAL(in.Done(), false);
  BOOST_CHECK_EQUAL(in.Key(), -2);
  BOOST_CHECK_EQUAL(in.Value(), string("hoho"));
  // End
  in.Next();
  BOOST_CHECK_EQUAL(in.Done(), true);
}

BOOST_AUTO_TEST_CASE( ReducerSourceGrouping ) {
  istringstream strm(
      "1\t1\n"
      "1\t1\n"
      "1\t1\n"
      "3\t3\n"
      "2\t2\n"
      "2\t2\n"
      "1\t1\n"
      "1\t1\n"
      "1\t1\n"
      "3\t3\n"
      "2\t2\n"
      "2\t2\n");
  ReducerSource in(strm);
  for (int i = 0; i < 2; ++i) {
    // key is 1
    {
      WordId key = in.Key();
      BOOST_CHECK_EQUAL(key, 1);
      int read = 0;
      for (; !in.Done() && in.Key() == key; in.Next()) {
        BOOST_CHECK_EQUAL(in.Key(), 1);
        BOOST_CHECK_EQUAL(in.Value(), string("1"));
        ++read;
      }
      BOOST_CHECK_EQUAL(read, 3);
    }
    // key is 3
    {
      WordId key = in.Key();
      BOOST_CHECK_EQUAL(key, 3);
      int read = 0;
      for (; !in.Done() && in.Key() == key; in.Next()) {
        BOOST_CHECK_EQUAL(in.Key(), 3);
        BOOST_CHECK_EQUAL(in.Value(), string("3"));
        ++read;
      }
      BOOST_CHECK_EQUAL(read, 1);
    }
    // key is 2
    {
      WordId key = in.Key();
      BOOST_CHECK_EQUAL(key, 2);
      int read = 0;
      for (; !in.Done() && in.Key() == key; in.Next()) {
        BOOST_CHECK_EQUAL(in.Key(), 2);
        BOOST_CHECK_EQUAL(in.Value(), string("2"));
        ++read;
      }
      BOOST_CHECK_EQUAL(read, 2);
    }
  }
}
