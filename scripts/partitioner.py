#!/usr/bin/env python

import argparse
import sys

parser = argparse.ArgumentParser()
parser.add_argument('-n', type=int, default=1, help='Number of partitions')
parser.add_argument('-o', required=True, help='Output file name formatter')

args = parser.parse_args()

N = args.n
O = args.o

fo = [open(O % i, 'w') for i in range(N)]

for line in sys.stdin:
    key, _ = line.split('\t', 1)
    part = int(key) % N
    if part < 0:
        part += N
    fo[part].write(line)

for f in fo:
    f.close()
