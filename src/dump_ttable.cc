#include <iostream>
#include "options.h"
#include "ttable.h"

using namespace std;
using namespace paralign;

int main() {
  Options opts = Options::FromEnv();
  LOG(INFO) << "Dumping " << opts.ttable_parts << "-piece ttable from " << opts.ttable_dir;
  TTable table(opts.ttable_dir, opts.ttable_parts);
  table.Dump(cout);
  return 0;
}
