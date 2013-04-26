#!/bin/bash

[ "x$WORKDIR" != x ] || { echo "Set WORKDIR!"; exit 1; }
[ "x$INPUT" != x ] || { echo "Set INPUT!"; exit 1; }
[ ! -e "$WORKDIR" ] || { echo "$WORKDIR already exists"; exit 1; }
[ -d "$INPUT" ] || { echo "$INPUT does not exist or is not a directory"; exit 1; }
mkdir -p "$WORKDIR"

for i in pa-mapper pa-reducer pa-combiner pa-diagonal pa-env.sh; do
    which $i > /dev/null 2> /dev/null || { echo "Cannot find $i! Is it on your PATH?"; exit 1; }
done

set -e

BASE=`readlink -f "$0"`
BASE=`dirname "$BASE"`

if [ "x$PARTS" = x ]; then
    PARTS=1
fi

if [ "x$VB" = x ]; then
    VB=no
fi

echo "INPUT = $INPUT"
echo "PARTS = $PARTS"
echo "VB = $VB"
echo "WORKDIR = $WORKDIR"

export pa_ttable_parts=$PARTS
export pa_variational_bayes=$VB

# Create initial parameters
export pa_ttable_dir=$WORKDIR/0000
mkdir -p "$WORKDIR/0000"
pa-env.sh pa-init-ttable 2> "$WORKDIR/0000/init.err"

for i in `seq 5`; do
    CUR="$WORKDIR/`printf %04d $i`"
    mkdir -p "$CUR"
    # Run mapper
    for j in "$INPUT"/*; do
	TASK=`basename "$j"`
	pa-mapper < $j > "$CUR/map.$TASK.out" 2> "$CUR/map.$TASK.err" &
    done
    wait
    # Run partitioner
    cat "$CUR/map."*.out | "$BASE/partitioner.py" -n $PARTS -o "$CUR/part.%d.out" 2> "$CUR/part.err"
    # Run combiner
    for j in `seq 0 $(($PARTS-1))`; do
	LC_ALL=C sort "$CUR/part.$j.out" | pa-env.sh pa-combiner > "$CUR/combine.$j.out" 2> "$CUR/combine.$j.err" &
    done
    wait
    # Run reducer
    export mapreduce_task_output_dir=$CUR
    for j in `seq 0 $(($PARTS-1))`; do
	export mapreduce_task_partition=$j
	LC_ALL=C sort "$CUR/combine.$j.out" | pa-env.sh pa-reducer > "$CUR/reduce.$j.out" 2> "$CUR/reduce.$j.err" &
    done
    wait
    # Run diagonal tension optimizer
    if [ "$i" -eq 1 ]; then
	export pa_optimize_tension=no
    else
	export pa_optimize_tension=yes
    fi
    LC_ALL=C sort "$CUR/reduce."*.out | pa-env.sh pa-diagonal > "$CUR/diagonal.out" 2> "$CUR/diagonal.err"
    # For next iteration
    export pa_ttable_dir=$CUR
    if [ "$i" -gt 1 ]; then
	export pa_diagonal_tension=`cat "$CUR/diagonal.out"`
    fi

    echo "ITERATION $i"
    cat "$CUR/diagonal.err"
done
