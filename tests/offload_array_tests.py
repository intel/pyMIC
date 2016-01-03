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

import sys
import unittest
import numpy

import pymic

from helper import skipNoDevice
from helper import get_library

epsilon = 0.0000000001
cutoff_value = 16777215

class OffloadArrayTest(unittest.TestCase):
    """This class defines the test cases for the OffloadArray class."""

    def assertEqualEpsilon(self, first, second, msg=None):
        """Test equal for floating point with deviation."""

        def epsilonCompare(value):
            return abs(value) <= epsilon

        comparison = map(epsilonCompare, (first - second))
        return self.assertTrue(all(comparison), msg)

    @skipNoDevice
    def test_bind_int(self):
        """Test if a Numpy array is correctly bound and transferred to the
           target device."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        library = get_library(device, "libtests.so")
        pattern = int(0xdeadbeefabbaabba)
        a = numpy.empty((4711 * 1024,), dtype=int)
        a[:] = pattern
        offl_a = stream.bind(a)
        r = numpy.empty((1,), dtype=int)
        offl_r = stream.bind(r)
        stream.invoke(library.test_check_pattern,
                      offl_a, offl_a.size, offl_r, pattern)
        offl_r.update_host()
        stream.sync()

        self.assertEqual(r[0], a.shape[0])

    @skipNoDevice
    def test_update_host(self):
        """Test if a Numpy array is correctly updated from the target."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        library = get_library(device, "libtests.so")
        a = numpy.empty((4711 * 1024,), dtype=int)
        a_expect = numpy.empty_like(a)
        pattern = int(0xdeadbeefabbaabba)
        a_expect[:] = pattern
        offl_a = stream.bind(a)
        stream.invoke(library.test_set_pattern, offl_a, offl_a.size, pattern)
        offl_a.update_host()
        stream.sync()

        self.assertTrue((a == a_expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(a, a_expect))

    @skipNoDevice
    def test_update_device(self):
        """Test if a Numpy array is correctly updated on the target."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        library = get_library(device, "libtests.so")
        a = numpy.empty((4711 * 1024,), dtype=int)
        a_expect = numpy.empty_like(a)
        pattern = int(0xdeadbeefabbaabba)
        offl_a = stream.bind(a)
        r = numpy.empty((1,), dtype=int)
        offl_r = stream.bind(r)
        a[:] = pattern
        offl_a.update_device()
        stream.invoke(library.test_check_pattern,
                      offl_a, offl_a.size, offl_r, pattern)
        offl_r.update_host()
        stream.sync()

        self.assertEqual(r[0], a.shape[0])

    @skipNoDevice
    def test_op_add_scalar_int(self):
        """Test __add__ operation of OffloadArray with scalar operand."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1, 4711 * 1024, dtype=int)
        s = 1

        old = numpy.empty_like(a)
        old[:] = a[:]
        expect = a + s

        offl_a = stream.bind(a)
        offl_r = offl_a + s
        offl_a.update_host()
        r = offl_r.update_host().array
        stream.sync()

        self.assertEqual(r.shape, a.shape)
        self.assertEqual(r.dtype, a.dtype)
        self.assertTrue((a == old).all(),
                        "Input array operand must not be modified: "
                        "{0} should be {1}".format(a, old))
        self.assertTrue((r == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(r, expect))

    @skipNoDevice
    def test_op_add_offload_array_int(self):
        """Test __add__ operation of OffloadArray with OffloadArray operand."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1, 4711 * 1024, dtype=int)
        o = a + 1

        old_a = numpy.empty_like(a)
        old_o = numpy.empty_like(o)
        old_a[:] = a[:]
        old_o[:] = o[:]
        expect = a + o

        offl_a = stream.bind(a)
        offl_o = stream.bind(o)
        offl_r = offl_a + offl_o
        offl_a.update_host()
        r = offl_r.update_host().array
        stream.sync()

        self.assertEqual(r.shape, a.shape)
        self.assertEqual(r.dtype, a.dtype)
        self.assertTrue((a == old_a).all(),
                        "Input array operand 1 must not be modified: "
                        "{0} should be {1}".format(a, old_a))
        self.assertTrue((o == old_o).all(),
                        "Input array operand 2 must not be modified: "
                        "{0} should be {1}".format(o, old_o))
        self.assertTrue((r == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(r, expect))

    @skipNoDevice
    def test_op_add_array_int(self):
        """Test __add__ operation of OffloadArray with array operand."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1, 4711 * 1024, dtype=int)
        o = a + 1

        old_a = numpy.empty_like(a)
        old_o = numpy.empty_like(o)
        old_a[:] = a[:]
        old_o[:] = o[:]
        expect = a + o

        offl_a = stream.bind(a)
        offl_r = offl_a + o
        offl_a.update_host()
        r = offl_r.update_host().array
        stream.sync()

        self.assertEqual(r.shape, a.shape)
        self.assertEqual(r.dtype, a.dtype)
        self.assertTrue((a == old_a).all(),
                        "Input array operand 1 must not be modified: "
                        "{0} should be {1}".format(a, old_a))
        self.assertTrue((o == old_o).all(),
                        "Input array operand 2 must not be modified: "
                        "{0} should be {1}".format(o, old_o))
        self.assertTrue((r == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(r, expect))

    @skipNoDevice
    def test_op_add_scalar_float(self):
        """Test __add__ operation of OffloadArray with scalar operand."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1.0, 4711.0 * 1024, dtype=float)
        s = 1.3

        old = numpy.empty_like(a)
        old[:] = a[:]
        expect = a + s

        offl_a = stream.bind(a)
        offl_r = offl_a + s
        offl_a.update_host()
        r = offl_r.update_host().array
        stream.sync()

        self.assertEqual(r.shape, a.shape)
        self.assertEqual(r.dtype, a.dtype)
        self.assertTrue((a == old).all(),
                        "Input array operand must not be modified: "
                        "{0} should be {1}".format(a, old))
        self.assertTrue((r == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(r, expect))

    @skipNoDevice
    def test_op_add_offload_array_float(self):
        """Test __add__ operation of OffloadArray with OffloadArray operand."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1.0, 4711.0 * 1024, dtype=float)
        o = a + 1.0

        old_a = numpy.empty_like(a)
        old_o = numpy.empty_like(o)
        old_a[:] = a[:]
        old_o[:] = o[:]
        expect = a + o

        offl_a = stream.bind(a)
        offl_o = stream.bind(o)
        offl_r = offl_a + offl_o
        offl_a.update_host()
        r = offl_r.update_host().array
        stream.sync()

        self.assertEqual(r.shape, a.shape)
        self.assertEqual(r.dtype, a.dtype)
        self.assertTrue((a == old_a).all(),
                        "Input array operand 1 must not be modified: "
                        "{0} should be {1}".format(a, old_a))
        self.assertTrue((o == old_o).all(),
                        "Input array operand 2 must not be modified: "
                        "{0} should be {1}".format(o, old_o))
        self.assertTrue((r == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(r, expect))

    @skipNoDevice
    def test_op_add_array_float(self):
        """Test __add__ operation of OffloadArray with array operand."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1.0, 4711.0 * 1024, dtype=float)
        o = a + 1.0

        old_a = numpy.empty_like(a)
        old_o = numpy.empty_like(o)
        old_a[:] = a[:]
        old_o[:] = o[:]
        expect = a + o

        offl_a = stream.bind(a)
        offl_r = offl_a + o
        offl_a.update_host()
        r = offl_r.update_host().array
        stream.sync()

        self.assertEqual(r.shape, a.shape)
        self.assertEqual(r.dtype, a.dtype)
        self.assertTrue((a == old_a).all(),
                        "Input array operand 1 must not be modified: "
                        "{0} should be {1}".format(a, old_a))
        self.assertTrue((o == old_o).all(),
                        "Input array operand 2 must not be modified: "
                        "{0} should be {1}".format(o, old_o))
        self.assertTrue((r == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(r, expect))

    @skipNoDevice
    def test_op_add_scalar_complex(self):
        """Test __add__ operation of OffloadArray with scalar operand."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1.0, 4711.0 * 1024, dtype=complex)
        s = complex(1.2, -1.3)

        old = numpy.empty_like(a)
        old[:] = a[:]
        expect = a + s

        offl_a = stream.bind(a)
        offl_r = offl_a + s
        offl_a.update_host()
        r = offl_r.update_host().array
        stream.sync()

        self.assertEqual(r.shape, a.shape)
        self.assertEqual(r.dtype, a.dtype)
        self.assertTrue((a == old).all(),
                        "Input array operand must not be modified: "
                        "{0} should be {1}".format(a, old))
        self.assertTrue((r == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(r, expect))

    @skipNoDevice
    def test_op_add_offload_array_complex(self):
        """Test __add__ operation of OffloadArray with OffloadArray operand."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1.0, 4711.0 * 1024, dtype=complex)
        o = a + complex(1.0, -1.3)

        old_a = numpy.empty_like(a)
        old_o = numpy.empty_like(o)
        old_a[:] = a[:]
        old_o[:] = o[:]
        expect = a + o

        offl_a = stream.bind(a)
        offl_o = stream.bind(o)
        offl_r = offl_a + offl_o
        offl_a.update_host()
        r = offl_r.update_host().array
        stream.sync()

        self.assertEqual(r.shape, a.shape)
        self.assertEqual(r.dtype, a.dtype)
        self.assertTrue((a == old_a).all(),
                        "Input array operand 1 must not be modified: "
                        "{0} should be {1}".format(a, old_a))
        self.assertTrue((o == old_o).all(),
                        "Input array operand 2 must not be modified: "
                        "{0} should be {1}".format(o, old_o))
        self.assertTrue((r == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(r, expect))

    @skipNoDevice
    def test_op_add_array_complex(self):
        """Test __add__ operation of OffloadArray with array operand."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1.0, 4711.0 * 1024, dtype=complex)
        o = a + complex(1.2, -1.5)

        old_a = numpy.empty_like(a)
        old_o = numpy.empty_like(o)
        old_a[:] = a[:]
        old_o[:] = o[:]
        expect = a + o

        offl_a = stream.bind(a)
        offl_r = offl_a + o
        offl_a.update_host()
        r = offl_r.update_host().array
        stream.sync()

        self.assertEqual(r.shape, a.shape)
        self.assertEqual(r.dtype, a.dtype)
        self.assertTrue((a == old_a).all(),
                        "Input array operand 1 must not be modified: "
                        "{0} should be {1}".format(a, old_a))
        self.assertTrue((o == old_o).all(),
                        "Input array operand 2 must not be modified: "
                        "{0} should be {1}".format(o, old_o))
        self.assertTrue((r == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(r, expect))

    @skipNoDevice
    def test_op_iadd_scalar_int(self):
        """Test __iadd__ operation of OffloadArray with scalar operand."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1, 4711 * 1024, dtype=int)
        s = 1

        old = numpy.empty_like(a)
        old[:] = a[:]
        expect = a + s

        offl_a = stream.bind(a)
        offl_a += s
        offl_a.update_host()
        r = offl_a.array
        stream.sync()

        self.assertTrue((r != old).all(),
                        "Input array operand must be modified: "
                        "{0} should be {1}".format(r, old))
        self.assertTrue((r == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(r, expect))

    @skipNoDevice
    def test_op_iadd_offload_array_int(self):
        """Test __iadd__ operation of OffloadArray with OffloadArray
           operand."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1, 4711 * 1024, dtype=int)
        o = a + 1

        old_a = numpy.empty_like(a)
        old_o = numpy.empty_like(o)
        old_a[:] = a[:]
        old_o[:] = o[:]
        expect = a + o

        offl_a = stream.bind(a)
        offl_o = stream.bind(o)
        offl_a += offl_o
        offl_a.update_host()
        r = offl_a.array
        stream.sync()

        self.assertTrue((r != old_a).all(),
                        "Array operand must be modified: "
                        "{0} should be {1}".format(a, old_a))
        self.assertTrue((o == old_o).all(),
                        "Input array operand 2 must not be modified: "
                        "{0} should be {1}".format(o, old_o))
        self.assertTrue((r == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(r, expect))

    @skipNoDevice
    def test_op_iadd_array_int(self):
        """Test __iadd__ operation of OffloadArray with array operand."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1, 4711 * 1024, dtype=int)
        o = a + 1

        old_a = numpy.empty_like(a)
        old_o = numpy.empty_like(o)
        old_a[:] = a[:]
        old_o[:] = o[:]
        expect = a + o

        offl_a = stream.bind(a)
        offl_o = stream.bind(o)
        offl_a += o
        offl_a.update_host()
        r = offl_a.array
        stream.sync()

        self.assertTrue((r != old_a).all(),
                        "Array operand must be modified: "
                        "{0} should be {1}".format(a, old_a))
        self.assertTrue((o == old_o).all(),
                        "Input array operand 2 must not be modified: "
                        "{0} should be {1}".format(o, old_o))
        self.assertTrue((r == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(r, expect))

    @skipNoDevice
    def test_op_iadd_scalar_float(self):
        """Test __iadd__ operation of OffloadArray with scalar operand."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1, 4711 * 1024, dtype=float)
        s = 1.3

        old = numpy.empty_like(a)
        old[:] = a[:]
        expect = a + s

        offl_a = stream.bind(a)
        offl_a += s
        offl_a.update_host()
        r = offl_a.array
        stream.sync()

        self.assertTrue((r != old).all(),
                        "Input array operand must be modified: "
                        "{0} should be {1}".format(r, old))
        self.assertTrue((r == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(r, expect))

    @skipNoDevice
    def test_op_iadd_offload_array_float(self):
        """Test __iadd__ operation of OffloadArray with OffloadArray
           operand."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1, 4711 * 1024, dtype=float)
        o = a + 1.3

        old_a = numpy.empty_like(a)
        old_o = numpy.empty_like(o)
        old_a[:] = a[:]
        old_o[:] = o[:]
        expect = a + o

        offl_a = stream.bind(a)
        offl_o = stream.bind(o)
        offl_a += offl_o
        offl_a.update_host()
        r = offl_a.array
        stream.sync()

        self.assertTrue((r != old_a).all(),
                        "Array operand must be modified: "
                        "{0} should be {1}".format(a, old_a))
        self.assertTrue((o == old_o).all(),
                        "Input array operand 2 must not be modified: "
                        "{0} should be {1}".format(o, old_o))
        self.assertTrue((r == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(r, expect))

    @skipNoDevice
    def test_op_iadd_array_float(self):
        """Test __iadd__ operation of OffloadArray with array operand."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1, 4711 * 1024, dtype=float)
        o = a + 1.3

        old_a = numpy.empty_like(a)
        old_o = numpy.empty_like(o)
        old_a[:] = a[:]
        old_o[:] = o[:]
        expect = a + o

        offl_a = stream.bind(a)
        offl_o = stream.bind(o)
        offl_a += o
        offl_a.update_host()
        r = offl_a.array
        stream.sync()

        self.assertTrue((r != old_a).all(),
                        "Array operand must be modified: "
                        "{0} should be {1}".format(a, old_a))
        self.assertTrue((o == old_o).all(),
                        "Input array operand 2 must not be modified: "
                        "{0} should be {1}".format(o, old_o))
        self.assertTrue((r == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(r, expect))

    @skipNoDevice
    def test_op_iadd_scalar_complex(self):
        """Test __iadd__ operation of OffloadArray with scalar operand."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1, 4711 * 1024, dtype=complex)
        s = complex(1.2, -1.3)

        old = numpy.empty_like(a)
        old[:] = a[:]
        expect = a + s

        offl_a = stream.bind(a)
        offl_a += s
        offl_a.update_host()
        r = offl_a.array
        stream.sync()

        self.assertTrue((r != old).all(),
                        "Input array operand must be modified: "
                        "{0} should be {1}".format(r, old))
        self.assertTrue((r == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(r, expect))

    @skipNoDevice
    def test_op_iadd_offload_array_complex(self):
        """Test __iadd__ operation of OffloadArray with OffloadArray
           operand."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1, 4711 * 1024, dtype=complex)
        o = a + complex(1.2, -1.3)

        old_a = numpy.empty_like(a)
        old_o = numpy.empty_like(o)
        old_a[:] = a[:]
        old_o[:] = o[:]
        expect = a + o

        offl_a = stream.bind(a)
        offl_o = stream.bind(o)
        offl_a += offl_o
        offl_a.update_host()
        r = offl_a.array
        stream.sync()

        self.assertTrue((r != old_a).all(),
                        "Array operand must be modified: "
                        "{0} should be {1}".format(a, old_a))
        self.assertTrue((o == old_o).all(),
                        "Input array operand 2 must not be modified: "
                        "{0} should be {1}".format(o, old_o))
        self.assertTrue((r == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(r, expect))

    @skipNoDevice
    def test_op_iadd_array_complex(self):
        """Test __iadd__ operation of OffloadArray with array operand."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1, 4711 * 1024, dtype=complex)
        o = a + complex(1.2, -1.3)

        old_a = numpy.empty_like(a)
        old_o = numpy.empty_like(o)
        old_a[:] = a[:]
        old_o[:] = o[:]
        expect = a + o

        offl_a = stream.bind(a)
        offl_o = stream.bind(o)
        offl_a += o
        offl_a.update_host()
        r = offl_a.array
        stream.sync()

        self.assertTrue((r != old_a).all(),
                        "Array operand must be modified: "
                        "{0} should be {1}".format(a, old_a))
        self.assertTrue((o == old_o).all(),
                        "Input array operand 2 must not be modified: "
                        "{0} should be {1}".format(o, old_o))
        self.assertTrue((r == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(r, expect))

    @skipNoDevice
    def test_op_sub_scalar_int(self):
        """Test __sub__ operation of OffloadArray with scalar operand."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1, 4711 * 1024, dtype=int)
        s = 1

        old = numpy.empty_like(a)
        old[:] = a[:]
        expect = a - s

        offl_a = stream.bind(a)
        offl_r = offl_a - s
        offl_a.update_host()
        r = offl_r.update_host().array
        stream.sync()

        self.assertEqual(r.shape, a.shape)
        self.assertEqual(r.dtype, a.dtype)
        self.assertTrue((a == old).all(),
                        "Input array operand must not be modified: "
                        "{0} should be {1}".format(a, old))
        self.assertTrue((r == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(r, expect))

    @skipNoDevice
    def test_op_sub_offload_array_int(self):
        """Test __sub__ operation of OffloadArray with OffloadArray operand."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1, 4711 * 1024, dtype=int)
        o = a + 1

        old_a = numpy.empty_like(a)
        old_o = numpy.empty_like(o)
        old_a[:] = a[:]
        old_o[:] = o[:]
        expect = a - o

        offl_a = stream.bind(a)
        offl_o = stream.bind(o)
        offl_r = offl_a - offl_o
        offl_a.update_host()
        r = offl_r.update_host().array
        stream.sync()

        self.assertEqual(r.shape, a.shape)
        self.assertEqual(r.dtype, a.dtype)
        self.assertTrue((a == old_a).all(),
                        "Input array operand 1 must not be modified: "
                        "{0} should be {1}".format(a, old_a))
        self.assertTrue((o == old_o).all(),
                        "Input array operand 2 must not be modified: "
                        "{0} should be {1}".format(o, old_o))
        self.assertTrue((r == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(r, expect))

    @skipNoDevice
    def test_op_sub_array_int(self):
        """Test __sub__ operation of OffloadArray with array operand."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1, 4711 * 1024, dtype=int)
        o = a + 1

        old_a = numpy.empty_like(a)
        old_o = numpy.empty_like(o)
        old_a[:] = a[:]
        old_o[:] = o[:]
        expect = a - o

        offl_a = stream.bind(a)
        offl_r = offl_a - o
        offl_a.update_host()
        r = offl_r.update_host().array
        stream.sync()

        self.assertEqual(r.shape, a.shape)
        self.assertEqual(r.dtype, a.dtype)
        self.assertTrue((a == old_a).all(),
                        "Input array operand 1 must not be modified: "
                        "{0} should be {1}".format(a, old_a))
        self.assertTrue((o == old_o).all(),
                        "Input array operand 2 must not be modified: "
                        "{0} should be {1}".format(o, old_o))
        self.assertTrue((r == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(r, expect))

    @skipNoDevice
    def test_op_sub_scalar_float(self):
        """Test __sub__ operation of OffloadArray with scalar operand."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1.0, 4711.0 * 1024, dtype=float)
        s = 1.3

        old = numpy.empty_like(a)
        old[:] = a[:]
        expect = a - s

        offl_a = stream.bind(a)
        offl_r = offl_a - s
        offl_a.update_host()
        r = offl_r.update_host().array
        stream.sync()

        self.assertEqual(r.shape, a.shape)
        self.assertEqual(r.dtype, a.dtype)
        self.assertTrue((a == old).all(),
                        "Input array operand must not be modified: "
                        "{0} should be {1}".format(a, old))
        self.assertTrue((r == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(r, expect))

    @skipNoDevice
    def test_op_sub_offload_array_float(self):
        """Test __sub__ operation of OffloadArray with OffloadArray operand."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1.0, 4711.0 * 1024, dtype=float)
        o = a + 1.0

        old_a = numpy.empty_like(a)
        old_o = numpy.empty_like(o)
        old_a[:] = a[:]
        old_o[:] = o[:]
        expect = a - o

        offl_a = stream.bind(a)
        offl_o = stream.bind(o)
        offl_r = offl_a - offl_o
        offl_a.update_host()
        r = offl_r.update_host().array
        stream.sync()

        self.assertEqual(r.shape, a.shape)
        self.assertEqual(r.dtype, a.dtype)
        self.assertTrue((a == old_a).all(),
                        "Input array operand 1 must not be modified: "
                        "{0} should be {1}".format(a, old_a))
        self.assertTrue((o == old_o).all(),
                        "Input array operand 2 must not be modified: "
                        "{0} should be {1}".format(o, old_o))
        self.assertTrue((r == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(r, expect))

    @skipNoDevice
    def test_op_sub_array_float(self):
        """Test __sub__ operation of OffloadArray with array operand."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1.0, 4711.0 * 1024, dtype=float)
        o = a + 1.0

        old_a = numpy.empty_like(a)
        old_o = numpy.empty_like(o)
        old_a[:] = a[:]
        old_o[:] = o[:]
        expect = a - o

        offl_a = stream.bind(a)
        offl_r = offl_a - o
        offl_a.update_host()
        r = offl_r.update_host().array
        stream.sync()

        self.assertEqual(r.shape, a.shape)
        self.assertEqual(r.dtype, a.dtype)
        self.assertTrue((a == old_a).all(),
                        "Input array operand 1 must not be modified: "
                        "{0} should be {1}".format(a, old_a))
        self.assertTrue((o == old_o).all(),
                        "Input array operand 2 must not be modified: "
                        "{0} should be {1}".format(o, old_o))
        self.assertTrue((r == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(r, expect))

    @skipNoDevice
    def test_op_sub_scalar_complex(self):
        """Test __sub__ operation of OffloadArray with scalar operand."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1.0, 4711.0 * 1024, dtype=complex)
        s = complex(1.3, 1.3)

        old = numpy.empty_like(a)
        old[:] = a[:]
        expect = a - s

        offl_a = stream.bind(a)
        offl_r = offl_a - s
        offl_a.update_host()
        r = offl_r.update_host().array
        stream.sync()

        self.assertEqual(r.shape, a.shape)
        self.assertEqual(r.dtype, a.dtype)
        self.assertTrue((a == old).all(),
                        "Input array operand must not be modified: "
                        "{0} should be {1}".format(a, old))
        self.assertTrue((r == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(r, expect))

    @skipNoDevice
    def test_op_sub_offload_array_complex(self):
        """Test __sub__ operation of OffloadArray with OffloadArray operand."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1.0, 4711.0 * 1024, dtype=complex)
        o = a + complex(1.3, -1.7)

        old_a = numpy.empty_like(a)
        old_o = numpy.empty_like(o)
        old_a[:] = a[:]
        old_o[:] = o[:]
        expect = a - o

        offl_a = stream.bind(a)
        offl_o = stream.bind(o)
        offl_r = offl_a - offl_o
        offl_a.update_host()
        r = offl_r.update_host().array
        stream.sync()

        self.assertEqual(r.shape, a.shape)
        self.assertEqual(r.dtype, a.dtype)
        self.assertTrue((a == old_a).all(),
                        "Input array operand 1 must not be modified: "
                        "{0} should be {1}".format(a, old_a))
        self.assertTrue((o == old_o).all(),
                        "Input array operand 2 must not be modified: "
                        "{0} should be {1}".format(o, old_o))
        self.assertTrue((r == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(r, expect))

    @skipNoDevice
    def test_op_sub_array_complex(self):
        """Test __sub__ operation of OffloadArray with array operand."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1.0, 4711.0 * 1024, dtype=complex)
        o = a + complex(1.3, -1.4)

        old_a = numpy.empty_like(a)
        old_o = numpy.empty_like(o)
        old_a[:] = a[:]
        old_o[:] = o[:]
        expect = a - o

        offl_a = stream.bind(a)
        offl_r = offl_a - o
        offl_a.update_host()
        r = offl_r.update_host().array
        stream.sync()

        self.assertEqual(r.shape, a.shape)
        self.assertEqual(r.dtype, a.dtype)
        self.assertTrue((a == old_a).all(),
                        "Input array operand 1 must not be modified: "
                        "{0} should be {1}".format(a, old_a))
        self.assertTrue((o == old_o).all(),
                        "Input array operand 2 must not be modified: "
                        "{0} should be {1}".format(o, old_o))
        self.assertTrue((r == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(r, expect))

    @skipNoDevice
    def test_op_isub_scalar_int(self):
        """Test __isub__ operation of OffloadArray with scalar operand."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1, 4711 * 1024, dtype=int)
        s = 1

        old = numpy.empty_like(a)
        old[:] = a[:]
        expect = a - s

        offl_a = stream.bind(a)
        offl_a -= s
        offl_a.update_host()
        r = offl_a.array
        stream.sync()

        self.assertTrue((r != old).all(),
                        "Input array operand must be modified: "
                        "{0} should be {1}".format(r, old))
        self.assertTrue((r == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(r, expect))

    @skipNoDevice
    def test_op_isub_offload_array_int(self):
        """Test __isub__ operation of OffloadArray with OffloadArray
           operand."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1, 4711 * 1024, dtype=int)
        o = a + 1

        old_a = numpy.empty_like(a)
        old_o = numpy.empty_like(o)
        old_a[:] = a[:]
        old_o[:] = o[:]
        expect = a - o

        offl_a = stream.bind(a)
        offl_o = stream.bind(o)
        offl_a -= offl_o
        offl_a.update_host()
        r = offl_a.array
        stream.sync()

        self.assertTrue((r != old_a).all(),
                        "Array operand must be modified: "
                        "{0} should be {1}".format(a, old_a))
        self.assertTrue((o == old_o).all(),
                        "Input array operand 2 must not be modified: "
                        "{0} should be {1}".format(o, old_o))
        self.assertTrue((r == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(r, expect))

    @skipNoDevice
    def test_op_isub_array_int(self):
        """Test __isub__ operation of OffloadArray with array operand."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1, 4711 * 1024, dtype=int)
        o = a + 1

        old_a = numpy.empty_like(a)
        old_o = numpy.empty_like(o)
        old_a[:] = a[:]
        old_o[:] = o[:]
        expect = a - o

        offl_a = stream.bind(a)
        offl_o = stream.bind(o)
        offl_a -= o
        offl_a.update_host()
        r = offl_a.array
        stream.sync()

        self.assertTrue((r != old_a).all(),
                        "Array operand must be modified: "
                        "{0} should be {1}".format(a, old_a))
        self.assertTrue((o == old_o).all(),
                        "Input array operand 2 must not be modified: "
                        "{0} should be {1}".format(o, old_o))
        self.assertTrue((r == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(r, expect))

    @skipNoDevice
    def test_op_isub_scalar_float(self):
        """Test __isub__ operation of OffloadArray with scalar operand."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1, 4711 * 1024, dtype=float)
        s = 1.3

        old = numpy.empty_like(a)
        old[:] = a[:]
        expect = a - s

        offl_a = stream.bind(a)
        offl_a -= s
        offl_a.update_host()
        r = offl_a.array
        stream.sync()

        self.assertTrue((r != old).all(),
                        "Input array operand must be modified: "
                        "{0} should be {1}".format(r, old))
        self.assertTrue((r == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(r, expect))

    @skipNoDevice
    def test_op_isub_offload_array_float(self):
        """Test __isub__ operation of OffloadArray with OffloadArray
           operand."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1, 4711 * 1024, dtype=float)
        o = a + 1.3

        old_a = numpy.empty_like(a)
        old_o = numpy.empty_like(o)
        old_a[:] = a[:]
        old_o[:] = o[:]
        expect = a - o

        offl_a = stream.bind(a)
        offl_o = stream.bind(o)
        offl_a -= offl_o
        offl_a.update_host()
        r = offl_a.array
        stream.sync()

        self.assertTrue((r != old_a).all(),
                        "Array operand must be modified: "
                        "{0} should be {1}".format(a, old_a))
        self.assertTrue((o == old_o).all(),
                        "Input array operand 2 must not be modified: "
                        "{0} should be {1}".format(o, old_o))
        self.assertTrue((r == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(r, expect))

    @skipNoDevice
    def test_op_isub_array_float(self):
        """Test __isub__ operation of OffloadArray with array operand."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1, 4711 * 1024, dtype=float)
        o = a + 1.3

        old_a = numpy.empty_like(a)
        old_o = numpy.empty_like(o)
        old_a[:] = a[:]
        old_o[:] = o[:]
        expect = a - o

        offl_a = stream.bind(a)
        offl_o = stream.bind(o)
        offl_a -= o
        offl_a.update_host()
        r = offl_a.array
        stream.sync()

        self.assertTrue((r != old_a).all(),
                        "Array operand must be modified: "
                        "{0} should be {1}".format(a, old_a))
        self.assertTrue((o == old_o).all(),
                        "Input array operand 2 must not be modified: "
                        "{0} should be {1}".format(o, old_o))
        self.assertTrue((r == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(r, expect))

    @skipNoDevice
    def test_op_isub_scalar_complex(self):
        """Test __isub__ operation of OffloadArray with scalar operand."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1, 4711 * 1024, dtype=complex)
        s = complex(1.2, -1.3)

        old = numpy.empty_like(a)
        old[:] = a[:]
        expect = a - s

        offl_a = stream.bind(a)
        offl_a -= s
        offl_a.update_host()
        r = offl_a.array
        stream.sync()

        self.assertTrue((r != old).all(),
                        "Input array operand must be modified: "
                        "{0} should be {1}".format(r, old))
        self.assertTrue((r == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(r, expect))

    @skipNoDevice
    def test_op_isub_offload_array_complex(self):
        """Test __isub__ operation of OffloadArray with OffloadArray
           operand."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1, 4711 * 1024, dtype=complex)
        o = a + complex(1.2, -1.3)

        old_a = numpy.empty_like(a)
        old_o = numpy.empty_like(o)
        old_a[:] = a[:]
        old_o[:] = o[:]
        expect = a - o

        offl_a = stream.bind(a)
        offl_o = stream.bind(o)
        offl_a -= offl_o
        offl_a.update_host()
        r = offl_a.array
        stream.sync()

        self.assertTrue((r != old_a).all(),
                        "Array operand must be modified: "
                        "{0} should be {1}".format(a, old_a))
        self.assertTrue((o == old_o).all(),
                        "Input array operand 2 must not be modified: "
                        "{0} should be {1}".format(o, old_o))
        self.assertTrue((r == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(r, expect))

    @skipNoDevice
    def test_op_isub_array_complex(self):
        """Test __isub__ operation of OffloadArray with array operand."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1, 4711 * 1024, dtype=complex)
        o = a + complex(1.2, -1.3)

        old_a = numpy.empty_like(a)
        old_o = numpy.empty_like(o)
        old_a[:] = a[:]
        old_o[:] = o[:]
        expect = a - o

        offl_a = stream.bind(a)
        offl_o = stream.bind(o)
        offl_a -= o
        offl_a.update_host()
        r = offl_a.array
        stream.sync()

        self.assertTrue((r != old_a).all(),
                        "Array operand must be modified: "
                        "{0} should be {1}".format(a, old_a))
        self.assertTrue((o == old_o).all(),
                        "Input array operand 2 must not be modified: "
                        "{0} should be {1}".format(o, old_o))
        self.assertTrue((r == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(r, expect))

    @skipNoDevice
    def test_op_mul_scalar_int(self):
        """Test __mul__ operation of OffloadArray with scalar operand."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1, 4711 * 1024, dtype=int)
        s = 1

        old = numpy.empty_like(a)
        old[:] = a[:]
        expect = a * s

        offl_a = stream.bind(a)
        offl_r = offl_a * s
        offl_a.update_host()
        r = offl_r.update_host().array
        stream.sync()

        self.assertEqual(r.shape, a.shape)
        self.assertEqual(r.dtype, a.dtype)
        self.assertTrue((a == old).all(),
                        "Input array operand must not be modified: "
                        "{0} should be {1}".format(a, old))
        self.assertTrue((r == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(r, expect))

    @skipNoDevice
    def test_op_mul_offload_array_int(self):
        """Test __mul__ operation of OffloadArray with OffloadArray operand."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1, 4711 * 1024, dtype=int)
        o = a + 1

        old_a = numpy.empty_like(a)
        old_o = numpy.empty_like(o)
        old_a[:] = a[:]
        old_o[:] = o[:]
        expect = a * o

        offl_a = stream.bind(a)
        offl_o = stream.bind(o)
        offl_r = offl_a * offl_o
        offl_a.update_host()
        r = offl_r.update_host().array
        stream.sync()

        self.assertEqual(r.shape, a.shape)
        self.assertEqual(r.dtype, a.dtype)
        self.assertTrue((a == old_a).all(),
                        "Input array operand 1 must not be modified: "
                        "{0} should be {1}".format(a, old_a))
        self.assertTrue((o == old_o).all(),
                        "Input array operand 2 must not be modified: "
                        "{0} should be {1}".format(o, old_o))
        self.assertTrue((r == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(r, expect))

    @skipNoDevice
    def test_op_mul_array_int(self):
        """Test __mul__ operation of OffloadArray with array operand."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1, 4711 * 1024, dtype=int)
        o = a + 1

        old_a = numpy.empty_like(a)
        old_o = numpy.empty_like(o)
        old_a[:] = a[:]
        old_o[:] = o[:]
        expect = a * o

        offl_a = stream.bind(a)
        offl_r = offl_a * o
        offl_a.update_host()
        r = offl_r.update_host().array
        stream.sync()

        self.assertEqual(r.shape, a.shape)
        self.assertEqual(r.dtype, a.dtype)
        self.assertTrue((a == old_a).all(),
                        "Input array operand 1 must not be modified: "
                        "{0} should be {1}".format(a, old_a))
        self.assertTrue((o == old_o).all(),
                        "Input array operand 2 must not be modified: "
                        "{0} should be {1}".format(o, old_o))
        self.assertTrue((r == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(r, expect))

    @skipNoDevice
    def test_op_mul_scalar_float(self):
        """Test __mul__ operation of OffloadArray with scalar operand."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1.0, 4711.0 * 1024, dtype=float)
        s = 1.3

        old = numpy.empty_like(a)
        old[:] = a[:]
        expect = a * s

        offl_a = stream.bind(a)
        offl_r = offl_a * s
        offl_a.update_host()
        r = offl_r.update_host().array
        stream.sync()

        self.assertEqual(r.shape, a.shape)
        self.assertEqual(r.dtype, a.dtype)
        self.assertTrue((a == old).all(),
                        "Input array operand must not be modified: "
                        "{0} should be {1}".format(a, old))
        self.assertTrue((r == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(r, expect))

    @skipNoDevice
    def test_op_mul_offload_array_float(self):
        """Test __mul__ operation of OffloadArray with OffloadArray operand."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1.0, 4711.0 * 1024, dtype=float)
        o = a + 1.0

        old_a = numpy.empty_like(a)
        old_o = numpy.empty_like(o)
        old_a[:] = a[:]
        old_o[:] = o[:]
        expect = a * o

        offl_a = stream.bind(a)
        offl_o = stream.bind(o)
        offl_r = offl_a * offl_o
        offl_a.update_host()
        r = offl_r.update_host().array
        stream.sync()

        self.assertEqual(r.shape, a.shape)
        self.assertEqual(r.dtype, a.dtype)
        self.assertTrue((a == old_a).all(),
                        "Input array operand 1 must not be modified: "
                        "{0} should be {1}".format(a, old_a))
        self.assertTrue((o == old_o).all(),
                        "Input array operand 2 must not be modified: "
                        "{0} should be {1}".format(o, old_o))
        self.assertTrue((r == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(r, expect))

    @skipNoDevice
    def test_op_mul_array_float(self):
        """Test __mul__ operation of OffloadArray with array operand."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1.0, 4711.0 * 1024, dtype=float)
        o = a + 1.0

        old_a = numpy.empty_like(a)
        old_o = numpy.empty_like(o)
        old_a[:] = a[:]
        old_o[:] = o[:]
        expect = a * o

        offl_a = stream.bind(a)
        offl_r = offl_a * o
        offl_a.update_host()
        r = offl_r.update_host().array
        stream.sync()

        self.assertEqual(r.shape, a.shape)
        self.assertEqual(r.dtype, a.dtype)
        self.assertTrue((a == old_a).all(),
                        "Input array operand 1 must not be modified: "
                        "{0} should be {1}".format(a, old_a))
        self.assertTrue((o == old_o).all(),
                        "Input array operand 2 must not be modified: "
                        "{0} should be {1}".format(o, old_o))
        self.assertTrue((r == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(r, expect))

    @skipNoDevice
    def test_op_mul_scalar_complex(self):
        """Test __mul__ operation of OffloadArray with scalar operand."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1.0, 4711.0 * 1024, dtype=complex)
        s = complex(1.3, 1.4)

        old = numpy.empty_like(a)
        old[:] = a[:]
        expect = a * s

        offl_a = stream.bind(a)
        offl_r = offl_a * s
        offl_a.update_host()
        r = offl_r.update_host().array
        stream.sync()

        self.assertEqual(r.shape, a.shape)
        self.assertEqual(r.dtype, a.dtype)
        self.assertTrue((a == old).all(),
                        "Input array operand must not be modified: "
                        "{0} should be {1}".format(a, old))
        self.assertTrue((r == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(r, expect))

    @skipNoDevice
    def test_op_mul_offload_array_complex(self):
        """Test __mul__ operation of OffloadArray with OffloadArray operand."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1.0, 4711.0 * 1024, dtype=complex)
        o = a + complex(-1.7, +1.7)

        old_a = numpy.empty_like(a)
        old_o = numpy.empty_like(o)
        old_a[:] = a[:]
        old_o[:] = o[:]
        expect = a * o

        offl_a = stream.bind(a)
        offl_o = stream.bind(o)
        offl_r = offl_a * offl_o
        offl_a.update_host()
        r = offl_r.update_host().array
        stream.sync()

        self.assertEqual(r.shape, a.shape)
        self.assertEqual(r.dtype, a.dtype)
        self.assertTrue((a == old_a).all(),
                        "Input array operand 1 must not be modified: "
                        "{0} should be {1}".format(a, old_a))
        self.assertTrue((o == old_o).all(),
                        "Input array operand 2 must not be modified: "
                        "{0} should be {1}".format(o, old_o))
        self.assertTrue((r == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(r, expect))

    @skipNoDevice
    def test_op_mul_array_complex(self):
        """Test __mul__ operation of OffloadArray with array operand."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1.0, 4711.0 * 1024, dtype=complex)
        o = a + complex(-1.6, -2.2)

        old_a = numpy.empty_like(a)
        old_o = numpy.empty_like(o)
        old_a[:] = a[:]
        old_o[:] = o[:]
        expect = a * o

        offl_a = stream.bind(a)
        offl_r = offl_a * o
        offl_a.update_host()
        r = offl_r.update_host().array
        stream.sync()

        self.assertEqual(r.shape, a.shape)
        self.assertEqual(r.dtype, a.dtype)
        self.assertTrue((a == old_a).all(),
                        "Input array operand 1 must not be modified: "
                        "{0} should be {1}".format(a, old_a))
        self.assertTrue((o == old_o).all(),
                        "Input array operand 2 must not be modified: "
                        "{0} should be {1}".format(o, old_o))
        self.assertTrue((r == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(r, expect))

    @skipNoDevice
    def test_op_imul_scalar_int(self):
        """Test __imul__ operation of OffloadArray with scalar operand."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1, 4711 * 1024, dtype=int)
        s = 2

        old = numpy.empty_like(a)
        old[:] = a[:]
        expect = a * s

        offl_a = stream.bind(a)
        offl_a *= s
        offl_a.update_host()
        r = offl_a.array
        stream.sync()

        self.assertTrue((r != old).all(),
                        "Input array operand must be modified: "
                        "{0} should be {1}".format(r, old))
        self.assertTrue((r == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(r, expect))

    @skipNoDevice
    def test_op_imul_offload_array_int(self):
        """Test __imul__ operation of OffloadArray with OffloadArray
           operand."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1, 4711 * 1024, dtype=int)
        o = a + 1

        old_a = numpy.empty_like(a)
        old_o = numpy.empty_like(o)
        old_a[:] = a[:]
        old_o[:] = o[:]
        expect = a * o

        offl_a = stream.bind(a)
        offl_o = stream.bind(o)
        offl_a *= offl_o
        offl_a.update_host()
        r = offl_a.array
        stream.sync()

        self.assertTrue((r != old_a).all(),
                        "Array operand must be modified: "
                        "{0} should be {1}".format(a, old_a))
        self.assertTrue((o == old_o).all(),
                        "Input array operand 2 must not be modified: "
                        "{0} should be {1}".format(o, old_o))
        self.assertTrue((r == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(r, expect))

    @skipNoDevice
    def test_op_imul_array_int(self):
        """Test __imul__ operation of OffloadArray with array operand."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1, 4711 * 1024, dtype=int)
        o = a + 1

        old_a = numpy.empty_like(a)
        old_o = numpy.empty_like(o)
        old_a[:] = a[:]
        old_o[:] = o[:]
        expect = a * o

        offl_a = stream.bind(a)
        offl_o = stream.bind(o)
        offl_a *= o
        offl_a.update_host()
        r = offl_a.array
        stream.sync()

        self.assertTrue((r != old_a).all(),
                        "Array operand must be modified: "
                        "{0} should be {1}".format(a, old_a))
        self.assertTrue((o == old_o).all(),
                        "Input array operand 2 must not be modified: "
                        "{0} should be {1}".format(o, old_o))
        self.assertTrue((r == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(r, expect))

    @skipNoDevice
    def test_op_imul_scalar_float(self):
        """Test __imul__ operation of OffloadArray with scalar operand."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1, 4711 * 1024, dtype=float)
        s = 1.3

        old = numpy.empty_like(a)
        old[:] = a[:]
        expect = a * s

        offl_a = stream.bind(a)
        offl_a *= s
        offl_a.update_host()
        r = offl_a.array
        stream.sync()

        self.assertTrue((r != old).all(),
                        "Input array operand must be modified: "
                        "{0} should be {1}".format(r, old))
        self.assertTrue((r == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(r, expect))

    @skipNoDevice
    def test_op_imul_offload_array_float(self):
        """Test __imul__ operation of OffloadArray with OffloadArray
           operand."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1, 4711 * 1024, dtype=float)
        o = a + 1.3

        old_a = numpy.empty_like(a)
        old_o = numpy.empty_like(o)
        old_a[:] = a[:]
        old_o[:] = o[:]
        expect = a * o

        offl_a = stream.bind(a)
        offl_o = stream.bind(o)
        offl_a *= offl_o
        offl_a.update_host()
        r = offl_a.array
        stream.sync()

        self.assertTrue((r != old_a).all(),
                        "Array operand must be modified: "
                        "{0} should be {1}".format(a, old_a))
        self.assertTrue((o == old_o).all(),
                        "Input array operand 2 must not be modified: "
                        "{0} should be {1}".format(o, old_o))
        self.assertTrue((r == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(r, expect))

    @skipNoDevice
    def test_op_imul_array_float(self):
        """Test __imul__ operation of OffloadArray with array operand."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1, 4711 * 1024, dtype=float)
        o = a + 1.3

        old_a = numpy.empty_like(a)
        old_o = numpy.empty_like(o)
        old_a[:] = a[:]
        old_o[:] = o[:]
        expect = a * o

        offl_a = stream.bind(a)
        offl_o = stream.bind(o)
        offl_a *= o
        offl_a.update_host()
        r = offl_a.array
        stream.sync()

        self.assertTrue((r != old_a).all(),
                        "Array operand must be modified: "
                        "{0} should be {1}".format(a, old_a))
        self.assertTrue((o == old_o).all(),
                        "Input array operand 2 must not be modified: "
                        "{0} should be {1}".format(o, old_o))
        self.assertTrue((r == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(r, expect))

    @skipNoDevice
    def test_op_imul_scalar_complex(self):
        """Test __imul__ operation of OffloadArray with scalar operand."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1, 4711 * 1024, dtype=complex)
        s = complex(1.2, -1.3)

        old = numpy.empty_like(a)
        old[:] = a[:]
        expect = a * s

        offl_a = stream.bind(a)
        offl_a *= s
        offl_a.update_host()
        r = offl_a.array
        stream.sync()

        self.assertTrue((r != old).all(),
                        "Input array operand must be modified: "
                        "{0} should be {1}".format(r, old))
        self.assertTrue((r == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(r, expect))

    @skipNoDevice
    def test_op_imul_offload_array_complex(self):
        """Test __imul__ operation of OffloadArray with OffloadArray
           operand."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1, 4711 * 1024, dtype=complex)
        o = a + complex(1.2, -1.3)

        old_a = numpy.empty_like(a)
        old_o = numpy.empty_like(o)
        old_a[:] = a[:]
        old_o[:] = o[:]
        expect = a * o

        offl_a = stream.bind(a)
        offl_o = stream.bind(o)
        offl_a *= offl_o
        offl_a.update_host()
        r = offl_a.array
        stream.sync()

        self.assertTrue((r != old_a).all(),
                        "Array operand must be modified: "
                        "{0} should be {1}".format(a, old_a))
        self.assertTrue((o == old_o).all(),
                        "Input array operand 2 must not be modified: "
                        "{0} should be {1}".format(o, old_o))
        self.assertTrue((r == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(r, expect))

    @skipNoDevice
    def test_op_imul_array_complex(self):
        """Test __imul__ operation of OffloadArray with array operand."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1, 4711 * 1024, dtype=complex)
        o = a + complex(1.2, -1.3)

        old_a = numpy.empty_like(a)
        old_o = numpy.empty_like(o)
        old_a[:] = a[:]
        old_o[:] = o[:]
        expect = a * o

        offl_a = stream.bind(a)
        offl_o = stream.bind(o)
        offl_a *= o
        offl_a.update_host()
        r = offl_a.array
        stream.sync()

        self.assertTrue((r != old_a).all(),
                        "Array operand must be modified: "
                        "{0} should be {1}".format(a, old_a))
        self.assertTrue((o == old_o).all(),
                        "Input array operand 2 must not be modified: "
                        "{0} should be {1}".format(o, old_o))
        self.assertTrue((r == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(r, expect))

    @skipNoDevice
    def test_op_pow_scalar_int(self):
        """Test __pow__ operation of OffloadArray with scalar operand."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1, 4711 * 1024, dtype=int)
        s = 2

        old = numpy.empty_like(a)
        old[:] = a[:]
        cutoff = numpy.empty_like(a)
        cutoff[:] = cutoff_value
        expect = numpy.minimum(pow(a, s), cutoff)

        offl_a = stream.bind(a)
        offl_r = pow(offl_a, s)
        r = offl_r.update_host().array
        stream.sync()
        r = numpy.minimum(r, cutoff)

        self.assertEqual(r.shape, a.shape)
        self.assertEqual(r.dtype, a.dtype)
        self.assertTrue((a == old).all(),
                        "Input array operand must not be modified: "
                        "{0} should be {1}".format(a, old))
        self.assertTrue((r == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(r, expect))

    @skipNoDevice
    def test_op_pow_offload_array_int(self):
        """Test __pow__ operation of OffloadArray with OffloadArray operand."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1.0, 4711.0 * 1024, dtype=int)
        o = numpy.empty_like(a)
        o[:] = 2

        old_a = numpy.empty_like(a)
        old_o = numpy.empty_like(o)
        old_a[:] = a[:]
        old_o[:] = o[:]
        cutoff = numpy.empty_like(a)
        cutoff[:] = cutoff_value
        expect = numpy.minimum(pow(a, o), cutoff)

        offl_a = stream.bind(a)
        offl_o = stream.bind(o)
        offl_r = pow(offl_a, offl_o)
        r = offl_r.update_host().array
        stream.sync()
        r = numpy.minimum(r, cutoff)

        self.assertEqual(r.shape, a.shape)
        self.assertEqual(r.dtype, a.dtype)
        self.assertTrue((a == old_a).all(),
                        "Input array operand 1 must not be modified: "
                        "{0} should be {1}".format(a, old_a))
        self.assertTrue((o == old_o).all(),
                        "Input array operand 2 must not be modified: "
                        "{0} should be {1}".format(o, old_o))
        self.assertTrue((r == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(r, expect))

    @skipNoDevice
    def test_op_pow_array_int(self):
        """Test __pow__ operation of OffloadArray with array operand."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1.0, 4711.0 * 1024, dtype=int)
        o = numpy.empty_like(a)
        o[:] = 2

        old_a = numpy.empty_like(a)
        old_o = numpy.empty_like(o)
        old_a[:] = a[:]
        old_o[:] = o[:]
        cutoff = numpy.empty_like(a)
        cutoff[:] = cutoff_value
        expect = numpy.minimum(pow(a, o), cutoff)

        offl_a = stream.bind(a)
        offl_r = pow(offl_a, o)
        r = offl_r.update_host().array
        stream.sync()
        r = numpy.minimum(r, cutoff)

        self.assertEqual(r.shape, a.shape)
        self.assertEqual(r.dtype, a.dtype)
        self.assertTrue((a == old_a).all(),
                        "Input array operand 1 must not be modified: "
                        "{0} should be {1}".format(a, old_a))
        self.assertTrue((o == old_o).all(),
                        "Input array operand 2 must not be modified: "
                        "{0} should be {1}".format(o, old_o))
        self.assertTrue((r == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(r, expect))

    @skipNoDevice
    def test_op_pow_scalar_float(self):
        """Test __pow__ operation of OffloadArray with scalar operand."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1.0, 4711.0 * 1024, dtype=float)
        s = 0.7

        old = numpy.empty_like(a)
        old[:] = a[:]
        expect = pow(a, s)

        offl_a = stream.bind(a)
        offl_r = pow(offl_a, s)
        r = offl_r.update_host().array
        stream.sync()

        self.assertEqual(r.shape, a.shape)
        self.assertEqual(r.dtype, a.dtype)
        self.assertTrue((a == old).all(),
                        "Input array operand must not be modified: "
                        "{0} should be {1}".format(a, old))
        self.assertEqualEpsilon(r, expect,
                                "Array contains unexpected values: "
                                "{0} should be {1}".format(r, expect))

    @skipNoDevice
    def test_op_pow_offload_array_float(self):
        """Test __pow__ operation of OffloadArray with OffloadArray operand."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1.0, 4711.0 * 1024, dtype=float)
        o = 1 / a

        old_a = numpy.empty_like(a)
        old_o = numpy.empty_like(o)
        old_a[:] = a[:]
        old_o[:] = o[:]
        expect = pow(a, o)

        offl_a = stream.bind(a)
        offl_o = stream.bind(o)
        offl_r = pow(offl_a, offl_o)
        r = offl_r.update_host().array
        stream.sync()

        self.assertEqual(r.shape, a.shape)
        self.assertEqual(r.dtype, a.dtype)
        self.assertTrue((a == old_a).all(),
                        "Input array operand 1 must not be modified: "
                        "{0} should be {1}".format(a, old_a))
        self.assertTrue((o == old_o).all(),
                        "Input array operand 2 must not be modified: "
                        "{0} should be {1}".format(o, old_o))
        self.assertEqualEpsilon(r, expect,
                                "Array contains unexpected values: "
                                "{0} should be {1}".format(r, expect))

    @skipNoDevice
    def test_op_pow_array_float(self):
        """Test __pow__ operation of OffloadArray with array operand."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1.0, 4711.0 * 1024, dtype=float)
        o = 1 / a

        old_a = numpy.empty_like(a)
        old_o = numpy.empty_like(o)
        old_a[:] = a[:]
        old_o[:] = o[:]
        expect = pow(a, o)

        offl_a = stream.bind(a)
        offl_r = pow(offl_a, o)
        r = offl_r.update_host().array
        stream.sync()

        self.assertEqual(r.shape, a.shape)
        self.assertEqual(r.dtype, a.dtype)
        self.assertTrue((a == old_a).all(),
                        "Input array operand 1 must not be modified: "
                        "{0} should be {1}".format(a, old_a))
        self.assertTrue((o == old_o).all(),
                        "Input array operand 2 must not be modified: "
                        "{0} should be {1}".format(o, old_o))
        self.assertEqualEpsilon(r, expect,
                                "Array contains unexpected values: "
                                "{0} should be {1}".format(r, expect))

    @skipNoDevice
    def test_op_pow_scalar_complex(self):
        """Test __pow__ operation of OffloadArray with scalar operand."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1.0, 4711.0 * 1024, dtype=complex)
        s = complex(0.7, 0.6)

        old = numpy.empty_like(a)
        old[:] = a[:]
        expect = pow(a, s)

        offl_a = stream.bind(a)
        offl_r = pow(offl_a, s)
        r = offl_r.update_host().array
        stream.sync()

        self.assertEqual(r.shape, a.shape)
        self.assertEqual(r.dtype, a.dtype)
        self.assertTrue((a == old).all(),
                        "Input array operand must not be modified: "
                        "{0} should be {1}".format(a, old))
        self.assertEqualEpsilon(r, expect,
                                "Array contains unexpected values: "
                                "{0} should be {1}".format(r, expect))

    @skipNoDevice
    def test_op_pow_offload_array_complex(self):
        """Test __pow__ operation of OffloadArray with OffloadArray operand."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1.0, 4711.0 * 1024, dtype=complex)
        o = 1 / a

        old_a = numpy.empty_like(a)
        old_o = numpy.empty_like(o)
        old_a[:] = a[:]
        old_o[:] = o[:]
        expect = pow(a, o)

        offl_a = stream.bind(a)
        offl_o = stream.bind(o)
        offl_r = pow(offl_a, offl_o)
        r = offl_r.update_host().array
        stream.sync()

        self.assertEqual(r.shape, a.shape)
        self.assertEqual(r.dtype, a.dtype)
        self.assertTrue((a == old_a).all(),
                        "Input array operand 1 must not be modified: "
                        "{0} should be {1}".format(a, old_a))
        self.assertTrue((o == old_o).all(),
                        "Input array operand 2 must not be modified: "
                        "{0} should be {1}".format(o, old_o))
        self.assertEqualEpsilon(r, expect,
                                "Array contains unexpected values: "
                                "{0} should be {1}".format(r, expect))

    @skipNoDevice
    def test_op_pow_array_complex(self):
        """Test __pow__ operation of OffloadArray with array operand."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1.0, 4711.0 * 1024, dtype=complex)
        o = 1 / a

        old_a = numpy.empty_like(a)
        old_o = numpy.empty_like(o)
        old_a[:] = a[:]
        old_o[:] = o[:]
        expect = pow(a, o)

        offl_a = stream.bind(a)
        offl_r = pow(offl_a, o)
        r = offl_r.update_host().array
        stream.sync()

        self.assertEqual(r.shape, a.shape)
        self.assertEqual(r.dtype, a.dtype)
        self.assertTrue((a == old_a).all(),
                        "Input array operand 1 must not be modified: "
                        "{0} should be {1}".format(a, old_a))
        self.assertTrue((o == old_o).all(),
                        "Input array operand 2 must not be modified: "
                        "{0} should be {1}".format(o, old_o))
        self.assertEqualEpsilon(r, expect,
                                "Array contains unexpected values: "
                                "{0} should be {1}".format(r, expect))

    @skipNoDevice
    def test_op_abs_offload_array_int(self):
        """Test __abs__ operation of OffloadArray."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1, 4711 * 1024, dtype=int)
        a[:] = a[:] * -1

        old_a = numpy.empty_like(a)
        old_a[:] = a[:]
        expect = abs(a)

        offl_a = stream.bind(a)
        offl_r = abs(offl_a)
        r = offl_r.update_host().array
        stream.sync()

        self.assertTrue((a == old_a).all(),
                        "Input array operand 1 must not be modified: "
                        "{0} should be {1}".format(a, old_a))
        self.assertTrue((r == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(r, expect))

    @skipNoDevice
    def test_op_abs_offload_array_float(self):
        """Test __abs__ operation of OffloadArray."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1.0, 4711.0 * 1024, dtype=float)
        a[:] = a[:] * -1.0

        old_a = numpy.empty_like(a)
        old_a[:] = a[:]
        expect = abs(a)

        offl_a = stream.bind(a)
        offl_r = abs(offl_a)
        r = offl_r.update_host().array
        stream.sync()

        self.assertTrue((a == old_a).all(),
                        "Input array operand 1 must not be modified: "
                        "{0} should be {1}".format(a, old_a))
        self.assertTrue((r == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(r, expect))

    @skipNoDevice
    def test_op_abs_offload_array_complex(self):
        """Test __abs__ operation of OffloadArray."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1.0, 4711.0 * 1024, dtype=complex)

        old_a = numpy.empty_like(a)
        old_a[:] = a[:]
        expect = abs(a)

        offl_a = stream.bind(a)
        offl_r = abs(offl_a)
        r = offl_r.update_host().array
        stream.sync()

        self.assertTrue((a == old_a).all(),
                        "Input array operand 1 must not be modified: "
                        "{0} should be {1}".format(a, old_a))
        self.assertTrue((r == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(r, expect))

    @skipNoDevice
    def test_op_zero_int(self):
        """Test zero operation of OffloadArray with integer data."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1, 4711 * 1024, dtype=int)
        offl_a = stream.bind(a)
        offl_a.zero()
        offl_a.update_host()
        stream.sync()
        self.assertEqual(sum(a), 0,
                         "Array should be all zeros.")

    @skipNoDevice
    def test_op_zero_float(self):
        """Test zero operation of OffloadArray with float data."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1.0, 4711.0 * 1024, dtype=float)
        offl_a = stream.bind(a)
        offl_a.zero()
        offl_a.update_host()
        stream.sync()
        self.assertEqual(sum(a), 0.0,
                         "Array should be all zeros.")

    @skipNoDevice
    def test_op_zero_complex(self):
        """Test zero operation of OffloadArray with complex data."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1.0, 4711.0 * 1024, dtype=complex)
        offl_a = stream.bind(a)
        offl_a.zero()
        offl_a.update_host()
        stream.sync()
        self.assertEqual(sum(a), 0.0 + 0.0j,
                         "Array should be all zeros.")

    @skipNoDevice
    def test_op_one_int(self):
        """Test one operation of OffloadArray with integer data."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1, 4711 * 1024, dtype=int)
        offl_a = stream.bind(a)
        offl_a.one()
        offl_a.update_host()
        stream.sync()
        self.assertTrue((a == 1).all(),
                        "Array should be all one." + str(a))

    @skipNoDevice
    def test_op_one_float(self):
        """Test one operation of OffloadArray with float data."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1.0, 4711.0 * 1024, dtype=float)
        offl_a = stream.bind(a)
        offl_a.one()
        offl_a.update_host()
        stream.sync()
        self.assertTrue((a == 1.0).all(),
                        "Array should be all one." + str(a))

    @skipNoDevice
    def test_op_one_complex(self):
        """Test one operation of OffloadArray with float data."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1.0, 4711.0 * 1024, dtype=complex)
        offl_a = stream.bind(a)
        offl_a.one()
        offl_a.update_host()
        stream.sync()
        self.assertTrue((a == complex(1.0, 0.0)).all(),
                        "Array should be all one." + str(a))

    @skipNoDevice
    def test_op_fill_int(self):
        """Test fill operation of OffloadArray with integer data."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        value = 73422137
        a = numpy.arange(1, 4711 * 1024, dtype=int)
        expect = numpy.empty_like(a)
        expect[:] = value
        offl_a = stream.bind(a)
        offl_a.fill(value)
        offl_a.update_host()
        stream.sync()
        self.assertTrue((a == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(a, expect))

    @skipNoDevice
    def test_op_fill_float(self):
        """Test fill operation of OffloadArray with float data."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        value = 7342.2137
        a = numpy.arange(1.0, 4711.0 * 1024, dtype=float)
        expect = numpy.empty_like(a)
        expect[:] = value
        offl_a = stream.bind(a)
        offl_a.fill(value)
        offl_a.update_host()
        stream.sync()
        self.assertTrue((a == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(a, expect))

    @skipNoDevice
    def test_op_fill_complex(self):
        """Test fill operation of OffloadArray with complex data."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        value = complex(7342.0, -2137.0)
        a = numpy.arange(1.0, 4711.0 * 1024, dtype=complex)
        expect = numpy.empty_like(a)
        expect[:] = value
        offl_a = stream.bind(a)
        offl_a.fill(value)
        offl_a.update_host()
        stream.sync()
        self.assertTrue((a == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(a, expect))

    @skipNoDevice
    def test_op_fillfrom_int(self):
        """Test fillform operation of OffloadArray."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1, 4711 * 1024, dtype=int)
        offl_r = stream.empty_like(a)
        offl_r.fillfrom(a)
        r = offl_r.update_host().array
        stream.sync()
        self.assertTrue((a == r).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(a, r))

    @skipNoDevice
    def test_op_fillfrom_float(self):
        """Test fillform operation of OffloadArray."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1.0, 4711.0 * 1024, dtype=float)
        offl_r = stream.empty_like(a)
        offl_r.fillfrom(a)
        r = offl_r.update_host().array
        stream.sync()
        self.assertTrue((a == r).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(a, r))

    @skipNoDevice
    def test_op_fillfrom_complex(self):
        """Test fillform operation of OffloadArray."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1.0, 4711.0 * 1024, dtype=complex)
        offl_r = stream.empty_like(a)
        offl_r.fillfrom(a)
        offl_r.update_host()
        stream.sync()
        r = offl_r.array
        self.assertTrue((a == r).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(a, r))

    @skipNoDevice
    def test_op_reverse_int(self):
        """Test reverse operation of OffloadArray."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1, 4711 * 1024, dtype=int)
        old_a = numpy.empty_like(a)
        old_a[:] = a[:]
        expect = numpy.array(a[::-1])
        offl_a = stream.bind(a)
        offl_r = offl_a.reverse()
        r = offl_r.update_host().array
        stream.sync()

        self.assertTrue((a == old_a).all(),
                        "Input array operand 1 must not be modified: "
                        "{0} should be {1}".format(a, old_a))
        self.assertTrue((r == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(a, r))

    @skipNoDevice
    def test_op_reverse_float(self):
        """Test reverse operation of OffloadArray."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1.0, 4711.0 * 1024, dtype=float)
        old_a = numpy.empty_like(a)
        old_a[:] = a[:]
        expect = numpy.array(a[::-1])

        offl_a = stream.bind(a)
        offl_r = offl_a.reverse()
        r = offl_r.update_host().array
        stream.sync()

        self.assertTrue((a == old_a).all(),
                        "Input array operand 1 must not be modified: "
                        "{0} should be {1}".format(a, old_a))
        self.assertTrue((r == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(a, r))

    @skipNoDevice
    def test_op_reverse_complex(self):
        """Test reverse operation of OffloadArray."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1.0, 4711.0 * 1024, dtype=complex)
        old_a = numpy.empty_like(a)
        old_a[:] = a[:]
        expect = numpy.array(a[::-1])

        offl_a = stream.bind(a)
        offl_r = offl_a.reverse()
        offl_r.update_host()
        stream.sync()
        r = offl_r.array

        self.assertTrue((a == old_a).all(),
                        "Input array operand 1 must not be modified: "
                        "{0} should be {1}".format(a, old_a))
        self.assertTrue((r == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(a, r))

    @skipNoDevice
    def test_op_setslice_scalar_int(self):
        """Test __setslice__ operation of OffloadArray."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1, 4711 * 1024, dtype=int)
        b = -1

        expect = numpy.empty_like(a)
        expect[:] = b

        offl_a = stream.bind(a)
        offl_a[:] = b
        offl_a.update_host()
        stream.sync()

        self.assertTrue((a == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(a, expect))

    @skipNoDevice
    def test_op_setslice_scalar_float(self):
        """Test __setslice__ operation of OffloadArray."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1, 4711 * 1024, dtype=float)
        b = 2.5

        expect = numpy.empty_like(a)
        expect[:] = b

        offl_a = stream.bind(a)
        offl_a[:] = b
        offl_a.update_host()
        stream.sync()

        self.assertTrue((a == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(a, expect))

    @skipNoDevice
    def test_op_setslice_scalar_complex(self):
        """Test __setslice__ operation of OffloadArray."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1, 4711 * 1024, dtype=complex)
        b = 1.0 + 1.7j

        expect = numpy.empty_like(a)
        expect[:] = b

        offl_a = stream.bind(a)
        offl_a[:] = b
        offl_a.update_host()
        stream.sync()

        self.assertTrue((a == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(a, expect))

    @skipNoDevice
    def test_op_setslice_array_int(self):
        """Test __setslice__ operation of OffloadArray."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1, 4711 * 1024, dtype=int)
        b = a * -1
        expect = numpy.empty_like(b)
        old_b = numpy.empty_like(b)
        expect[:] = b[:]
        old_b[:] = b[:]

        offl_a = stream.bind(a)
        offl_a[:] = b
        offl_a.update_host()
        stream.sync()

        self.assertTrue((b == old_b).all(),
                        "Input array operand 1 must not be modified: "
                        "{0} should be {1}".format(a, old_b))
        self.assertTrue((a == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(a, expect))

    @skipNoDevice
    def test_op_setslice_array_float(self):
        """Test __setslice__ operation of OffloadArray."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1, 4711 * 1024, dtype=float)
        b = a + 2.5
        expect = numpy.empty_like(b)
        old_b = numpy.empty_like(b)
        expect[:] = b[:]
        old_b[:] = b[:]

        offl_a = stream.bind(a)
        offl_a[:] = b
        offl_a.update_host()
        stream.sync()

        self.assertTrue((b == old_b).all(),
                        "Input array operand 1 must not be modified: "
                        "{0} should be {1}".format(a, old_b))
        self.assertTrue((a == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(a, expect))

    @skipNoDevice
    def test_op_setslice_array_complex(self):
        """Test __setslice__ operation of OffloadArray."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1, 4711 * 1024, dtype=complex)
        b = a + 2.5j
        expect = numpy.empty_like(b)
        old_b = numpy.empty_like(b)
        expect[:] = b[:]
        old_b[:] = b[:]

        offl_a = stream.bind(a)
        offl_a[:] = b
        offl_a.update_host()
        stream.sync()

        self.assertTrue((b == old_b).all(),
                        "Input array operand 1 must not be modified: "
                        "{0} should be {1}".format(a, old_b))
        self.assertTrue((a == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(a, expect))

    @skipNoDevice
    def test_op_setslice_offload_array_int(self):
        """Test __setslice__ operation of OffloadArray."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1, 4711 * 1024, dtype=int)
        b = a * -1
        expect = numpy.empty_like(b)
        old_b = numpy.empty_like(b)
        expect[:] = b[:]
        old_b[:] = b[:]

        offl_a = stream.bind(a)
        offl_b = stream.bind(b)
        offl_a[:] = offl_b
        offl_a.update_host()
        stream.sync()

        self.assertTrue((b == old_b).all(),
                        "Input array operand 1 must not be modified: "
                        "{0} should be {1}".format(a, old_b))
        self.assertTrue((a == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(a, expect))

    @skipNoDevice
    def test_op_setslice_offload_array_float(self):
        """Test __setslice__ operation of OffloadArray."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1, 4711 * 1024, dtype=float)
        b = a + 2.5
        expect = numpy.empty_like(b)
        old_b = numpy.empty_like(b)
        expect[:] = b[:]
        old_b[:] = b[:]

        offl_a = stream.bind(a)
        offl_b = stream.bind(b)
        offl_a[:] = offl_b
        offl_a.update_host()
        stream.sync()

        self.assertTrue((b == old_b).all(),
                        "Input array operand 1 must not be modified: "
                        "{0} should be {1}".format(a, old_b))
        self.assertTrue((a == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(a, expect))

    @skipNoDevice
    def test_op_setslice_offload_array_complex(self):
        """Test __setslice__ operation of OffloadArray."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        a = numpy.arange(1, 4711 * 1024, dtype=complex)
        b = a + 2.5j
        expect = numpy.empty_like(b)
        old_b = numpy.empty_like(b)
        expect[:] = b[:]
        old_b[:] = b[:]

        offl_a = stream.bind(a)
        offl_b = stream.bind(b)
        offl_a[:] = offl_b
        offl_a.update_host()
        stream.sync()

        self.assertTrue((b == old_b).all(),
                        "Input array operand 1 must not be modified: "
                        "{0} should be {1}".format(a, old_b))
        self.assertTrue((a == expect).all(),
                        "Array contains unexpected values: "
                        "{0} should be {1}".format(a, expect))
