#!/usr/bin/python

# Copyright (c) 2014-2016, Intel Corporation All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without 
# modification, are permitted provided that the following conditions are 
# met: 
# 
# 1. Redistributions of source code must retain the above copyright 
# notice, this list of conditions and the following disclaimer. 
#
# 2. Redistributions in binary form must reproduce the above copyright 
# notice, this list of conditions and the following disclaimer in the 
# documentation and/or other materials provided with the distribution. 
#
# 3. Neither the name of the copyright holder nor the names of its 
# contributors may be used to endorse or promote products derived from 
# this software without specific prior written permission. 
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS 
# IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED 
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A 
# PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED 
# TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 

from __future__ import print_function

import sys
import time

import pymic
import numpy as np


def limiter(data_size):
    if data_size < 16384:
        return 100000
    if data_size < 131072:
        return 10000
    if data_size < 16777216:
        return 1000
    return 100

benchmark = sys.argv[0][2:][:-3]

# number of elements to copyin (8B to 2 GB)
data_sizes = [pow(2, i) for i in range(29)]
repeats = map(limiter, data_sizes)

device = pymic.devices[0]
device.load_library("libbenchmark_kernels.so")
stream = device.get_default_stream()

timings = {}
for ds, nrep in zip(data_sizes, repeats):
    print("Measuring {0} elements ({1} bytes, "
          "repeating {2})".format(ds, ds * 8, nrep))
    arr = np.zeros(ds)
    ts = time.time()
    for i in xrange(nrep):
        offl_arr = stream.bind(arr)
        stream.sync()
    te = time.time()
    timings[ds] = (te - ts, nrep)
    
try:
    csv = open(benchmark + ".csv", "w")
    print("benchmark;elements;data size;avg time;MB/sec", file=csv)
    for ds in sorted(list(timings)):
        t, nrep = timings[ds]
        t = (float(t) / nrep) 
        mbs = float(ds * 8) / 1000 / 1000 / t
        print("{0};{1};{2};{3};{4}".format(benchmark, ds, ds * 8, t, mbs),
              file=csv)
finally:
    csv.close()
