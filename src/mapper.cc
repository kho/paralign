#include <iostream>
#include <map>
#include <utility>
#include <vector>
#include <boost/foreach.hpp>

#include "io.h"
#include "options.h"
#include "ttable.h"
#include "types.h"
#include "contrib/da.h"

using namespace std;

namespace paralign {
// For each sentence, the mapper collects the following statistics:
// 1. size_counts (for computing the gradient of diagonal_tension), key: kSizeCountsKey
// 2. emp_feat, key: kEmpFeatKey
// 3. toks, key: kToksKey (denom is just toks)
// 4. pseudo_count, key: src word id
// 5. log-likelihood, key: kLogLikelihoodKey
class Mapper {
 public:
  Mapper(const Options &opts, const TTable &table, MapperSource *input, MapperSink *output)
      : opts_(opts), tbl_(table), in_(input), out_(output), pseudo_counts_(),
        size_counts_(), toks_(0), emp_feat_(0), log_likelihood_(0) {}

  void Run() {
    vector<WordId> src, tgt;
    for (; !in_->Done(); in_->Next()) {
      in_->Read(&src, &tgt);
      if (opts_.reverse) swap(src, tgt);
      Map(src, tgt);
    }
    Flush();
  }

 private:
  void Map(const vector<WordId> &src, const vector<WordId> &tgt) {
    toks_ += tgt.size();
    ++size_counts_[MkSzPair(tgt.size(), src.size())];

    probs_.resize(src.size() + 1);

    for (size_t j = 0; j < tgt.size(); ++j) {
      const WordId f_j = tgt[j];
      double sum = 0;
      double prob_a_i = 1.0 / (src.size() + !opts_.no_null_word);  // uniform (model 1)
      if (!opts_.no_null_word) {
        if (opts_.favor_diagonal) prob_a_i = opts_.prob_align_null;
        probs_[0] = tbl_.Query(kNull, f_j) * prob_a_i;
        sum += probs_[0];
      }
      double az = 0;
      if (opts_.favor_diagonal)
        az = DiagonalAlignment::ComputeZ(j+1, tgt.size(), src.size(), opts_.diagonal_tension) / (1 - opts_.prob_align_null);
      for (unsigned i = 1; i <= src.size(); ++i) {
        if (opts_.favor_diagonal)
          prob_a_i = DiagonalAlignment::UnnormalizedProb(j + 1, i, tgt.size(), src.size(), opts_.diagonal_tension) / az;
        probs_[i] = tbl_.Query(src[i-1], f_j) * prob_a_i;
        sum += probs_[i];
      }
      if (!opts_.no_null_word) {
        double count = probs_[0] / sum;
        pseudo_counts_[kNull][f_j] += count;
      }
      for (unsigned i = 1; i <= src.size(); ++i) {
        const double p = probs_[i] / sum;
        pseudo_counts_[src[i-1]][f_j] += p;
        emp_feat_ += DiagonalAlignment::Feature(j, i, tgt.size(), src.size()) * p;
      }
      log_likelihood_ += log(sum);
    }
    // We use in-mapper combining and only write at the end.

    // TODO: see how much memory this uses and set a buffering limit
    // for `pseudo_counts_`.
  }

  void Flush() {
    FlushPseudoCounts();
    out_->WriteSizeCounts(size_counts_.begin(), size_counts_.end());
    out_->WriteToks(toks_);
    out_->WriteEmpFeat(emp_feat_);
    out_->WriteLogLikelihood(log_likelihood_);
  }

  void FlushPseudoCounts() {
    typedef pair<WordId, map<WordId, double> > P;
    BOOST_FOREACH(const P &i, pseudo_counts_) {
      out_->WriteTTableEntry(i.first, TTableEntry(i.second));
    }
  }

  const Options opts_;
  const TTable &tbl_;
  MapperSource *in_;
  MapperSink *out_;

  map<WordId, map<WordId, double> > pseudo_counts_;
  map<SentSzPair, int> size_counts_;
  double toks_;
  double emp_feat_;
  double log_likelihood_;

  vector<double> probs_;
};
} // namespace paralign

using namespace paralign;

int main() {
  Options opts = Options::FromEnv();

  LOG(INFO) << "Options:" << endl
            << opts << endl;

  TTable table(opts.ttable_dir, opts.ttable_parts);
  MapperSource input(cin);
  MapperSink output(cout);

  Mapper(opts, table, &input, &output).Run();

  return 0;
}
