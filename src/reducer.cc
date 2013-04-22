#include <map>

#include "io.h"
#include "log.h"
#include "options.h"
#include "ttable.h"
#include "types.h"

using namespace std;

namespace paralign {
// Each reducer constructs normalized translation table and output the following
// 1. Sum of size_counts
// 2. Sum of emp_feat
// 3. Sum of toks
// 4. Sum of log-likelihood
// These values will be used to compute the update for `diagonal_tension`
class Reducer {
 public:
  Reducer(const Options &opts, TTableWriter *writer, ReducerSource *input, ReducerSink *output)
      : opts_(opts), tbl_writer_(writer), in_(input), out_(output),
        entry_(), size_counts_(), toks_(0), emp_feat_(0), log_likelihood_(0) {}

  void Run() {
    for (; !in_->Done(); in_->Next()) {
      WordId key = in_->Key();
      if (key >= 0)
        ReduceTTableEntry(key);
      else if (key == kSizeCountsKey)
        ReduceSizeCounts();
      else if (key == kEmpFeatKey)
        ReduceDoubleValue(&emp_feat_);
      else if (key == kToksKey)
        ReduceDoubleValue(&toks_);
      else if (key == kLogLikelihoodKey)
        ReduceDoubleValue(&log_likelihood_);
      else
        LOG(FATAL) << "Unrecognized key type: " << key;
    }
    Flush();
  }

 private:
  void ReduceTTableEntry(WordId key) {
    // `entry_` should be empty before this call
    ReducerSource::TTableEntryIter it(*in_);
    for (; !it.Done(); it.Next()) {
      entry_.PlusEq(it.Value());
    }
    if (opts_.variational_bayes)
      entry_.NormalizeVB(opts_.alpha);
    else
      entry_.Normalize();
    tbl_writer_->Write(key, entry_);
    entry_.Clear();
  }

  void ReduceSizeCounts() {
  }

  void ReduceDoubleValue(double *dest) {
    ReducerSource::DoubleValueIter it(*in_);
    for (; !it.Done(); it.Next())
      *dest += it.Value();
  }

  void Flush() {
    out_->WriteSizeCounts(size_counts_.begin(), size_counts_.end());
    out_->WriteToks(toks_);
    out_->WriteEmpFeat(emp_feat_);
    out_->WriteLogLikelihood(log_likelihood_);
  }

  const Options opts_;
  TTableWriter *tbl_writer_;
  ReducerSource *in_;
  ReducerSink *out_;

  TTableEntry entry_;
  map<SentSzPair, int> size_counts_;
  double toks_;
  double emp_feat_;
  double log_likelihood_;
};
} // namespace paralign
