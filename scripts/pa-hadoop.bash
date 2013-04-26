#!/bin/bash

set -e

function INFO {
    echo "INFO:" "$@" 1>&2
}

[ "x$WORKDIR" != x ] || { INFO "Set WORKDIR!"; exit 1; }
[ "x$INPUT" != x ] || { INFO "Set INPUT!"; exit 1; }

echo "$WORKDIR" | grep -q "^hdfs://" || { INFO "WORKDIR must starts with hdfs://"; exit 1; }

# FIXME: this assume the user only customizes prefix
JAR=/cliphomes/wuke/paralign/build/share/paralign/paralign-0.1.jar
LIBEXEC=/cliphomes/wuke/paralign/build/libexec/paralign
# FIXME: hardcoded streaming jar path
STREAMING=/usr/lib/hadoop-mapreduce/hadoop-streaming.jar

for i in hadoop; do
    which $i > /dev/null 2> /dev/null || { INFO "Cannot find $i! Is it on your PATH?"; exit 1; }
done

for i in pa-mapper pa-reducer pa-combiner pa-diagonal pa-env.sh; do
    [ -x "$LIBEXEC/$i" ] || { INFO "Cannot find $i under $LIBEXEC!"; exit 1; }
done

if [ "x$PARTS" = x ]; then
    PARTS=1
fi

if [ "x$VB" = x ]; then
    VB=no
fi

if [ "x$MEM" = x ]; then
    MEM=8000
fi

if [ "x$ITERS" = x ]; then
    ITERS=5
fi

INFO "INPUT = $INPUT"
INFO "PARTS = $PARTS"
INFO "VB = $VB"
INFO "ITERS = $ITERS"
INFO "WORKDIR = $WORKDIR"
INFO "MEM = $MEM"

TENSION=4

export pa_ttable_parts=$PARTS
export pa_variational_bayes=$VB
export pa_diagonal_tension=$TENSION

# Create initial parameters
INFO "Creating initial parameters..."
hadoop fs -mkdir -p "$WORKDIR/0000"
for i in `seq 0 $(($PARTS-1))`; do
    hadoop fs -put /dev/null "$WORKDIR/0000/index.$i"
    hadoop fs -put /dev/null "$WORKDIR/0000/entry.$i"
done

export pa_ttable_dir=$WORKDIR/0000
for i in `seq $ITERS`; do
    CUR="$WORKDIR/`printf %04d $i`"
    # Prepare -files options
    FILES="$LIBEXEC/pa-mapper,$LIBEXEC/pa-combiner,$LIBEXEC/pa-reducer,$LIBEXEC/pa-env.sh,"
    for j in `seq 0 $(($PARTS-1))`; do
	FILES="$FILES,$pa_ttable_dir/entry.$j,$pa_ttable_dir/index.$j"
    done
    # Streaming command
    hadoop jar "$STREAMING" \
	-D mapreduce.job.name="align-`basename "$WORKDIR"`-$i" \
	-D mapreduce.reduce.memory.mb="$MEM" \
	-files "$FILES" \
	-libjars "$JAR" \
	-mapper "/usr/bin/time -v ./pa-mapper" \
	-reducer "/usr/bin/time -v ./pa-env.sh ./pa-reducer" \
	-combiner "/usr/bin/time -v ./pa-env.sh ./pa-combiner" \
	-partitioner "paralign.Partitioner1" \
	-input "$INPUT" \
	-output "$CUR" \
	-numReduceTasks $PARTS \
	-cmdenv pa_ttable_parts=$PARTS \
	-cmdenv pa_variational_bayes=$VB \
	-cmdenv pa_diagonal_tension=$TENSION \
	-cmdenv pa_ttable_dir=.
    # Run diagonal tension optimizer
    if [ "$i" -eq 1 ]; then
	export pa_optimize_tension=no
    else
	export pa_optimize_tension=yes
    fi
    INFO "ITERATION $i"
    R=`hadoop fs -cat "$CUR/part-"* | LC_ALL=C sort | "$LIBEXEC/pa-env.sh" "$LIBEXEC/pa-diagonal"`
    # For next iteration
    export pa_ttable_dir=$CUR
    if [ "$i" -gt 1 ]; then
	[ "x$R" != x ] || { INFO "Tension optimization failed!"; exit 1; }
	TENSION=$R
	echo $TENSION | hadoop fs -put - "$CUR/diagonal.out"
	export pa_diagonal_tension=$TENSION
    fi
done
