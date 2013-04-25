#include "options.h"

#include <cstdlib>
#include <cstring>
#include <ostream>
#include <boost/lexical_cast.hpp>

#include "contrib/log.h"

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
  LOG(FATAL) << "Invalid " << name << ": expected bool, got " << env;
}

template <class T>
static inline void SetNumberFromEnv(const char *name, T *dest) {
  const char *env = getenv(name);
  if (env == NULL) return;
  try {
    *dest = boost::lexical_cast<T>(env);
  } catch (const boost::bad_lexical_cast &e) {
    LOG(FATAL) << "Invalid " << name << ": expected number, got " << env;
  }
}

static inline void SetStringFromEnv(const char *name, string *dest) {
  const char *env = getenv(name);
  if (env == NULL) return;
  *dest = env;
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
  SetStringFromEnv("pa_ttable_prefix", &ret.ttable_prefix);
  SetNumberFromEnv("pa_ttable_parts", &ret.ttable_parts);
  ret.Check();
  return ret;
}

void Options::Check() const {
  if (favor_diagonal && (prob_align_null < 0 || prob_align_null > 1))
    LOG(FATAL) << "prob_align_null must be probability: " << prob_align_null;
  if (variational_bayes && (alpha <= 0))
    LOG(FATAL) << "alpha must be positive: " << alpha;
  if (ttable_parts <= 0)
    LOG(FATAL) << "ttable_parts not given or invalid: " << ttable_parts;
}

ostream &operator<<(ostream &output, const Options &opts) {
  output << "reverse = " << opts.reverse << endl
         << "favor_diagonal = " << opts.favor_diagonal << endl
         << "prob_align_null = " << opts.prob_align_null << endl
         << "diagonal_tension = " << opts.diagonal_tension << endl
         << "optimize_tension = " << opts.optimize_tension << endl
         << "variational_bayes = " << opts.variational_bayes << endl
         << "alpha = " << opts.alpha << endl
         << "no_null_word = " << opts.no_null_word << endl
         << "ttable_prefix = " << opts.ttable_prefix << endl
         << "ttable_parts = " << opts.ttable_parts << endl
         << "local = " << opts.local << endl;
  return output;
}
} // namespace paralign
