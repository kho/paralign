#include <cstdlib>
#include <iostream>
#include <string>

#include "io.h"
#include "reducer.h"
#include "ttable.h"
#include "contrib/log.h"


using namespace std;
using namespace paralign;

static string GetPartition() {
  char *env = getenv("mapred_task_partition");
  if (env == NULL) env = getenv("mapreduce_task_partition");
  if (env == NULL) LOG(FATAL) << "Cannot find partition!";
  return string(env);
}

int main() {
  Options opts = Options::FromEnv();
  const char *mapreduce_task_output_dir = getenv("mapreduce_task_output_dir");
  if (mapreduce_task_output_dir == NULL)
    LOG(FATAL) << "Cannot read mapreduce_task_output_dir from env; are you using hadoop?";
  TTableWriter writer(mapreduce_task_output_dir, GetPartition());
  ReducerSource input(cin);
  ReducerSink output(cout);

  Reducer(opts, &writer, &input, &output, Reducer::kReducer).Run();

  return 0;
}
