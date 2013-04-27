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

class Viterbi {
 public:
  Viterbi(const Options &opts, const TTable &table, MapperSource *input, ViterbiSink *output)
      : opts_(opts), tbl_(table), in_(input), out_(output) {}

  void Run() {
    size_t id;
    vector<WordId> src, tgt;
    for (; !in_->Done(); in_->Next()) {
      in_->Read(&id, &src, &tgt);
      if (opts_.reverse) swap(src, tgt);
      Map(id, src, tgt);
    }
  }

 private:
  // FIXME: a lot of duplicate code (vs mapper.cc)
  void Map(size_t id, const vector<WordId> &src, const vector<WordId> &tgt) {
    vector<SentSzPair> al;
    for (size_t j = 0; j < tgt.size(); ++j) {
      const WordId f_j = tgt[j];
      double prob_a_i = 1.0 / (src.size() + !opts_.no_null_word);  // uniform (model 1)
      WordId max_i = 0;
      double max_p = -1;
      int max_index = -1;
      // Null
      if (!opts_.no_null_word) {
        max_i = kNull;
        max_index = 0;
        if (opts_.favor_diagonal) prob_a_i = opts_.prob_align_null;
        max_p = tbl_.Query(kNull, f_j) * prob_a_i;
      }
      // Non-null
      double az = 0;
      if (opts_.favor_diagonal)
        az = DiagonalAlignment::ComputeZ(j+1, tgt.size(), src.size(), opts_.diagonal_tension) / (1 - opts_.prob_align_null);
      for (unsigned i = 1; i <= src.size(); ++i) {
        if (opts_.favor_diagonal)
          prob_a_i = DiagonalAlignment::UnnormalizedProb(j + 1, i, tgt.size(), src.size(), opts_.diagonal_tension) / az;
        double prob = tbl_.Query(src[i-1], f_j) * prob_a_i;
        if (prob > max_p) {
          max_index = i;
          max_p = prob;
          max_i = src[i-1];
        }
      }
      // Alignment point
      if (max_index > 0) {
        if (opts_.reverse)
          al.push_back(MkSzPair(j, max_index - 1));
        else
          al.push_back(MkSzPair(max_index - 1, j));
      }
    }
    out_->WriteAlignment(id, al.begin(), al.end());
  }

  const Options opts_;
  const TTable &tbl_;
  MapperSource *in_;
  ViterbiSink *out_;
};
} // namespace paralign

using namespace paralign;

int main() {
  Options opts = Options::FromEnv();

  LOG(INFO) << "Options:" << endl
            << opts << endl;

  TTable table(opts.ttable_dir, opts.ttable_parts);
  MapperSource input(cin);
  ViterbiSink output(cout);

  Viterbi(opts, table, &input, &output).Run();

  return 0;
}
