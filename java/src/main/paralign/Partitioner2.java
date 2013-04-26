package paralign;

import org.apache.hadoop.io.Text;

public class Partitioner2 extends org.apache.hadoop.mapreduce.Partitioner<Text, Text> {
  public int getPartition(Text key, Text value, int numPartitions) {
    int p = Integer.parseInt(key.toString()) % numPartitions;
    if (p < 0) p += numPartitions;
    return p;
  }
}
