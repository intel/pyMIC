#!/usr/bin/python

# Copyright (c) 2014-2015, Intel Corporation All rights reserved. 
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

import pymic as mic
import numpy as np
import sys
import time

# load the library with the kernel function (on the target)
device = mic.devices[0]
library = device.load_library("libdgemm.so")

# use the default stream
stream = device.get_default_stream()

# sizes of the matrices
ds = 4096
m, n, k = ds, ds, ds
if len(sys.argv) > 1:
    sz = int(sys.argv[1])
    m, n, k = sz, sz, sz
alpha = 1.0
beta = 0.0

# construct some matrices
np.random.seed(10)
a = np.random.random(m * k).reshape((m, k))
b = np.random.random(k * n).reshape((k, n))
c = np.zeros((m, n))

print("data sizes: m={0} k={1} n={2}".format(m, k, n))
print()

# associate host arrays with device arrats
offl_a = stream.bind(a)
offl_b = stream.bind(b)
offl_c = stream.bind(c)

# convert a and b to matrices (eases MxM in numpy)
Am = np.matrix(a)
Bm = np.matrix(b)

# print the input
print("input:")
print("--------------------------------------")
print("a=", a)
print("b=", b)
print() 
print() 

# print the input of numpy's MxM if it is small enough
np_mxm_start = time.time()
Cm = Am * Bm
np_mxm_end = time.time()
print("numpy gives us:")
print("--------------------------------------")
print(Cm)
print("checksum:", np.sum(Cm))
print()
print()

print('++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++')
# invoke the offloaded dgemm
c[:] = 0.0
offl_c.update_device()
np_mic_start = time.time()
stream.invoke(library.dgemm_kernel, 
              offl_a, offl_b, offl_c, 
              m, n, k, alpha, beta)
stream.sync()
np_mic_end = time.time()
print('++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++')

# print the output of the offloaded MxM
print("offloading computed:")
print("--------------------------------------")
offl_c.update_host()
stream.sync()
print(offl_c)
print("checksum:", np.sum(offl_c.array))
print()
print()

# print the performance information
flops = (2 * m * n * k) / 1000 / 1000 / 1000
np_mxm_time = np_mxm_end - np_mxm_start
np_mic_time = np_mic_end - np_mic_start
print("performance:")
print("--------------------------------------")
print("MxM (numpy)           {0:>6.3} sec    "
      "{1:>6.3} GFLOPS".format(np_mxm_time, flops / np_mxm_time))
print("MxM (offload)         {0:>6.3} sec    "
      "{1:>6.3} GFLOPS".format(np_mic_time, flops / np_mic_time))

print()
sum1 = np.sum(Cm)
sum2 = np.sum(offl_c.array)
if sum1 != sum2:
    print('Validation failed: ', sum1, '!=', sum2, 'diff', abs(sum1-sum2))
else:
    print('Validation succeeded: ', sum1, '==', sum2)
