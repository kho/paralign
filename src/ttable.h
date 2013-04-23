#ifndef _PARALIGN_TTABLE_H_
#define _PARALIGN_TTABLE_H_

#include <iosfwd>
#include <map>
#include <string>

#include "types.h"

namespace paralign {
const WordId kNull = 0;

// A table entry
class TTableEntry {
 public:
  explicit TTableEntry(const std::map<WordId, double> &);
  TTableEntry();
  void NormalizeVB(double alpha);
  void Normalize();
  void Clear();

  friend std::ostream &operator<<(std::ostream &, const TTableEntry &);
  friend std::istream &operator>>(std::istream &, TTableEntry &);
};

inline std::ostream &operator<<(std::ostream &, const TTableEntry &);
inline std::istream &operator>>(std::istream &, TTableEntry &);

inline void PlusEq(const TTableEntry &x, const TTableEntry &y, TTableEntry *z) {
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
