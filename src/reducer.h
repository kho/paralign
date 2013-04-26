#ifndef _PARALIGN_REDUCER_H_
#define _PARALIGN_REDUCER_H_

#include <cmath>
#include <map>

#include "io.h"
#include "options.h"
#include "ttable.h"
#include "types.h"
#include "contrib/da.h"
#include "contrib/log.h"

namespace paralign {
// Each reducer constructs normalized translation table and output the following
// 1. Sum of size_counts
// 2. Sum of emp_feat
// 3. Sum of toks
// 4. Sum of log-likelihood
// These values will be used to compute the update for `diagonal_tension`
class Reducer {
 public:
  enum Mode {
    kReducer,
    kCombiner,
    kTension,
  };

  Reducer(const Options &opts, TTableWriter *writer, ReducerSource *input, ReducerSink *output, Mode mode)
      : opts_(opts), tbl_writer_(writer), in_(input), out_(output),
        size_counts_(), toks_(0), emp_feat_(0), log_likelihood_(0), mode_(mode) {
    if (mode == kReducer) {
    } else if (mode == kCombiner || mode == kTension) {
      if (writer)
        LOG(FATAL) << "Running in combiner mode but given a TTableWriter!";
    } else {
      LOG(FATAL) << "Unknown mode: " << mode;
    }
  }

  void Run() {
    while (!in_->Done()) {
      WordId key = in_->Key();
      if (key >= 0) {
        if (mode_ == kReducer || mode_ == kCombiner)
          ReduceTTableEntry(key);
        else
          LOG(FATAL) << "Got ttable entry but not under reducer or combiner mode";
      } else if (key == kSizeCountsKey)
        ReduceSizeCounts();
      else if (key == kEmpFeatKey)
        ReduceDoubleValue(key, &emp_feat_);
      else if (key == kToksKey)
        ReduceDoubleValue(key, &toks_);
      else if (key == kLogLikelihoodKey)
        ReduceDoubleValue(key, &log_likelihood_);
      else
        LOG(FATAL) << "Unrecognized key type: " << key;
    }
    Flush();
  }

 private:
  void ReduceTTableEntry(WordId key) {
    // before this call, `entry_[0]` and `entry_[1]` should be both empty
    int src = 0, dst = 1, val = 2;
    for (; !in_->Done() && in_->Key() == key; in_->Next()) {
      in_->Read(&entry_[val]);
      PlusEq(entry_[val], entry_[src], &entry_[dst]);
      std::swap(src, dst);
    }
    // `entry_[src]` now holds the sum
    TTableEntry &result = entry_[src];
    if (mode_ == kReducer) {
      if (opts_.variational_bayes)
        result.NormalizeVB(opts_.alpha);
      else
        result.Normalize();
      tbl_writer_->Write(key, result);
    } else if (mode_ == kCombiner) {
      out_->WriteTTableEntry(key, result);
    } else {
      LOG(FATAL) << "How did you get here if you are not a reducer or a combiner?";
    }
    entry_[0].Clear();
    entry_[1].Clear();
    entry_[2].Clear();
  }

  void ReduceSizeCounts() {
    for (; !in_->Done() && in_->Key() == kSizeCountsKey; in_->Next()) {
      std::istringstream strm(in_->Value());
      SentSzPair p;
      int c;
      while (strm >> p >> c)
        size_counts_[p] += c;
    }
  }

  void ReduceDoubleValue(WordId key, double *dest) {
    double v;
    for (; !in_->Done() && in_->Key() == key; in_->Next()) {
      in_->Read(&v);
      *dest += v;
    }
  }

  void Flush() {
    if (mode_ == kReducer)
      tbl_writer_->WriteIndex();
    if (mode_ == kReducer || mode_ == kCombiner) {
      out_->WriteSizeCounts(size_counts_.begin(), size_counts_.end());
      out_->WriteToks(toks_);
      out_->WriteEmpFeat(emp_feat_);
      out_->WriteLogLikelihood(log_likelihood_);
    }
    if (mode_ == kTension) {
      emp_feat_ /= toks_;
      const double base2_log_likelihood = log_likelihood_ / std::log(2);
      LOG(INFO) << "  log_e likelihood: " << log_likelihood_;
      LOG(INFO) << "  log_2 likelihood: " << base2_log_likelihood;
      LOG(INFO) << "     cross entropy: " << base2_log_likelihood / toks_;
      LOG(INFO) << "        perplexity: " << std::pow(2.0, -base2_log_likelihood / toks_);
      LOG(INFO) << " posterior al-feat: " << emp_feat_;
      LOG(INFO) << "       size counts: " << size_counts_.size();
      if (opts_.favor_diagonal && opts_.optimize_tension) {
        double diagonal_tension = opts_.diagonal_tension;
        for (int ii = 0; ii < 8; ++ii) {
          double mod_feat = 0;
          std::map<SentSzPair, int>::const_iterator it = size_counts_.begin();
          for(; it != size_counts_.end(); ++it) {
            SentSzPair p = it->first;
            for (int j = 1; j <= FirstSz(p); ++j)
              mod_feat += it->second * DiagonalAlignment::ComputeDLogZ(j, FirstSz(p), SecondSz(p), diagonal_tension);
          }
          mod_feat /= toks_;
          LOG(INFO) << "  " << ii + 1 << "  model al-feat: " << mod_feat << " (tension=" << diagonal_tension << ")";
          diagonal_tension += (emp_feat_ - mod_feat) * 20.0;
          if (diagonal_tension <= 0.1) diagonal_tension = 0.1;
          if (diagonal_tension > 14) diagonal_tension = 14;
        }
        LOG(INFO) << "     final tension: " << diagonal_tension;
        out_->WriteTension(diagonal_tension);
      }
    }
  }

  const Options opts_;
  TTableWriter *tbl_writer_;
  ReducerSource *in_;
  ReducerSink *out_;

  TTableEntry entry_[3];
  std::map<SentSzPair, int> size_counts_;
  double toks_;
  double emp_feat_;
  double log_likelihood_;

  Mode mode_;
};
} // namespace paralign


#endif  // _PARALIGN_REDUCER_H_
