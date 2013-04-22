#ifndef _PARALIGN_IO_H_
#define _PARALIGN_IO_H_

#include <vector>

#include "ttable.h"
#include "types.h"

namespace paralign {

const WordId kSizeCountsKey = -1;
const WordId kEmpFeatKey = -2;
const WordId kToksKey = -3;
const WordId kLogLikelihoodKey = -4;

class MapperSource {
 public:
  bool Done() const;
  void Read(std::vector<WordId> *src, std::vector<WordId> *tgt) const;
  void Next();
};

class ReducerSource {
 public:
  class TTableEntryIter {
   public:
    explicit TTableEntryIter(ReducerSource &);
    bool Done() const;
    const TTableEntry &Value() const;
    void Next();
  };
  class SizeCountsIter {
   public:
    explicit SizeCountsIter(ReducerSource &);
    bool Done() const;
    std::pair<SentSzPair, int> Value() const;
    void Next();
  };
  class DoubleValueIter {
   public:
    explicit DoubleValueIter(ReducerSource &);
    bool Done() const;
    double Value() const;
    void Next();
  };
  bool Done() const;
  WordId Key() const;
  void Next();
};

class Sink {
 public:
  void WriteTTableEntry(WordId src, const TTableEntry &entry);
  template <class It>
  void WriteSizeCounts(It begin, It end);
  void WriteToks(double toks);
  void WriteEmpFeat(double emp_feat);
  void WriteLogLikelihood(double log_likelihood);
};

typedef Sink MapperSink;
typedef Sink ReducerSink;
} // paralign

#endif  // _PARALIGN_IO_H_
