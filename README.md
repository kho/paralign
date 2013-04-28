paralign
========
paralign is parallel word alignment program for machine translation on Hadoop. It implements the word alignment algorithm in [Dyer et al. 2013].

[Dyer et al. 2013]: http://www.ark.cs.cmu.edu/cdyer/fast_valign.pdf

Dependencies
------------
paralign depends on the following libraries:
- boost
- hadoop
- libhdfs

Older versions of gcc (< 4.5) does not properly handle struct packing in templates, newer versions are recommended (or just use clang). If you have to use an old gcc, `ttable_test` should fail in two tests about size checks. This does not prevent the program from producing correct result, but the execution will require about 1/3 more memory, disk space and IO.

How to build
------------
First of all, you need typical GNU toolchains (autoconf, automake, libtool, make) and Apache ant.

Then, run
```
autoreconf -if
./configure
make
make install
```

If boost is installed at a non-standard location, you can specify it in `./configure` with `--with-boost=LOCATION`.

The configure script also looks for JNI library under the directory specified by `$JAVA_HOME`. You need to set it both at configure and when running the program.

To build the unit tests, run `make check` and run all executables named with a `_test` suffix.

How to run
----------

### Prepare data

You need sentence aligned parallel data. Do any preprocessing as you see necessary (tokenization, lower-casing, filtering long sentences). paralign assumes each line is a single sentence and words are separated by a single space (only space, no _tabs_).

Suppose the French and English sides of the corpus are stored as `fr.txt` and `en.txt`, respectively. Run the following to prepare input for paralign and put it on your HDFS,
```
paste fr.txt en.txt | pa-corpus.py | bzip2 - | hadoop fs -put - CORPUS_NAME.bz2
```

The input should not have any blank lines on either side. When this happens, `pa-corpus.py` will warn you and you will not get alignment output for these lines.

### Actual alignment

Then, run the following to align with French as the source side,
```
WORKDIR=hdfs://YOUR_WORK_DIR INPUT=CORPUS_NAME.bz2 ITERS=N pa-hadoop.bash
```

- `WORKDIR` is where you want to put your intermediate data, consider putting them under a temporary directory since most of the data will be useless after a successful run. It must be a full path specify as a URI.
- `INPUT` is what you have just put onto HDFS in last step.
- `ITERS` is the number of EM iterations, if you don't specify the number, it defaults to 5, which is usually enough.

Next, run the following to align with English as the source side,
```
WORKDIR=hdfs://YOUR_ANOTHER_WORK_DIR INPUT=CORPUS_NAME.bz2 ITERS=N REVERSE=yes pa-hadoop.bash
```

The only significant difference is the `REVERSE=yes` variable, which tells paralign to reverse the source-target order.

You can also manually specify the number of mappers and reducers in your jobs, simply set `MAPS=[number]` or `REDUCES=[number]`. Setting an appropriate number of mappers and reducers is crucial to how long the jobs take. But it has no effect on the correctness of the final alignment output.

### Post-processing

To get Viterbi alignment, run
```
hadoop fs -get hdfs://YOUR_WORK_DIR/viterbi/part-00000 fr-en.viterbi
hadoop fs -get hdfs://YOUR_ANOTHER_WORK_DIR/viterbi/part-00000 fr-en.reverse.viterbi
```

Each line of these two files is a tab-delimited key-value pair, with the key being sentence number and the value being the alignment points. If you don't have any sentence pair filtered by `pa-corpus.py`, simply take the values and do the normal grow-diag-final-and symmetrization.
```
cut -f2 fr-en.viterbi fr-en.al
cut -f2 fr-en.reverse.viterbi fr-en.reverse.al
GDFA_TOOL_OF_YOUR_CHOICE fr-en.al fr-en.reverse.al
```
