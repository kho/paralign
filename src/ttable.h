#ifndef _PARALIGN_TTABLE_H_
#define _PARALIGN_TTABLE_H_

#include <map>

#include "types.h"

namespace paralign {
const WordId kNull = 0;

// A table entry
class TTableEntry {
 public:
  explicit TTableEntry(const std::map<WordId, double> &);
  TTableEntry();
  void PlusEq(const TTableEntry &that);
  void NormalizeVB(double alpha);
  void Normalize();
  void Clear();
};

// Distributed translation table
class TTable {
 public:
  double Query(WordId src, WordId tgt) const;
};

// A single piece of the translation table
class PartialTTable {
 public:
};

// Writer to a single piece of the distributed translation table
class TTableWriter {
 public:
  void Write(WordId src, const TTableEntry &entry);
};
}// namespace paralign

#endif  // _PARALIGN_TTABLE_H_
