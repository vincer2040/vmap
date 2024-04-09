#!/usr/bin/env python3
import sys
import matplotlib.pyplot as plt

four_byte_keys = []
sixty_four_byte_keys = []
amts = []

num_ones = 0

for line in sys.stdin:
    line = line.strip()
    s = line.split(" ")
    amt = int(s[0])
    time = float(s[1])
    if amt == 1:
        num_ones += 1
    if num_ones < 2:
        amts.append(amt)
        four_byte_keys.append(time)
    else:
        sixty_four_byte_keys.append(time)

plt.plot(amts, four_byte_keys)
plt.xscale("log")

plt.show()
