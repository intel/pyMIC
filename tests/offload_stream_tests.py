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

import unittest
import numpy

import pymic 

from helper import skipNoDevice
from helper import get_library


class OffloadStreamTests(unittest.TestCase):
    """This class defines the test cases for the OffloadStream class."""

    @skipNoDevice
    def test_invalid_kernel_name(self):
        """Test if invoke_kernel_string throws an exception if a kernel's 
           name does not exist in any loaded library."""

        device = pymic.devices[0]
        library = get_library(device, "libtests.so")
        stream = device.get_default_stream()
            
        try:
            self.assertRaises(pymic.OffloadError, 
                              library.this_kernel_does_not_exist_anywhere)
        except pymic.OffloadError:
            self.assertTrue(True)
        else:
            self.assertFalse(False)

    @skipNoDevice
    def test_invoke_empty_kernel(self):
        """Test if invoke_kernel_string does successfully invoke a 
           kernel function with no arguments."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        library = get_library(device, "libtests.so")
        stream.invoke(library.test_offload_stream_empty)
        stream.sync()

    @skipNoDevice
    def test_invoke_kernel_scalar(self):
        """Test if a kernel is correctly invoked with copyin/copyout
           semantics for scalar arguments."""

        a = 42
        b = 42.0
        c = 0
        d = 0.0
        ival = numpy.empty((2,), dtype=int)
        dval = numpy.empty((2,), dtype=float)
        argc = numpy.empty((1,), dtype=int)
        device = pymic.devices[0]
        library = get_library(device, "libtests.so")
        stream = device.get_default_stream()
        stream.invoke(library.test_offload_stream_kernel_scalars, 
                      a, b, c, d, ival, dval, argc)
        stream.sync()
        self.assertEqual((ival[0], ival[1]), (42, 0))
        self.assertEqual((dval[0], dval[1]), (42.0, 0.0))
        self.assertEqual(argc[0], 7)

    @skipNoDevice
    def test_invoke_kernel_arrays_int(self):
        """Test if a kernel is correctly invoked with copyin/copyout
           semantics for array arguments of dtype=int."""

        a = numpy.arange(0, 4711 * 1024, dtype=int)
        b = numpy.arange(0, 4096, dtype=int)
        c = numpy.empty((2,), dtype=int)
        d = 13
        m = 7

        a_expect = a / d
        b_expect = b % m
        c_expect = (a.shape[0], b.shape[0])

        device = pymic.devices[0]
        library = get_library(device, "libtests.so")
        stream = device.get_default_stream()
        stream.invoke(library.test_offload_stream_kernel_arrays_int, 
                      a, b, c, d, m)
        stream.sync()

        self.assertTrue((a == a_expect).all(), 
                        "Wrong contents of array: "
                        "{0} should be {1}".format(a, a_expect))
        self.assertTrue((a == a_expect).all(), 
                        "Wrong contents of array: "
                        "{0} should be {1}".format(b, b_expect))
        self.assertTrue((c == c_expect).all(),
                        "Wrong number of array elements ({0} should be "
                        "{1}, {2} should be {3}".format(c[0], c[1], 
                                                        c_expect[0], 
                                                        c_expect[1]))

    @skipNoDevice
    def test_invoke_kernel_arrays_float(self):
        """Test if a kernel is correctly invoked with copyin/copyout
           semantics for array arguments of dtype=float."""

        a = numpy.arange(0.0, 4711.0 * 1024, dtype=float)
        b = numpy.arange(0.0, 4096.0, dtype=float)
        c = numpy.empty((2,), dtype=int)
        p = 1.5
        s = 0.5

        a_expect = a + p
        b_expect = b - s
        c_expect = (a.shape[0], b.shape[0])

        device = pymic.devices[0]
        library = get_library(device, "libtests.so")
        stream = device.get_default_stream()
        stream.invoke(library.test_offload_stream_kernel_arrays_float, 
                      a, b, c, p, s)
        stream.sync()

        self.assertTrue((a == a_expect).all(), 
                        "Wrong contents of array: "
                        "{0} should be {1}".format(a, a_expect))
        self.assertTrue((a == a_expect).all(), 
                        "Wrong contents of array: "
                        "{0} should be {1}".format(b, b_expect))
        self.assertTrue((c == c_expect).all(),
                        "Wrong number of array elements ({0} should be "
                        "{1}, {2} should be {3}".format(c[0], c[1], 
                                                        c_expect[0], 
                                                        c_expect[1]))

    @skipNoDevice
    def test_bind_int(self):
        """Test proper functioning of the bind operation."""

        pattern = int(0xdeadbeefabbaabba)
        device = pymic.devices[0]
        library = get_library(device, "libtests.so")
        stream = device.get_default_stream()
        a = numpy.empty((4711 * 1024,), dtype=int)
        r = numpy.empty((1,), dtype=int)

        a[:] = pattern
        offl_a = stream.bind(a)
        offl_r = stream.bind(r)
        stream.invoke(library.test_check_pattern, offl_a, offl_r, pattern)
        offl_r.update_host()
        stream.sync()

        self.assertEqual(r[0], a.shape[0])
        self.assertEqual(offl_a.dtype, a.dtype)
        self.assertEqual(offl_a.shape, a.shape)

    @skipNoDevice
    def test_bind_float(self):
        """Test proper functioning of the bind operation."""

        device = pymic.devices[0]
        library = get_library(device, "libtests.so")
        stream = device.get_default_stream()
        a = numpy.arange(0.0, 4711.0 * 1024, dtype=float)
        r = numpy.empty((1,), dtype=float)

        offl_a = stream.bind(a)
        offl_r = stream.bind(r)
        stream.invoke(library.test_sum_float, offl_a, offl_r)
        offl_r.update_host()
        stream.sync()

        self.assertEqual(r[0], sum(a))
        self.assertEqual(offl_a.dtype, a.dtype)
        self.assertEqual(offl_a.shape, a.shape)

    @skipNoDevice
    def test_lowlevel_transfers(self):
        device = pymic.devices[0]
        stream = device.get_default_stream()

        a = numpy.arange(0.0, 16.0)
        b = numpy.empty_like(a)

        a_expect = numpy.empty_like(a)
        a_expect[:] = a[:]
        b_expect = numpy.empty_like(a)
        b_expect[:] = a[:]

        nbytes = a.dtype.itemsize * a.size
        ptr_a_host = a.ctypes.data
        ptr_b_host = b.ctypes.data

        device_ptr_1 = stream.allocate_device_memory(nbytes)
        device_ptr_2 = stream.allocate_device_memory(nbytes)

        stream.transfer_host2device(ptr_a_host, device_ptr_1, nbytes)
        stream.transfer_device2device(device_ptr_1, device_ptr_2, nbytes)

        stream.transfer_device2host(device_ptr_2, ptr_b_host, nbytes)
        stream.sync()

        self.assertTrue((a == a_expect).all(), 
                        "Wrong contents of array: "
                        "{0} should be {1}".format(b, b_expect))
        self.assertTrue((b == b_expect).all(), 
                        "Wrong contents of array: "
                        "{0} should be {1}".format(b, b_expect))

    @skipNoDevice
    def test_lowlevel_transfers_offsets(self):
        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(0.0, 32.0)
        b = numpy.empty_like(a)

        a_expect = numpy.empty_like(a)
        a_expect[:] = a[:]
        b_expect = numpy.empty_like(a)
        b_expect[0:a.size / 2] = a[a.size / 2:]
        b_expect[a.size / 2:] = a[0:a.size / 2]

        nbytes = a.dtype.itemsize * a.size
        ptr_a_host = a.ctypes.data
        ptr_b_host = b.ctypes.data

        device_ptr_1 = stream.allocate_device_memory(nbytes)
        device_ptr_2 = stream.allocate_device_memory(nbytes)

        stream.transfer_host2device(ptr_a_host, device_ptr_1, nbytes / 2, 
                                    offset_host=0, offset_device=nbytes / 2)
        stream.transfer_host2device(ptr_a_host, device_ptr_1, nbytes / 2, 
                                    offset_host=nbytes / 2, offset_device=0)

        for i in xrange(0, 4):                            
            stream.transfer_device2device(device_ptr_1, device_ptr_2, 
                                          nbytes / 4,
                                          offset_device_src=i * (nbytes / 4), 
                                          offset_device_dst=(3 - i) *
                                                            (nbytes / 4))
        for i in xrange(0, 4):
            stream.transfer_device2host(device_ptr_2, ptr_b_host, nbytes / 4,
                                        offset_device=i * (nbytes / 4), 
                                        offset_host=(3 - i) * (nbytes / 4))
        stream.sync()

        self.assertTrue((a == a_expect).all(), 
                        "Wrong contents of array: "
                        "{0} should be {1}".format(b, b_expect))
        self.assertTrue((b == b_expect).all(), 
                        "Wrong contents of array: "
                        "{0} should be {1}".format(b, b_expect))
