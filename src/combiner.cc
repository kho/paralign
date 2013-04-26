#include <iostream>

#include "io.h"
#include "reducer.h"
#include "ttable.h"
#include "contrib/log.h"

using namespace std;
using namespace paralign;

int main() {
  Options opts = Options::FromEnv();
  ReducerSource input(cin);
  ReducerSink output(cout);

  Reducer(opts, NULL, &input, &output, Reducer::kCombiner).Run();

  return 0;
}
