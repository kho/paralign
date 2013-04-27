#!/usr/bin/env python

import sys

d = {}

def to_int(w):
    try:
        ret = d[w]
    except KeyError:
        ret = len(d) + 1
        d[w] = ret
    return str(ret)

n = 0

for line in sys.stdin:
    n += 1
    src, tgt = line.split('\t', 1)
    src = ' '.join(map(to_int, src.split()))
    tgt = ' '.join(map(to_int, tgt.split()))
    if src and tgt:
        print str(n) + '\t' + src + '\t' + tgt
    else:
        print >> sys.stderr, 'skipping line %d: at least one side is empty' % n
