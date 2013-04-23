#include <map>

#include "io.h"
#include "options.h"
#include "ttable.h"
#include "types.h"
#include "contrib/log.h"

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
        size_counts_(), toks_(0), emp_feat_(0), log_likelihood_(0) {}

  void Run() {
    while (!in_->Done()) {
      WordId key = in_->Key();
      if (key >= 0)
        ReduceTTableEntry(key);
      else if (key == kSizeCountsKey)
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
      swap(src, dst);
    }
    // `entry_[src]` now holds the sum
    TTableEntry &result = entry_[src];
    if (opts_.variational_bayes)
      result.NormalizeVB(opts_.alpha);
    else
      result.Normalize();
    tbl_writer_->Write(key, result);
    entry_[0].Clear();
    entry_[1].Clear();
    entry_[2].Clear();
  }

  void ReduceSizeCounts() {
    for (; !in_->Done() && in_->Key() == kSizeCountsKey; in_->Next()) {
      istringstream strm(in_->Value());
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
    out_->WriteSizeCounts(size_counts_.begin(), size_counts_.end());
    out_->WriteToks(toks_);
    out_->WriteEmpFeat(emp_feat_);
    out_->WriteLogLikelihood(log_likelihood_);
  }

  const Options opts_;
  TTableWriter *tbl_writer_;
  ReducerSource *in_;
  ReducerSink *out_;

  TTableEntry entry_[3];
  map<SentSzPair, int> size_counts_;
  double toks_;
  double emp_feat_;
  double log_likelihood_;
};
} // namespace paralign

using namespace paralign;

int main() {
  Options opts = Options::FromEnv();
  TTableWriter writer(opts.ttable_prefix);
  ReducerSource input(cin);
  ReducerSink output(cout);

  Reducer(opts, &writer, &input, &output).Run();

  return 0;
}
