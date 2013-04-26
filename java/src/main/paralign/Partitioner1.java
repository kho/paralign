package paralign;

import org.apache.hadoop.io.Text;

public class Partitioner1 implements org.apache.hadoop.mapred.Partitioner<Text, Text> {
  public int getPartition(Text key, Text value, int numPartitions) {
    int p = Integer.parseInt(key.toString()) % numPartitions;
    if (p < 0) p += numPartitions;
    return p;
  }

  public void configure(org.apache.hadoop.mapred.JobConf job) {
  }
}
