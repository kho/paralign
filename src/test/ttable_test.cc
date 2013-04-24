#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE ttable_test
#include <boost/test/unit_test.hpp>

#include <map>
#include <string>
#include <sstream>

#include "ttable.h"

using namespace std;
using namespace paralign;

BOOST_AUTO_TEST_CASE( KVSize ) {
  BOOST_CHECK_EQUAL(sizeof(KV<WordId, double>), sizeof(WordId) + sizeof(double));
  BOOST_CHECK_EQUAL(sizeof(KV<char, int>), sizeof(char) + sizeof(int));
}

BOOST_AUTO_TEST_CASE( KVBinarySearch ) {
  typedef KV<int, int> P;
  P arr[] = { P(0, 0), P(2, 2), P(3, -3), P(3, 3), P(4, 4) };
  size_t num = sizeof(arr) / sizeof(P);
  P *ret = NULL;

  // Empty
  ret = LookUp(0, arr, 0);
  BOOST_REQUIRE(ret == NULL);
  ret = LookUp(1, arr, 0);
  BOOST_REQUIRE(ret == NULL);

  // Non-empty
  ret = LookUp(0, arr, num);
  BOOST_REQUIRE(ret != NULL);
  BOOST_CHECK_EQUAL(ret->k, 0);
  BOOST_CHECK_EQUAL(ret->v, 0);

  ret = LookUp(1, arr, num);
  BOOST_REQUIRE(ret == NULL);

  ret = LookUp(2, arr, num);
  BOOST_REQUIRE(ret != NULL);
  BOOST_CHECK_EQUAL(ret->k, 2);
  BOOST_CHECK_EQUAL(ret->v, 2);

  ret = LookUp(3, arr, num);
  BOOST_REQUIRE(ret != NULL);
  BOOST_CHECK_EQUAL(ret->k, 3);
  BOOST_CHECK_EQUAL(ret->v, 3);

  ret = LookUp(4, arr, num);
  BOOST_REQUIRE(ret != NULL);
  BOOST_CHECK_EQUAL(ret->k, 4);
  BOOST_CHECK_EQUAL(ret->v, 4);

  ret = LookUp(5, arr, num);
  BOOST_REQUIRE(ret == NULL);
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
