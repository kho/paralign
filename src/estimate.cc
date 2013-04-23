#include <cstring>
#include <map>
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <boost/foreach.hpp>

using namespace std;

const int kIntSize = sizeof(int), kLongLongSize = sizeof(long long), kDoubleSize = sizeof(double);

void AddTokens(const string &sentence, vector<int> *output) {
  istringstream strm(sentence);
  int w;
  while (strm >> w) output->push_back(w);
}

void Tokenize(const string &buf, bool reverse, vector<int> *src, vector<int> *tgt) {
  if (reverse) swap(src, tgt);
  src->clear();
  tgt->clear();
  size_t tab = buf.find('\t');
  AddTokens(buf.substr(0, tab), src);
  AddTokens(buf.substr(tab + 1), tgt);
}

int main(int argc, char *argv[]) {
  bool reverse = false;
  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "--reverse") == 0) {
      cerr << "reversing src and tgt order" << endl;
      reverse = true;
      break;
    }
  }

  cerr << "sizeof(int) = " << kIntSize << endl
       << "sizeof(long long) = " << kLongLongSize << endl
       << "sizeof(double) = " << kDoubleSize << endl;

  map<int, set<int> > src_stripes;
  set<int> tgt_vocab;

  vector<int> src, tgt;
  string buf;
  while (getline(cin, buf)) {
    Tokenize(buf, reverse, &src, &tgt);
    BOOST_FOREACH(int w, src) {
      BOOST_FOREACH(int v, tgt) {
        src_stripes[w].insert(v);
        tgt_vocab.insert(v);
      }
    }
  }

  size_t m = src_stripes.size();
  size_t n = tgt_vocab.size();
  long long bytes = kIntSize +       // count of index records
      (m + 1) * (kIntSize + kLongLongSize + kLongLongSize) + // index size
      n * (kIntSize + kDoubleSize);
  size_t l = 0;

  typedef pair<int, set<int> > Pair;
  BOOST_FOREACH(const Pair &p, src_stripes) {
    bytes += p.second.size() * (kIntSize + kDoubleSize);
    l += p.second.size();
  }

  double mb = bytes / (1024.0 * 1024.0);
  double gb = bytes / (1024.0 * 1024.0 * 1024.0);

  cout << "src vocab size: " << m << endl
       << "tgt vocab size: " << n << endl
       << "non-null pairs: " << l << endl
       << "sparse: " << bytes << " bytes = " << setprecision(2) << mb << " MB = " << gb << " GB" << endl;

  return 0;
}
