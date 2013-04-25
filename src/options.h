#ifndef _PARALIGN_OPTIONS_H_
#define _PARALIGN_OPTIONS_H_

#include <iosfwd>
#include <string>

namespace paralign {
// Options for controlling the alignment
struct Options {
  // Reverse estimation (swap source and target during training)
  bool reverse;
  // Use a static alignment distribution that assigns higher probabilities to alignments near the diagonal
  bool favor_diagonal;
  // When `favor_diagonal` is set, what's the probability of a null alignment?
  double prob_align_null;
  // How sharp or flat around the diagonal is the alignment distribution (<1 = flat >1 = sharp)
  double diagonal_tension;
  // Optimize diagonal tension during EM
  bool optimize_tension;
  // Infer VB estimate of parameters under a symmetric Dirichlet prior
  bool variational_bayes;
  // Hyperparameter for optional Dirichlet prior
  double alpha;
  // Do not generate from a null token
  bool no_null_word;
  // Path prefix of translation tables
  std::string ttable_prefix;
  // Number of translation table pieces
  int ttable_parts;

  // Default values
  Options()
      : reverse(false), favor_diagonal(true), prob_align_null(0.08),
        diagonal_tension(4.0), optimize_tension(true), variational_bayes(true),
        alpha(0.01), no_null_word(false), ttable_prefix("ttable"), ttable_parts(0) {}

  // Construct from environment variables
  static Options FromEnv();

  // Show options
  friend std::ostream &operator<<(std::ostream &, const Options &);

  // Check for invalid combination of options
  void Check() const;
};

std::ostream &operator<<(std::ostream &, const Options &);
}      // namespace paralign

#endif  // _PARALIGN_OPTIONS_H_
