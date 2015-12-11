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
from helper import skipNoMultipleDevices
from helper import get_library


class OffloadLibraryTests(unittest.TestCase):
    """This class defines the test cases for the OffloadLibrary class."""

    @skipNoDevice
    def test_kernel_names_available(self):
        """Test if all device kernels can be found with their correct function
           pointers."""

        device = pymic.devices[0]
        stream = device.get_default_stream()
        library = get_library(device, "libkernelnames.so")

        pointers = numpy.empty((5,), dtype=numpy.int64)
        offl_pointers = stream.bind(pointers)

        stream.invoke(library.test_offload_library_get, offl_pointers)
        offl_pointers.update_host()
        stream.sync()

        kernels = [library.kernel_underscores, library.a, library.bb,
                   library._bb, library.a123]
        for p, k in zip(pointers, kernels):
            self.assertEqual(p, k[1], "Kernel pointer does not match (0x{0:x} "
                                      "should be 0x{0:x}".format(k[1], p))

    @skipNoMultipleDevices
    def test_library_device_mismatch(self):
        """Test that kernel invocation fails if library has been loaded for
           another device."""

        device1 = pymic.devices[0]
        device2 = pymic.devices[1]
        stream = device2.get_default_stream()
        library = get_library(device1, "libkernelnames.so")

        self.assertRaises(pymic.OffloadError, stream.invoke,
                          library.kernel_underscores)
