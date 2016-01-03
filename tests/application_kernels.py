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

import unittest
import numpy

import pymic

from helper import skipNoDevice
from helper import get_library

epsilon = 0.0000000001


class ApplicationKernelTests(unittest.TestCase):
    """This class defines test cases constructed from application kernels."""

    def assertEqualEpsilon(self, first, second, msg=None):
        """Test equal for floating point with deviation."""

        def epsilonCompare(value):
            return (abs(value) <= epsilon)

        comparison = map(epsilonCompare, (first - second))
        return self.assertTrue(all(comparison), msg)

    @skipNoDevice
    def impl_test_dgemm(self, size):
        """Test an MKL dgemm with 256x256 matrices."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        library = get_library(device, "libtests.so")

        m, n, k = size, size, size
        alpha, beta = 1.2, 1.2
        numpy.random.seed(10)
        a = numpy.random.random(m * k).reshape((m, k))
        b = numpy.random.random(k * n).reshape((k, n))
        c = numpy.random.random(m * n).reshape((m, n))

        A = numpy.matrix(a)
        B = numpy.matrix(b)
        C = numpy.matrix(c)
        expect = numpy.array(alpha * A * B + beta * C)

        offl_a = stream.bind(a)
        offl_b = stream.bind(b)
        offl_c = stream.bind(c)

        stream.invoke(library.test_kernel_dgemm,
                      offl_a, offl_b, offl_c,
                      m, n, k, alpha, beta)
        offl_c.update_host()
        stream.sync()
        r = offl_c.array

        self.assertEqualEpsilon(r.reshape((m * n,)), expect.reshape((m * n,)),
                                "Array contains unexpected values: "
                                "{0} should be {1}".format(r, expect))

    @skipNoDevice
    def impl_test_zgemm(self, size):
        """Test an MKL zgemm with 256x256 matrices."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        library = get_library(device, "libtests.so")

        m, n, k = size, size, size
        alpha, beta = complex(1.2, 0.5j), complex(0.5, 0.6j)
        numpy.random.seed(20)
        ar = numpy.random.random(m * k).reshape((m, k))
        aj = numpy.random.random(m * k).reshape((m, k))
        br = numpy.random.random(k * n).reshape((k, n))
        bj = numpy.random.random(k * n).reshape((k, n))
        cr = numpy.random.random(m * n).reshape((m, n))
        cj = numpy.random.random(m * n).reshape((m, n))
        a = ar + 1j * aj
        b = br + 1j * bj
        c = cr + 1j * cj

        A = numpy.matrix(a)
        B = numpy.matrix(b)
        C = numpy.matrix(c)
        expect = numpy.array(alpha * A * B + beta * C)

        offl_a = stream.bind(a)
        offl_b = stream.bind(b)
        offl_c = stream.bind(c)
        stream.invoke(library.test_kernel_zgemm,
                      offl_a, offl_b, offl_c,
                      m, n, k, alpha, beta)
        offl_c.update_host()
        stream.sync()
        r = offl_c.array

        self.assertEqualEpsilon(r.reshape((m * n,)), expect.reshape((m * n,)),
                                "Array contains unexpected values: "
                                "{0} should be {1}".format(r, expect))


def _add_tests_gemm(size):
    def test_method_dgemm(self):
        self.impl_test_dgemm(size)

    def test_method_zgemm(self):
        self.impl_test_zgemm(size)

    setattr(ApplicationKernelTests, 'test_dgemm_{0}'.format(size),
            test_method_dgemm)
    setattr(ApplicationKernelTests, 'test_zgemm_{0}'.format(size),
            test_method_zgemm)
    test_method_dgemm.__name__ = 'test_dgemm_{0}'.format(size)
    test_method_zgemm.__name__ = 'test_zgemm_{0}'.format(size)


# for size in xrange(8, 128, 8):
#     _add_tests_gemm(size)
# for size in xrange(256, 4096, 256):
#     _add_tests_gemm(size)
_add_tests_gemm(1024)
_add_tests_gemm(256)
