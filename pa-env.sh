#!/bin/sh

BASE=`dirname "$0"`

# Add hadoop jars to classpath
for i in /usr/lib/hadoop*/*.jar /usr/lib/hadoop*/*/*.jar; do
    CLASSPATH=$CLASSPATH:$i
done

export CLASSPATH

# Find libjvm and set LD_LIBRARY_PATH
LIBJVM=`find "$JAVA_HOME/" -name libjvm.so`
test "$LIBJVM" || { echo "Cannot find libjvm.so!" 1>&2; exit 1; }

LIBJVM=`dirname "$LIBJVM"`

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$LIBJVM

"$@"
