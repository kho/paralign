#include "options.h"

#include <cstdlib>
#include <cstring>
#include <ostream>
#include <boost/lexical_cast.hpp>

#include "log.h"

using namespace std;

static inline void SetBooleanFromEnv(const char *name, bool *dest) {
  const char *env = getenv(name);
  if (env == NULL) return;
  const char *true_strings[] = { "true", "yes", "y", "1", NULL};
  const char *false_strings[] = { "false", "no", "n", "0", NULL };
  for (const char **i = true_strings; *i; ++i) {
    if (strcmp(env, *i) == 0) {
      *dest = true;
      return;
    }
  }
  for (const char **i = false_strings; *i; ++i) {
    if (strcmp(env, *i) == 0) {
      *dest = false;
      return;
    }
  }
  LOG(FATAL) << "Invalid " << name << ": expected bool, got " << env << ".";
}

template <class T>
static inline void SetNumberFromEnv(const char *name, T *dest) {
  const char *env = getenv(name);
  if (env == NULL) return;
  try {
    *dest = boost::lexical_cast<T>(env);
  } catch (const boost::bad_lexical_cast &e) {
    LOG(FATAL) << "Invalid " << name << ": expected number, got " << env << ".";
  }
}

namespace paralign {

Options Options::FromEnv() {
  Options ret;
  SetBooleanFromEnv("pa_reverse", &ret.reverse);
  SetBooleanFromEnv("pa_favor_diagonal", &ret.favor_diagonal);
  SetNumberFromEnv("pa_prob_align_null", &ret.prob_align_null);
  SetNumberFromEnv("pa_diagonal_tension", &ret.diagonal_tension);
  SetBooleanFromEnv("pa_optimize_tension", &ret.optimize_tension);
  SetBooleanFromEnv("pa_variational_bayes", &ret.variational_bayes);
  SetNumberFromEnv("pa_alpha", &ret.alpha);
  SetBooleanFromEnv("pa_no_null_word", &ret.no_null_word);
  return ret;
}

ostream &operator<<(ostream &output, const Options &opts) {
  output << "reverse = " << opts.reverse << endl
         << "favor_diagonal = " << opts.favor_diagonal << endl
         << "prob_align_null = " << opts.prob_align_null << endl
         << "diagonal_tension = " << opts.diagonal_tension << endl
         << "optimize_tension = " << opts.optimize_tension << endl
         << "variational_bayes = " << opts.variational_bayes << endl
         << "alpha = " << opts.alpha << endl
         << "no_null_word = " << opts.no_null_word << endl;
  return output;
}
} // namespace paralign
