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

import os
import sys
import numpy


class pymicConfig:
    """Object to hold the runtime configuration of pymic."""
    def __init__(self):
        # PYMIC_DEBUG
        self._debug_level = None
        _debug_level = os.getenv("PYMIC_DEBUG", None)
        try:
            self._debug_level = int(_debug_level)
        except:
            pass

        # PYMIC_TRACE
        self._trace_level = 0
        _trace_level = os.getenv("PYMIC_TRACE", 0)
        try:
            self._trace_level = int(_trace_level)
        except:
            pass

        # PYMIC_TRACE_STACKS
        _collect_stacks_str = os.getenv("PYMIC_TRACE_STACKS", "compact")
        self._collect_stacks_str = _collect_stacks_str.lower()

        # PYMIC_LIBRARY_PATH
        self._search_path = os.getenv("PYMIC_LIBRARY_PATH", "")


_config = pymicConfig()


# determine the integer value for PYMIC_DEBUG
if _config._debug_level is not None:
    print("PYMIC: debug level set to {0}".format(_config._debug_level),
          file=sys.stderr)


def _debug(level, format, *objs):
    if _config._debug_level >= level:
        msg = "PYMIC: " + format.format(*objs)
        print(msg, file=sys.stderr)
    return None


def _deprecated(func):
    def wrapper(*args):
        print("Warning: function '{0}' has been "
              "deprecated".format(func.__name__))
        func(*args)
    return wrapper


def _get_order(array):
    """Determine the storage order of an array and map it to its string
       representation.
    """
    if isinstance(array, numpy.ndarray):
        if array.flags['CA']:
            return 'C'
        if array.flags['FA']:
            return 'F'
        raise TypeError("cannot determine order of the input array")
    else:
        return array.order

_data_type_map = {
    # Python types
    int: 0,
    float: 1,
    complex: 2,

    # Numpy dtypes
    numpy.dtype(numpy.int64): 0,
    numpy.dtype(numpy.float64): 1,
    numpy.dtype(numpy.complex128): 2,
    numpy.dtype(numpy.uint64) : 3
}


def _map_data_types(dtype):
    """Map a (known) data type to an integer to be used in the native kernels
       that implement basic operations of OffloadArray."""
    return _data_type_map[dtype]


class _DeviceAllocation:
    """Smart pointer to store the fake pointer and perform pointer
       translation and offset computation."""

    _stream = None
    _device_id = None
    _device_ptr = None
    _sticky = False

    def __init__(self, stream, device, device_ptr, sticky):
        """Initialize the fake pointer with the data coming from
           OffloadStream.allocate_device_memory."""
        assert stream != None
        assert device != None
        assert device_ptr != None
        self._stream = stream
        self._device = device
        self._device_ptr = device_ptr
        self._sticky = sticky

    def __str__(self):
        """Pretty print the value of this fake pointer."""
        return '0x{0:x}'.format(self._device_ptr)

    def __del__(self):
        if not self._sticky:
            self._stream.deallocate_device_memory(self)
