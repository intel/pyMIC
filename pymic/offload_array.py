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

import numpy

from pymic._misc import _debug as debug
from pymic._misc import _deprecated as deprecated
from pymic._tracing import _trace as trace
from pymic._misc import _map_data_types as map_data_types
from pymic._misc import _is_complex_type as is_complex_type

from pymic.offload_device import OffloadDevice
from pymic.offload_device import devices

# TODO:
#   - find out how to easily copy the whole numpy.array interface


_offload_libraries = {}
for d in devices:
    if d is not any:
        dev = devices[d]
        _offload_libraries[d] = dev.load_library("liboffload_array.so")


def _check_arrays(arr_a, arr_b):
    if arr_a.shape != arr_b.shape:
        raise ValueError("shapes of the arrays do not match: "
                         "{0} != {1}".format(arr_a.shape, arr_b.shape))
    if arr_a.dtype != arr_b.dtype:
        raise ValueError("Data types do not match: "
                         "{0} != {1}".format(arr_a.dtype, arr_b.dtype))


def _check_scalar(array, scalar):
    if array.dtype != type(other):
        raise ValueError("Data types do not match: "
                         "{0} != {1}".format(array.dtype, type(other)))


class OffloadArray(object):
    """An offloadable array structure to perform array-based computation
       on an Intel(R) Xeon Phi(tm) Coprocessor

       The interface is largely numpy-alike.  All operators execute their
       respective operation in an element-wise fashion on the target device.
    """

    array = None
    device = None
    stream = None
    _library = None
    _device_ptr = None
    _nbytes = None

    def __init__(self, shape, dtype, order="C",
                 alloc_arr=True, base=None, device=None, stream=None):
        # allocate the array on the coprocessor
        self.order = order
        self.dtype = numpy.dtype(dtype)
        self.base = base

        # save a reference to the device
        assert device is not None
        assert stream is not None
        self.device = device
        self.stream = stream

        # determine size of the array from its shape
        try:
            size = 1
            for d in shape:
                size *= d
            ndim = len(shape)
        except TypeError:
            assert isinstance(shape, (int, long))
            size = shape
            shape = (shape,)
            ndim = 1
        self.size = size
        self.shape = shape
        self.ndim = ndim
        self._nbytes = self.dtype.itemsize * self.size

        if base is not None:
            self.array = base.array.reshape(shape)
        else:
            if alloc_arr:
                if stream is None:
                    stream = self.stream
                self.array = numpy.empty(self.shape, self.dtype, self.order)
                self._device_ptr = stream.allocate_device_memory(self._nbytes)

        self._library = _offload_libraries[device.device_id]

    def __str__(self):
        return str(self.array)

    def __repr__(self):
        return repr(self.array)

    def __hash__(self):
        raise TypeError("An OffloadArray is not hashable.")

    @trace
    def update_device(self):
        """Update the OffloadArray's buffer space on the associated
           device by copying the contents of the associated numpy.ndarray
           to the device.

           Parameters
           ----------
           n/a

           Returns
           -------
           out : OffloadArray
               The object instance of this OffloadArray.

           See Also
           --------
           update_host
        """
        host_ptr = self.array.ctypes.get_data()
        self.stream.transfer_host2device(host_ptr, self._device_ptr,
                                         self._nbytes)
        return None

    @trace
    def update_host(self):
        """Update the associated numpy.ndarray on the host with the contents
           by copying the OffloadArray's buffer space from the device to the
           host.

           The operation is enqueued into the array's default stream object
           and completes asynchronously.

           Parameters
           ----------
           n/a

           Returns
           -------
           out : OffloadArray
               The object instance of this OffloadArray.

           See Also
           --------
           update_device
        """
        host_ptr = self.array.ctypes.get_data()
        self.stream.transfer_device2host(self._device_ptr, host_ptr,
                                         self._nbytes)
        return self

    def assign_stream(self, stream):
        """Assign a new stream for this OffloadArray's operations
           (update_device, update_host, __add__, etc.).

           Parameters
           ----------
           stream : OffloadStream
               New default stream.

           Returns
           -------
           n/a

           See Also
           --------
           n/a
        """
        if stream.get_device() is not self.device:
            raise ValueError("Cannot assign a stream from different device "
                             "({0} != {1})".format(self.device,
                                                   stream.get_device()))
        self.stream = stream

    def __add__(self, other):
        """Add an array or scalar to an array.

           The operation is enqueued into the array's default stream object
           and completes asynchronously.
        """

        dt = map_data_types(self.dtype)
        n = int(self.size)
        x = self
        y = other
        incx = int(1)
        if isinstance(other, (OffloadArray, numpy.ndarray)):
            _check_arrays(self, other)
            incy = int(1)
            incr = int(1)
        else:
            # scalar
            _check_scalar(self, other)
            incy = int(0)
            incr = int(1)
        result = OffloadArray(self.shape, self.dtype, device=self.device,
                              stream=self.stream)
        self.stream.invoke(self._library.pymic_offload_array_add,
                           dt, n, x, incx, y, incy, result, incr)
        return result

    def __iadd__(self, other):
        """Add an array or scalar to an array (in-place operation)."""

        dt = map_data_types(self.dtype)
        n = int(self.size)
        x = self
        y = other
        incx = int(1)
        if isinstance(other, (OffloadArray, numpy.ndarray)):
            _check_arrays(self, other)
            incy = int(1)
        else:
            # scalar
            _check_scalar(self, other)
            incy = int(0)
        self.stream.invoke(self._library.pymic_offload_array_add,
                           dt, n, x, incx, y, incy, x, incx)
        return self

    def __sub__(self, other):
        """Subtract an array or scalar from an array.

           The operation is enqueued into the array's default stream object
           and completes asynchronously.
        """

        dt = map_data_types(self.dtype)
        n = int(self.size)
        x = self
        y = other
        incx = int(1)
        if isinstance(other, (OffloadArray, numpy.ndarray)):
            _check_arrays(self, other)
            incy = int(1)
            incr = int(1)
        else:
            # scalar
            _check_scalar(self, other)
            incy = int(0)
            incr = int(1)
        result = OffloadArray(self.shape, self.dtype, device=self.device,
                              stream=self.stream)
        self.stream.invoke(self._library.pymic_offload_array_sub,
                           dt, n, x, incx, y, incy, result, incr)
        return result

    def __isub__(self, other):
        """Subtract an array or scalar from an array (in-place operation)."""

        dt = map_data_types(self.dtype)
        n = int(self.size)
        x = self
        y = other
        incx = int(1)
        if isinstance(other, (OffloadArray, numpy.ndarray)):
            _check_arrays(self, other)
            incy = int(1)
        else:
            # scalar
            _check_scalar(self, other)
            incy = int(0)
        self.stream.invoke(self._library.pymic_offload_array_sub,
                           dt, n, x, incx, y, incy, x, incx)
        return self

    def __mul__(self, other):
        """Multiply an array or a scalar with an array.

           The operation is enqueued into the array's default stream object
           and completes asynchronously.
        """

        dt = map_data_types(self.dtype)
        n = int(self.size)
        x = self
        y = other
        incx = int(1)
        if isinstance(other, (OffloadArray, numpy.ndarray)):
            _check_arrays(self, other)
            incy = int(1)
            incr = int(1)
        else:
            # scalar
            _check_scalar(self, other)
            incy = int(0)
            incr = int(1)
        result = OffloadArray(self.shape, self.dtype, device=self.device,
                              stream=self.stream)
        self.stream.invoke(self._library.pymic_offload_array_mul,
                           dt, n, x, incx, y, incy, result, incr)
        return result

    def __imul__(self, other):
        """Multiply an array or a scalar with an array (in-place operation)."""

        dt = map_data_types(self.dtype)
        n = int(self.size)
        x = self
        y = other
        incx = int(1)
        if isinstance(other, (OffloadArray, numpy.ndarray)):
            _check_arrays(self, other)
            incy = int(1)
        else:
            # scalar
            _check_scalar(self, other)
            incy = int(0)
        self.stream.invoke(self._library.pymic_offload_array_mul,
                           dt, n, x, incx, y, incy, x, incx)
        return self

    def fill(self, value):
        """Fill an array with the specified value.

           Parameters
           ----------
           value : type
               Value to fill the array with.

           Returns
           -------
           out : OffloadArray
               The object instance of this OffloadArray.

           See Also
           --------
           zero
        """
        if not numpy.issubdtype(self.dtype, type(value)):
            raise ValueError("Data types do not match: "
                             "{0} != {1}".format(self.dtype, type(value)))

        dt = map_data_types(self.dtype)
        n = int(self.size)
        x = self

        self.stream.invoke(self._library.pymic_offload_array_fill,
                           dt, n, x, value)

        return self

    def fillfrom(self, array):
        """Fill an array from a numpy.ndarray.

           The operation is enqueued into the array's default stream object
           and completes asynchronously.
        """

        if not isinstance(array, numpy.ndarray):
            raise TypeError("only numpy.ndarray supported")
        _check_arrays(self, array)

        # copy data directly into the offload buffer
        nbytes = self._nbytes
        cptr = array.ctypes.data
        self.stream.transfer_host2device(cptr, self._device_ptr, nbytes)

        return self

    def zero(self, zero_value=None):
        """Fill the array with zeros.

           Parameters
           ----------
           n/a

           Returns
           -------
           out : OffloadArray
               The object instance of this OffloadArray.

           See Also
           --------
           fill
        """
        if zero_value is None:
            if numpy.issubdtype(self.dtype, numpy.int):
                zero_value = 0
            elif numpy.issubdtype(self.dtype, numpy.float):
                zero_value = 0.0
            elif numpy.issubdtype(self.dtype, numpy.complex):
                zero_value = complex(0.0, 0.0)
            else:
                raise ValueError("Do not know representation of zero "
                                 "for type {0}".format(self.dtype))
        return self.fill(zero_value)

    def one(self, one_value=None):
        """Fill the array with ones.

           Parameters
           ----------
           n/a

           Returns
           -------
           out : OffloadArray
               The object instance of this OffloadArray.

           See Also
           --------
           fill
        """
        if one_value is None:
            if numpy.issubdtype(self.dtype, numpy.int):
                one_value = 1
            elif numpy.issubdtype(self.dtype, numpy.float):
                one_value = 1.0
            elif numpy.issubdtype(self.dtype, numpy.complex):
                one_value = complex(1.0, 0.0)
            else:
                raise ValueError("Do not know representation of one "
                                 "for type {0}".format(dtype))
        return self.fill(one_value)

    def __len__(self):
        """Return the of size of the leading dimension."""
        if len(self.shape):
            return self.shape[0]
        else:
            return 1

    def __abs__(self):
        """Return a new OffloadArray with the absolute values of the elements
           of `self`.

           The operation is enqueued into the array's default stream object
           and completes asynchronously.
        """

        dt = map_data_types(self.dtype)
        n = int(self.array.size)
        x = self
        if is_complex_type(self.dtype):
            result = self.stream.empty(self.shape, dtype=numpy.float,
                                       order=self.order, update_host=False)
        else:
            result = self.stream.empty_like(self, update_host=False)
        self.stream.invoke(self._library.pymic_offload_array_abs,
                           dt, n, x, result)
        return result

    def __pow__(self, other):
        """Element-wise pow() function.

           The operation is enqueued into the array's default stream object
           and completes asynchronously.
        """

        dt = map_data_types(self.dtype)
        n = int(self.size)
        x = self
        incx = int(1)
        if isinstance(other, (OffloadArray, numpy.ndarray)):
            _check_arrays(self, other)
            incy = int(1)
            incr = int(1)
        else:
            # scalar
            _check_scalar(self, other)
            incy = int(0)
            incr = int(1)
        result = OffloadArray(self.shape, self.dtype, device=self.device,
                              stream=self.stream)
        self.stream.invoke(self._library.pymic_offload_array_pow,
                           dt, n, x, incx, y, incy, result, incr)
        return result

    def reverse(self):
        """Return a new OffloadArray with all elements in reverse order.

           The operation is enqueued into the array's default stream object
           and completes asynchronously.
        """

        if self.ndim > 1:
            raise ValueError("Multi-dimensional arrays cannot be revered.")

        dt = map_data_types(self.dtype)
        n = int(self.array.size)
        result = self.stream.empty_like(self)
        self.stream.invoke(self._library.pymic_offload_array_reverse,
                           dt, n, self, result)
        return result

    def reshape(self, *shape):
        """Assigns a new shape to an existing OffloadArray without changing
           the data of it.

           The operation is enqueued into the array's default stream object
           and completes asynchronously.
        """

        if isinstance(shape[0], tuple) or isinstance(shape[0], list):
            shape = tuple(shape[0])
        # determine size of the array from its shape
        try:
            size = 1
            for d in shape:
                size *= d
        except TypeError:
            assert isinstance(shape, (int, long, numpy.integer))
            size = shape
            shape = (shape,)
        if size != self.size:
            raise ValueError("total size of reshaped array must be unchanged")
        return OffloadArray(shape, self.dtype, self.order,
                            False, self, device=self.device)

    def ravel(self):
        """Return a flattened array.

           The operation is enqueued into the array's default stream object
           and completes asynchronously.
        """
        return self.reshape(self.size)

    def __setslice__(self, i, j, sequence):
        """Overwrite this OffloadArray with slice coming from another array.

           The operation is enqueued into the array's default stream object
           and completes asynchronously.
        """
        lb = min(i, self.size)
        ub = min(j, self.size)
        self.__setitem__(slice(lb, ub, 1), sequence)

    def __setitem__(self, index, sequence):
        """Overwrite this OffloadArray with slice coming from another array.

           The operation is enqueued into the array's default stream object
           and completes asynchronously.
        """
        lb, ub, stride = index.start, index.stop, index.step
        if lb is None:
            lb = 0
        if ub is None:
            ub = self.size
        if stride is None:
            stride = 1

        # TODO: add additional checks here: shape/size/data type
        if stride != 1:
            raise ValueError('Cannot assign with stride not equal to 1')

        dt = map_data_types(self.dtype)
        if isinstance(sequence, OffloadArray):
            self.stream.invoke(self._library.pymic_offload_array_setslice,
                               dt, lb, ub, self, sequence)
        elif isinstance(sequence, numpy.ndarray):
            offl_sequence = self.stream.bind(sequence)
            self.stream.invoke(self._library.pymic_offload_array_setslice,
                               dt, lb, ub, self, offl_sequence)
            self.stream.sync()
        else:
            self.fill(sequence)
