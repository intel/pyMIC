#!/usr/bin/python

# Copyright (c) 2014, Intel Corporation All rights reserved. 
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

import sys
import time

import pyMIC as mic
import numpy as np

def limiter(data_size):
    if data_size < 128:
        return 10000
    if data_size < 1024:
        return 1000
    if data_size < 8192:
        return 100
    return 10

benchmark = sys.argv[0][2:][:-3]

# number of elements to copyin (8B to 2 GB)
data_sizes = []
data_sizes = [(128+i*128) for i in range(64)]
repeats = map(limiter, data_sizes)

device = mic.devices[0]
device.load_library("libbenchmark_kernels.so")

timings = {}
timings_kernel = {}
np.random.seed(10)
for ds,nrep in zip(data_sizes,repeats):
    print "Measuring {0}x{0} (repeating {2})".format(ds, ds * 8, nrep)
    
    m,k,n = ds, ds, ds
    
    a = np.random.random(m*k).reshape((m, k))
    b = np.random.random(k*n).reshape((k, n))
    c = np.zeros((m, n))
    alpha = 1.0
    beta = 0.0
    
    ts = time.time()
    for i in range(nrep):
        offl_a = device.bind(a)
        offl_b = device.bind(b)
        offl_c = device.bind(c)
        ts_kernel = time.time()
        device.invoke_kernel("dgemm_kernel", offl_a, offl_b, offl_c, m, n, k, alpha, beta)
        te_kernel = time.time()
    te = time.time()
    timings[ds] = (te - ts, nrep)
    offl_a = device.bind(a)
    offl_b = device.bind(b)
    offl_c = device.bind(c)
    ts_kernel = time.time()
    for i in range(nrep):
        device.invoke_kernel("dgemm_kernel", offl_a, offl_b, offl_c, m, n, k, alpha, beta)
    te_kernel = time.time()
    timings_kernel[ds] = (te_kernel - ts_kernel, nrep)
    
try:
    csv = open(benchmark + ".csv", "w")
    print >> csv, "benchmark;elements;avg time;avg time kernel;flops;gflops;gflops kernel"
    for ds in sorted(list(timings)):
        t, nrep = timings[ds]
        t = (float(t) / nrep) 
        t_k, dummy = timings_kernel[ds]
        t_k = (float(t_k) / nrep)
        flops = 2 * ds * ds * ds
        gflops = (float(flops) / (1000 * 1000 * 1000)) / t
        gflops_k = (float(flops) / (1000 * 1000 * 1000)) / t_k
        print >> csv, "{0};{1};{2};{3};{4};{5};{6}".format(benchmark, ds, t, t_k, flops, gflops, gflops_k)
finally:
    csv.close()
                                       