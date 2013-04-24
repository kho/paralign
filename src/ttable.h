#ifndef _PARALIGN_TTABLE_H_
#define _PARALIGN_TTABLE_H_

#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <vector>
#include <boost/foreach.hpp>
#include <boost/math/special_functions/digamma.hpp>

#include "types.h"
#include "contrib/log.h"

namespace paralign {
const WordId kNull = 0;

#pragma pack(push, 1)
struct WordIdDouble {
  WordIdDouble(WordId ww = 0, double dd = 0) : w(ww), d(dd) {}

  bool operator==(const WordIdDouble &that) const {
    return w == that.w && d == that.d;
  }

  WordId w;
  double d;
};
#pragma pack(pop)

// A table entry, all items are sorted by word id.
class TTableEntry {
 public:
  // Empty entry
  TTableEntry() : items_() {}

  TTableEntry(const TTableEntry &that) : items_(that.items_) {}

  // Converts a (ordered) map to an entry
  explicit TTableEntry(const std::map<WordId, double> &m) {
    items_.reserve(m.size());
    typedef std::pair<WordId, double> P;
    BOOST_FOREACH(const P &p, m) {
      if (p.second != 0)
        items_.push_back(WordIdDouble(p.first, p.second));
    }
  }

  bool Empty() const {
    return items_.empty();
  }

  size_t Size() const {
    return items_.size();
  }

  bool operator==(const TTableEntry &that) const {
    return items_ == that.items_;
  }

  // Treats items as holding probability and normalizes them to sum to
  // one. Does nothing when the entry is empty. When not empty, issues
  // a warning when dividing by zero.
  void Normalize() {
    if (Empty()) return;
    double sum = 0;
    BOOST_FOREACH(const WordIdDouble &i, items_) {
      sum += i.d;
    }
    if (sum == 0)
      LOG(WARNING) << "Division by zero in TTableEntry::Normalize";
    BOOST_FOREACH(WordIdDouble &i, items_) {
      i.d /= sum;
    }
  }

  // Treats items as holding probability and normalizes using
  // mean-field approximation. Does nothing when the entry is empty.
  void NormalizeVB(double alpha) {
    if (Empty()) return;
    double sum = alpha * items_.size();
    BOOST_FOREACH(const WordIdDouble &i, items_) {
      sum += i.d;
    }
    BOOST_FOREACH(WordIdDouble &i, items_) {
      i.d = exp(boost::math::digamma(i.d + alpha) - boost::math::digamma(sum));
    }
  }

  // Clears items;
  void Clear() {
    items_.clear();
  }

  friend std::ostream &operator<<(std::ostream &, const TTableEntry &);
  friend std::istream &operator>>(std::istream &, TTableEntry &);
  friend void PlusEq(const TTableEntry &, const TTableEntry &, TTableEntry *);

 private:
  std::vector<WordIdDouble> items_;
};

// Writes # of items, followed by tab-separated `WordIdDouble` pairs,
// without any line breaks in between.
inline std::ostream &operator<<(std::ostream &out, const TTableEntry &e) {
  out << e.items_.size();
  BOOST_FOREACH(const WordIdDouble &i, e.items_) {
    out << ' ' << i.w << ' ' << DoubleAsInt64(i.d);
  }
  return out;
}

// Reads what's written by `operator<<`, items must be ordered by word
// id.
inline std::istream &operator>>(std::istream &in, TTableEntry &e) {
  size_t n;
  in >> n;
  e.items_.resize(n);
  for (size_t i = 0; i < n; ++i) {
    in >> e.items_[i].w;
    int64_t v;
    in >> v;
    e.items_[i].d = DoubleFromInt64(v);
  }
  return in;
}

inline void PlusEq(const TTableEntry &x, const TTableEntry &y, TTableEntry *z) {
  z->items_.clear();
  size_t xi = 0, yi = 0;
  while (xi < x.items_.size() && yi < y.items_.size()) {
    const WordIdDouble &xp = x.items_[xi], &yp = y.items_[yi];
    if (xp.w < yp.w) {
      z->items_.push_back(xp);
      ++xi;
    } else if (yp.w < xp.w) {
      z->items_.push_back(yp);
      ++yi;
    } else {
      double d = xp.d + yp.d;
      if (d != 0)
        z->items_.push_back(WordIdDouble(xp.w, d));
      ++xi;
      ++yi;
    }
  }
  if (xi == x.items_.size()) {
    while (yi < y.items_.size()) {
      z->items_.push_back(y.items_[yi]);
      ++yi;
    }
  } else {
    while (xi < x.items_.size()) {
      z->items_.push_back(x.items_[xi]);
      ++xi;
    }
  }
}

// Distributed translation table
class TTable {
 public:
  TTable(const std::string &prefix, int parts);
  double Query(WordId src, WordId tgt) const;
};

// A single piece of the translation table
class PartialTTable {
 public:
};

// Writer to a single piece of the distributed translation table
class TTableWriter {
 public:
  TTableWriter(const std::string &prefix, bool local);
  void Write(WordId src, const TTableEntry &entry);
};
}// namespace paralign

#endif  // _PARALIGN_TTABLE_H_
