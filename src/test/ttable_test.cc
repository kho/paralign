#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE ttable_test
#include <boost/test/unit_test.hpp>

#include <map>
#include <string>
#include <sstream>
#include "ttable.h"

using namespace std;
using namespace paralign;

BOOST_AUTO_TEST_CASE( WordIdDoubleSize ) {
  BOOST_CHECK_EQUAL(sizeof(WordIdDouble), sizeof(WordId) + sizeof(double));
}

BOOST_AUTO_TEST_CASE( TTableEntryBasic ) {
  TTableEntry e;
  BOOST_REQUIRE(e.Empty());

  map<WordId, double> m;
  m[2] = 1; m[1] = 1; m[3] = 1; m[4] = 0;
  TTableEntry f(m);
  BOOST_REQUIRE(!f.Empty());
  BOOST_CHECK_EQUAL(f.Size(), 3);
  BOOST_REQUIRE(!(e == f));

  ostringstream os;
  os << e;

  istringstream is(os.str());
  is >> f;
  BOOST_REQUIRE(f.Empty());
  BOOST_CHECK_EQUAL(e, f);
}

BOOST_AUTO_TEST_CASE( TTableEntryNoramlize ) {
  map<WordId, double> m;
  m[1] = 1; m[3] = 1;
  TTableEntry e(m);
  e.Normalize();
  m[1] = 0.5; m[3] = 0.5;
  TTableEntry f(m);

  BOOST_CHECK_EQUAL(e, f);
}

BOOST_AUTO_TEST_CASE( TTableEntryNoramlizeVB ) {
  map<WordId, double> m;
  m[1] = 1; m[3] = 1;
  TTableEntry e(m);
  e.NormalizeVB(1.0);
  m[1] = 2; m[3] = 2;
  TTableEntry f(m);
  f.NormalizeVB(0);

  BOOST_CHECK_EQUAL(e, f);
}

BOOST_AUTO_TEST_CASE( TTableEntryPlusEqZero ) {
  map<WordId, double> m;
  m[1] = 1;
  TTableEntry e(m);
  TTableEntry f, g;

  PlusEq(e, f, &g);
  BOOST_CHECK_EQUAL(e, g);
  g.Clear();
  PlusEq(f, e, &g);
  BOOST_CHECK_EQUAL(e, g);
}

BOOST_AUTO_TEST_CASE( TTableEntryPlusEqNoOverlap ) {
  map<WordId, double> m;

  m[1] = 1; m[3] = 1;
  TTableEntry e(m);

  m.clear(); m[2] = 1;
  TTableEntry f(m);

  m[1] = 1; m[3] = 1;
  TTableEntry g(m);

  TTableEntry h;
  PlusEq(e, f, &h);
  BOOST_CHECK_EQUAL(g, h);
  h.Clear();
  PlusEq(f, e, &h);
  BOOST_CHECK_EQUAL(g, h);
}

BOOST_AUTO_TEST_CASE( TTableEntryPlusEqWithOverlap ) {
  map<WordId, double> m;

  m[1] = 1; m[2] = 1; m[3] = 1;
  TTableEntry e(m);

  m.clear(); m[2] = 1;
  TTableEntry f(m);

  m.clear(); m[1] = 1; m[2] = 2; m[3] = 1;
  TTableEntry g(m);

  TTableEntry h;
  PlusEq(e, f, &h);
  BOOST_CHECK_EQUAL(g, h);
  PlusEq(f, e, &h);
  BOOST_CHECK_EQUAL(g, h);
}
