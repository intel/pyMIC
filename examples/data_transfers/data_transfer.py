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

import pymic
import numpy

device = pymic.devices[0]
stream = device.get_default_stream()

a = numpy.arange(0.0, 16.0)
b = numpy.empty_like(a)

# determine size of the array in bytes and get pointer
nbytes = a.dtype.itemsize * a.size
ptr_a_host = a.ctypes.data

# allocate buffer spaces in the target
device_ptr_1 = stream.allocate_device_memory(nbytes)
device_ptr_2 = stream.allocate_device_memory(nbytes)

# transfer a into the first buffer, then copy buffer on the device
stream.transfer_host2device(ptr_a_host, device_ptr_1, nbytes)
stream.transfer_device2device(device_ptr_1, device_ptr_2, nbytes)

print('Before transfer_device2host()')
print(b)

# transfer data back into 'b' array
ptr_b_host = b.ctypes.data
stream.transfer_device2host(device_ptr_2, ptr_b_host, nbytes)
stream.sync()

print('After transfer_device2host()')
print(b)
