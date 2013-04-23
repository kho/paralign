#ifndef _PARALIGN_IO_H_
#define _PARALIGN_IO_H_

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <boost/lexical_cast.hpp>

#include "log.h"
#include "ttable.h"
#include "types.h"

namespace paralign {
// Constants for keys
const WordId kNoKey = -1;
const WordId kSizeCountsKey = -2;
const WordId kEmpFeatKey = -3;
const WordId kToksKey = -4;
const WordId kLogLikelihoodKey = -5;

// Reads from an input stream; the input records of the mapper is just
// lines, where each line is a tab-delimited integerized sentence
// pair.
class MapperSource {
 public:
  explicit MapperSource(std::istream &input) : in_(input), done_(false) {
    Next();
  }

  bool Done() const {
    return done_;
  }

  void Read(std::vector<WordId> *src, std::vector<WordId> *tgt) const {
    std::string::size_type sep = buf_.find('\t');
    if (sep == std::string::npos)
      LOG(FATAL) << "Invalid input line: " << buf_;
    {
      std::istringstream strm(buf_.substr(0, sep));
      PushWords(strm, src);
    }
    {
      std::istringstream strm(buf_.substr(sep + 1));
      PushWords(strm, tgt);
    }
  }

  void Next() {
    if (done_)
      LOG(FATAL) << "Iterator has reached the end";
    done_ = !getline(in_, buf_);
  }

 private:
  void PushWords(std::istringstream &strm, std::vector<WordId> *out) const {
    WordId tok;
    out->clear();
    while (strm >> tok)
      out->push_back(tok);
  }

  std::istream &in_;
  bool done_;
  std::string buf_;
};

class ReducerSource {
 public:
  explicit ReducerSource(std::istream &input) : in_(input), done_(false) {
    Next();
  }

  bool Done() const {
    return done_;
  }

  WordId Key() const {
    return cur_key_;
  }

  const string &Value() const {
    return buf_;
  }

  void Read(TTableEntry *entry) const {
  }

  void Read(double *dest) const {
    union { int64_t i; double d; } v;
    v.i = boost::lexical_cast<int64_t>(buf_);
    *dest = v.d;
  }

  void Next() {
    if (done_)
      LOG(FATAL) << "Iterator has reached the end";
    done_ = !getline(in_, buf_);
    if (!done_)
      SetKeyValue();
  }

 private:
  void SetKeyValue() {
    std::string::size_type sep = buf_.find('\t');
    if (sep == std::string::npos)
      LOG(FATAL) << "Invalid input line: " << buf_;
    cur_key_ = boost::lexical_cast<WordId>(buf_.substr(0, sep));
    buf_ = buf_.substr(sep + 1);
  }

  std::istream &in_;
  bool done_;
  WordId cur_key_;
  std::string buf_;
};

// Writes out key-value pairs for various purposes in textual
// format. Currently this can be shared between mappers and reducers.
class Sink {
 public:
  explicit Sink(std::ostream &output) : out_(output) {}

  void WriteTTableEntry(WordId src, const TTableEntry &entry) {
    out_ << src << '\t' << entry << '\n';
  }

  template <class It>
  void WriteSizeCounts(It begin, It end) {
    out_ << kSizeCountsKey << '\t';
    bool first = true;
    while (begin != end) {
      if (first)
        first = false;
      else
        out_ << ' ';
      out_ << begin->first << ' ' << begin->second;
      ++begin;
    }
    out_ << '\n';
  }

  void WriteToks(double toks) {
    out_ << kToksKey << '\t' << DoubleAsInt64(toks) << '\n';
  }

  void WriteEmpFeat(double emp_feat) {
    out_ << kEmpFeatKey << '\t' << DoubleAsInt64(emp_feat) << '\n';
  }

  void WriteLogLikelihood(double log_likelihood) {
    out_ << kLogLikelihoodKey << '\t' << DoubleAsInt64(log_likelihood) << '\n';
  }

 private:
  int64_t DoubleAsInt64(double v) {
    union { int64_t i; double d; } w;
    w.d = v;
    return w.i;
  }

  std::ostream &out_;
};

typedef Sink MapperSink;
typedef Sink ReducerSink;



} // paralign

#endif  // _PARALIGN_IO_H_
