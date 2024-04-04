#!/usr/bin/env python3

import sys, random, string

if len(sys.argv) != 3:
    print("usage:", sys.argv[0], " <length of key> <number of keys>")
    sys.exit(1)

L = None
N = None
try:
    L = int(sys.argv[1])
    N = int(sys.argv[2])
except:
    print("usage:", sys.argv[0], " <length of key> <number of keys>")
    sys.exit(1)

m = {}

for _ in range(N):
    s = ''.join(random.choices(string.ascii_letters + string.digits, k=L))
    x = random.randint(0, 100_000)
    m[s] = x;

for k, v in m.items():
    print(k, v)
