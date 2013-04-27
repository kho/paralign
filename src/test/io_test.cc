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
  istringstream strm("0\t1\t2 3");
  MapperSource in(strm);
  BOOST_CHECK_EQUAL(in.Done(), false);
  size_t id = 1;
  vector<WordId> v, w;
  in.Read(&id, &v, &w);
  BOOST_CHECK_EQUAL(id, 0);
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
      "8\t1\t2\n"
      "9\t2\t3\n"
      "10\t4\t5\n");
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

BOOST_AUTO_TEST_CASE( SinkWriteDouble ) {
  ostringstream os;
  Sink out(os);
  out.WriteToks(0.25);
  out.WriteEmpFeat(0.5);
  out.WriteLogLikelihood(-0.125);

  istringstream is(os.str());
  ReducerSource in(is);
  double v;
  // toks 0.25
  BOOST_REQUIRE(!in.Done());
  BOOST_CHECK_EQUAL(in.Key(), kToksKey);
  in.Read(&v);
  BOOST_CHECK_EQUAL(v, 0.25);
  // emp_feat 0.5
  in.Next();
  BOOST_REQUIRE(!in.Done());
  BOOST_CHECK_EQUAL(in.Key(), kEmpFeatKey);
  in.Read(&v);
  BOOST_CHECK_EQUAL(v, 0.5);
  // log_likelihood -0.125
  in.Next();
  BOOST_REQUIRE(!in.Done());
  BOOST_CHECK_EQUAL(in.Key(), kLogLikelihoodKey);
  in.Read(&v);
  BOOST_CHECK_EQUAL(v, -0.125);
  // end
  in.Next();
  BOOST_REQUIRE(in.Done());
}

BOOST_AUTO_TEST_CASE( ViterbiSinkWrite ) {
  ostringstream os;
  ViterbiSink out(os);
  size_t id;
  vector<SentSzPair> al;

  id = 1;
  out.WriteAlignment(id, al.begin(), al.end());

  id = 2;
  al.push_back(MkSzPair(0, 0));
  out.WriteAlignment(id, al.begin(), al.end());

  id = 4;
  al.push_back(MkSzPair(1, 2));
  out.WriteAlignment(id, al.begin(), al.end());

  BOOST_CHECK_EQUAL(os.str(),
                    "1\t\n"
                    "2\t0-0\n"
                    "4\t0-0 1-2\n");
}
