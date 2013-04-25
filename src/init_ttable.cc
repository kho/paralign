#include <string>
#include <boost/lexical_cast.hpp>

#include "options.h"
#include "ttable.h"
#include "contrib/log.h"

using namespace std;
using namespace paralign;

int main() {
  Options opts = Options::FromEnv();
  LOG(INFO) << "Creating " << opts.ttable_parts << "-piece init ttable at " << opts.ttable_dir;

  for (int i = 0; i < opts.ttable_parts; ++i) {
    TTableWriter writer(opts.ttable_dir, boost::lexical_cast<string>(i));
    writer.WriteIndex();
    writer.Close();
  }

  return 0;
}
