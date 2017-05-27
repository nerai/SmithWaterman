#!/usr/bin/env python

import sys
import csv
import numpy as np
import matplotlib.pyplot as plt

if len(sys.argv) != 2:
	print("usage: prog <input csv>")
	exit(1)

input = sys.argv[1]

with open(input) as f:
    reader = csv.reader(f)
    header = reader.next()
    x = []
    y = []
    for row in reader:
        x.append(row[0])
        y.append(row[1])

plt.scatter(x, y, color='r', s=50, marker='x')
plt.xlabel(header[0], fontsize=18)
plt.ylabel(header[1], fontsize=18)
plt.show()

exit(0)
