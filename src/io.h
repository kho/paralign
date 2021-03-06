#ifndef _PARALIGN_IO_H_
#define _PARALIGN_IO_H_

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <boost/lexical_cast.hpp>

#include "ttable.h"
#include "types.h"
#include "contrib/log.h"

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
  explicit MapperSource(std::istream &input) : in_(input), done_(false), counter_(0) {
    Next();
  }

  ~MapperSource() {
    LOG(INFO) << "MapperSource[" << std::hex << this << "] " << std::dec << counter_ << " reads";
  }

  bool Done() const {
    return done_;
  }

  void Read(size_t *id, std::vector<WordId> *src, std::vector<WordId> *tgt) const {
    std::string::size_type sep0 = buf_.find('\t');
    if (sep0 == std::string::npos)
      LOG(FATAL) << "Invalid input line: " << buf_;
    *id = boost::lexical_cast<size_t>(buf_.substr(0, sep0));
    std::string::size_type sep1 = buf_.find('\t', sep0 + 1);
    if (sep1 == std::string::npos)
      LOG(FATAL) << "Invalid input line: " << buf_;
    {
      std::istringstream strm(buf_.substr(sep0 + 1, sep1 - sep0 - 1));
      PushWords(strm, src);
    }
    {
      std::istringstream strm(buf_.substr(sep1 + 1));
      PushWords(strm, tgt);
    }
    ++counter_;
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
    if (strm.fail() && !strm.eof())
      LOG(FATAL) << "Failed to read input words! Are they integers?";
  }

  std::istream &in_;
  bool done_;
  std::string buf_;
  mutable size_t counter_;
};

class ReducerSource {
 public:
  explicit ReducerSource(std::istream &input) : in_(input), done_(false), counter_(0) {
    Next();
  }

  ~ReducerSource() {
    LOG(INFO) << "ReducerSource[" << std::hex << this << "] " << std::dec << counter_ << " reads";
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
    std::istringstream strm(buf_);
    strm >> *entry;
    ++counter_;
  }

  void Read(double *dest) const {
    *dest = DoubleFromInt64(boost::lexical_cast<int64_t>(buf_));
    ++counter_;
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
  mutable size_t counter_;
};

// Writes out key-value pairs for various purposes in textual
// format. Currently this can be shared between mappers and reducers.
class Sink {
 public:
  explicit Sink(std::ostream &output) : out_(output), counter_(0) {}

  ~Sink() {
    LOG(INFO) << "Sink[" << std::hex << this << "] " << std::dec << counter_ << " writes";
  }

  void WriteTTableEntry(WordId src, const TTableEntry &entry) {
    out_ << src << '\t' << entry << '\n';
    ++counter_;
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
    ++counter_;
  }

  void WriteToks(double toks) {
    out_ << kToksKey << '\t' << DoubleAsInt64(toks) << '\n';
    ++counter_;
  }

  void WriteEmpFeat(double emp_feat) {
    out_ << kEmpFeatKey << '\t' << DoubleAsInt64(emp_feat) << '\n';
    ++counter_;
  }

  void WriteLogLikelihood(double log_likelihood) {
    out_ << kLogLikelihoodKey << '\t' << DoubleAsInt64(log_likelihood) << '\n';
    ++counter_;
  }

  void WriteTension(double tension) {
    out_ << tension << '\n';
    ++counter_;
  }

 private:
  std::ostream &out_;
  mutable size_t counter_;
};

typedef Sink MapperSink;
typedef Sink ReducerSink;

// For writing out alignment points
class ViterbiSink {
 public:
  explicit ViterbiSink(std::ostream &output) : out_(output), counter_(0) {}

  ~ViterbiSink() {
    LOG(INFO) << "ViterbiSink[" << std::hex << this << "] " << std::dec << counter_ << " writes";
  }

  template <class It>
  void WriteAlignment(size_t id, It begin, It end) {
    out_ << id << '\t';
    bool first = true;
    while (begin != end) {
      if (first)
        first = false;
      else
        out_ << ' ';
      out_ << FirstSz(*begin) << '-' << SecondSz(*begin);
      ++begin;
    }
    out_ << '\n';
    ++counter_;
  }

 private:
  std::ostream &out_;
  size_t counter_;
};
} // paralign

#endif  // _PARALIGN_IO_H_
