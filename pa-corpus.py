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

for line in sys.stdin:
    src, tgt = line.split('\t', 1)
    print ' '.join(map(to_int, src.split())) + '\t' + ' '.join(map(to_int, tgt.split()))

