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

import numpy

from offload_error import OffloadError

from _pymicimpl import _pymic_impl_buffer_allocate
from _pymicimpl import _pymic_impl_buffer_release
from _pymicimpl import _pymic_impl_buffer_update_on_target
from _pymicimpl import _pymic_impl_buffer_update_on_host
from _pymicimpl import _pymic_impl_buffer_alloc_and_copy_to_target
from _pymicimpl import _pymic_impl_buffer_copy_from_target_and_release
from _pymicimpl import _pymic_impl_invoke_kernel

from _misc import _debug as debug
from _misc import _get_order as get_order
from _tracing import _trace as trace

import pymic


streamId = {}


class OffloadStream:
    """
    """

    _device = None
    _device_id = None
    _stream_id = None

    def __init__(self, device=None):
        # save a reference to the device
        assert device is not None
        self._device = device
        self._device_id = device.device_id
        
        # generate a new ID for this stream (it's not really needed,
        # but it's easier to talk about stream 1 :-))
        lastId = streamId.get(self._device_id)
        if lastId is None:
            lastId = -1
        lastId += 1
        streamId[self._device_id] = lastId
        self._stream_id = lastId

    def __del__(self):
        pass
        
    @trace
    def sync(self):
        """Wait for all outstanding requests in this OffloadStream to complete.
           The function does not return until all in-flight requests have
           completed.

           Parameters
           ----------
           n/a

           Returns
           -------
           out : OffloadArray
               The object instance of this OffloadArray.

           See Also
           --------
           n/a
        """
        pass

    def __eq__(self, other):
        return (self._device == other.device and 
                self._stream_id == other.stream_id)

    def __repr__(self):
        return "OffloadStream({0}) for {1}".format(self._stream_id, 
                                                   self._device.__repr__())

    def __str__(self):
        return "stream({0}) for {1}".format(self._stream_id, str(self._device))

    def _buffer_allocate(self, *arrays):
        if len(arrays) == 0:
            raise ValueError("no argument given")
        # if called from wrapper, actual arguments are wrapped in an 
        # extra tuple, so we unwrap them
        if type(arrays[0]) == tuple:
            arrays = arrays[0]
        for array in arrays:
            nbytes = int(array.nbytes)
            debug(2, "(device {0}) allocating buffer with "
                     "{1} byte(s)".format(self._device_id, nbytes))
            _pymic_impl_buffer_allocate(self._device_id, array, nbytes)
        return None

    def _buffer_release(self, *arrays):
        if len(arrays) == 0:
            raise ValueError("no argument given")
        # if called from wrapper, actual arguments are wrapped in an 
        # extra tuple, so we unwrap them
        if type(arrays[0]) == tuple:
            arrays = arrays[0]
        for array in arrays:
            nbytes = int(array.nbytes)
            debug(2, "(device {0}) releasing buffer with "
                     "{1} byte(s)".format(self._device_id, nbytes))
            _pymic_impl_buffer_release(self._device_id, array, nbytes)
        return None

    def _buffer_update_on_target(self, *arrays):
        if len(arrays) == 0:
            raise ValueError("no argument given")
        # if called from wrapper, actual arguments are wrapped in an 
        # extra tuple, so we unwrap them
        if type(arrays[0]) == tuple:
            arrays = arrays[0]
        for array in arrays:
            nbytes = int(array.nbytes)
            debug(1, "(host -> device {0}) transferring "
                     "{1} byte(s)".format(self._device_id, nbytes))
            _pymic_impl_buffer_update_on_target(self._device_id, array, nbytes)
        return None

    def _buffer_update_on_host(self, *arrays):
        if len(arrays) == 0:
            raise ValueError("no argument given")
        # if called from wrapper, actual arguments are wrapped in an 
        # extra tuple, so we unwrap them
        if type(arrays[0]) == tuple:
            arrays = arrays[0]
        for array in arrays:
            nbytes = int(array.nbytes)
            debug(1, "(device {0} -> host) transferring "
                     "{1} byte(s)".format(self._device_id, nbytes))
            _pymic_impl_buffer_update_on_host(self._device_id, array, nbytes)
        return None

    def _copy_to_target(self, *arrays):
        if len(arrays) == 0:
            raise ValueError("no argument given")
        # if called from wrapper, actual arguments are wrapped in an 
        # extra tuple, so we unwrap them
        if type(arrays[0]) == tuple:
            arrays = arrays[0]
        for array in arrays:
            nbytes = int(array.nbytes)
            debug(2, "(device {0}) allocating buffer with "
                     "{1} byte(s)".format(self._device_id, nbytes))
            debug(1, "(host -> device {0}) transferring "
                     "{1} byte(s)".format(self._device_id, nbytes))
            _pymic_impl_buffer_alloc_and_copy_to_target(self._device_id, 
                                                        array, nbytes)
        return None

    def _copy_from_target(self, *arrays):
        if len(arrays) == 0:
            raise ValueError("no argument given")
        # if called from wrapper, actual arguments are wrapped in an 
        # extra tuple, so we unwrap them
        if type(arrays[0]) == tuple:
            arrays = arrays[0]
        for array in arrays:
            nbytes = int(array.nbytes)
            debug(1, "(device {0} -> host) transferring "
                     "{1} byte(s)".format(self._device_id, nbytes))
            debug(2, "(device {0}) releasing buffer with "
                     "{1} byte(s)".format(self._device_id, nbytes))
            _pymic_impl_buffer_copy_from_target_and_release(self._device_id, 
                                                            array, nbytes)
        return None

    @staticmethod
    def _convert_argument_to_array(argument):
        """Internal helper function to convert scalar arguments to
           single-element numpy.ndarray.
        """

        # convert the argument to a single-element array
        if ((not isinstance(argument, numpy.ndarray)) and 
                (not isinstance(argument, pymic.OffloadArray))):
            cvtd = numpy.asarray(argument)
            return cvtd
        else:
            return argument

    @trace
    def invoke(self, kernel, *args):
        """Invoke a native kernel on the target device by enqueuing a request
           in the current stream.  The kernel is identified by accessing its 
           library's attribute with the same name as the kernel name. The 
           kernel function needs to be in a shared-object library that has 
           been loaded by calling the load_library of the target device 
           before invoke.

           The additional arguments of invoke can be either instances
           of OffloadArray, numpy.ndarray, or scalar data.  For numpy.ndarray
           or scalar arguments, invoke automatically performs copy-in and
           copy-out of the argument, that is, before the kernel is invoked,
           the argument is automatically transferred to the target device and
           transferred back after the kernel has finished.

           Parameters
           ----------
           kernel : kernel
              Kernel to be invoked
           args : OffloadArray, numpy.ndarray, or scalar type
              Arguments to be passed to the kernel function

           See Also
           --------
           load_library

           Returns
           -------
           None

           Examples
           --------
           >>> library = device.load_library("libdgemm")
           >>> stream.invoke(library.dgemm, A, B, C, n, m, k)
        """

        # if called from wrapper, actual arguments are wrapped in an 
        # extra tuple, so we unwrap them
        if len(args):
            if type(args[0]) == tuple:
                args = args[0]

        # safety check: avoid invoking a kernel if it's library has been loaded
        # on a different device
        if kernel[2] is not self._device:
            raise OffloadError("Cannot invoke kernel, "
                               "library not loaded on device")

        converted = map(OffloadStream._convert_argument_to_array, args)
        debug(1, "(device {0}, stream {1}) invoking kernel '{2}' "
                 "(pointer 0x{3:x}) with {4} "
                 "argument(s)".format(self._device_id, self._stream_id, 
                                      kernel[0], kernel[1], len(args)))
        _pymic_impl_invoke_kernel(self._device_id, kernel[1], tuple(converted))
        return None
        
    @trace
    def bind(self, array, update_device=True):
        """Create a new OffloadArray that is bound to an existing
           numpy.ndarray.  If update_device is False, then one needs to call
           update_device() to issue a data transfer to the target device.
           Otherwise, the OffloadArray will be in unspecified state on
           the target device.  Data transfers to the host overwrite the
           data in the bound numpy.ndarray.

           Parameters
           ----------
           array         : numpy.ndarray
               Array to be bound to a new OffloadArray
           update_device : bool, optional, default True
               Control if the device buffer is updated with the array data

           Returns
           -------
           out : OffloadArray
               Instance of OffloadArray bound to the input array

           See Also
           --------
           copy

           Examples
           --------
           >>> a = numpy.zeros((2,2))
           >>> oa = stream.bind(a)

           >>> b = numpy.ones((1,1))
           >>> ob = stream.bind(b, update_device=False)
           >>> ob.update_device()
        """

        if not isinstance(array, numpy.ndarray):
            raise ValueError("only numpy.ndarray can be associated "
                             "with OffloadArray")

        # detect the order of storage for 'array'
        if array.flags.c_contiguous:
            order = "C"
        elif array.flags.f_contiguous:
            order = "F"
        else:
            raise ValueError("could not detect storage order")

        # construct and return a new OffloadArray
        bound = pymic.OffloadArray(array.shape, array.dtype, order, False, 
                                   device=self._device, stream=self)
        bound.array = array

        # allocate the buffer on the device (and update data)
        if update_device:
            self._copy_to_target(bound.array)
        else:
            self._buffer_allocate(bound.array)

        return bound

    @trace
    def copy(self, array, update_device=True):
        """Create a new OffloadArray that is a copy of an existing
           numpy.ndarray.  If update_device is False, then one needs to call
           update_device() to issue a data transfer to the target device.
           Otherwise, the OffloadArray will be in unspecified state on
           the target device.  Once the copy has been made, the original
           numpy.ndarray will not be modified by any data transfer operations.

           Parameters
           ----------
           array         : numpy.ndarray
               Array to be bound to a new OffloadArray
           update_device : bool, optional, default True
               Control if the device buffer is updated with the array data

           Returns
           -------
           out : OffloadArray
               Instance of OffloadArray with copied array

           See Also
           --------
           bind

           Examples
           --------
           >>> a = numpy.zeros((2,2))
           >>> oa = stream.copy(a)

           >>> b = numpy.zeros(...)
           >>> ob = stream.copy(b, False)
           >>> ob.update_device()
        """

        if not isinstance(array, numpy.ndarray):
            raise ValueError("only numpy.ndarray can be associated "
                             "with OffloadArray")

        # detect the order of storage for 'array'
        if array.flags.c_contiguous:
            order = "C"
        elif array.flags.f_contiguous:
            order = "F"
        else:
            raise ValueError("could not detect storage order")

        array_copy = numpy.empty_like(array)
        array_copy[:] = array[:]
        return self.bind(array_copy, update_device, device=self._device, 
                         stream=self)

    @trace
    def empty(self, shape, dtype=numpy.float, order='C', update_host=True):
        """Create a new OffloadArray that is bound to an empty numpy.ndarray
           of the given shape and data type.  If update_host is True (the
           default), then the empty array is automatically transferred to
           the host numpy.ndarray.  If set to False, the data transfer is
           avoided and the data in the host array is in undefined state.

           Parameters
           ----------
           shape       : int or tuple of int
               Shape of the empty array
           dtype       : data-type, optional
               Desired output data-type of the empty array
           order       : {'C', 'F'}, optional, default 'C'
               Store multi-dimensional array data in row-major order
               (C/C++, 'C') or column-major order (Fortran, 'F')
           update_host : bool, optional, default True
               Control if the host array is updated with the array data
               during construction of the OffloadArray

           Returns
           -------
           out : OffloadArray
               Instance of OffloadArray bound to the input array

           See Also
           --------
           empty_like, zeros, zeros_like, ones, ones_like, bcast, bcast_like

           Examples
           --------
           >>> stream.empty((2,2))
           array([[  4.41694687e-321,   6.92646058e-310],
                  [  4.71338626e-321,   6.92646058e-310]])  # random data

           >>> o = stream.empty((2,2), update_host=False)
           >>> o
           array([[  0.00000000e+000,  -3.13334375e-294],
                  [  2.77063169e-302,   1.76125728e-312]])  # random data
           >>> o.update_host()
           array([[  4.26872718e-321,   6.92050581e-310],
                  [  4.34283703e-321,   6.92050581e-310]])  # random data
        """
        array = pymic.OffloadArray(shape, dtype, order, device=self._device, 
                                   stream=self)
        if update_host:
            array.update_host()
        return array

    @trace
    def empty_like(self, other, update_host=True):
        """Create a new OffloadArray that is bound to an empty numpy.ndarray
           that matches shape and type of an existing OffloadArray or
           numpy.ndarray.  If update_host is True (the default), then the empty
           array is automatically transferred to the host numpy.ndarray.  If
           set to False, the data transfer is avoided and the data in the host
           array is in undefined state.

           Parameters
           ----------
           other       : numpy.ndarray
               Template array used to determine shape, data type, and storage
               order
           update_host :
               Control if the host array is updated with the array data
               during construction of the OffloadArray

           See Also
           --------
           empty, zeros, zeros_like, ones, ones_like, bcast, bcast_like

           Examples
           --------
           >>> a = numpy.ones((2,2))
           >>> stream.empty_like(a)
           array([[  4.26872718e-321,   6.91990916e-310],
                  [  4.34283703e-321,   6.91990916e-310]])  # random data

           >>> o = stream.empty_like(a, update_host=False)
           >>> o
           array([[  1.12110451e-316,   1.32102240e-312],
                  [  6.94088686e-310,   6.94088688e-310]])  # random data
           >>> o.update_host()
           array([[  4.26872718e-321,   6.95150287e-310],
                  [  4.34283703e-321,   6.95150287e-310]])  # random data
        """
        if (not isinstance(other, numpy.ndarray) and 
                not isinstance(other, pymic.OffloadArray)):
            raise ValueError("only numpy.ndarray can be used "
                             "with this function")
        return self.empty(other.shape, other.dtype, get_order(other), 
                          update_host=update_host)

    @trace
    def zeros(self, shape, dtype=numpy.float, order='C', update_host=True):
        """Create a new OffloadArray that is bound to a numpy.ndarray
           of the given shape and data type, and that is initialized with all
           elements set to zero of the corresponding data type.  If update_host
           is True (the default), then the empty array is automatically
           transferred to the host numpy.ndarray.  If set to False, the
           data transfer is avoided and the data in the host array is in
           undefined state.

           Parameters
           ----------
           shape       : int or tuple of int
               Shape of the empty array
           dtype       : data-type, optional
               Desired output data-type of the empty array
           order       : {'C', 'F'}, optional, default 'C'
               Store multi-dimensional array data in row-major order 
               (C/C++, 'C') or column-major order (Fortran, 'F')
           update_host : bool, optional, default True
               Control if the host array is updated with the array data
               during construction of the OffloadArray

           Returns
           -------
           out : OffloadArray
               Instance of OffloadArray with all elements being zero

           See Also
           --------
           empty, empty_like, zeros_like, ones, ones_like, bcast, bcast_like

           Examples
           --------
           >>> stream.zeros((2,2))
           array([[ 0.,  0.],
                  [ 0.,  0.]])

           >>> o = stream.zeros((2,2), update_host=False)
           >>> o
           array([[  0.00000000e+000,  -3.13334375e-294],
                  [  2.77063169e-302,   1.16247550e-012]])  # random data
           >>> o.update_host()
           array([[ 0.,  0.],
                  [ 0.,  0.]])
        """
        array = pymic.OffloadArray(shape, dtype, order, device=self._device, 
                                   stream=self)
        array.zero()
        if update_host:
            array.update_host()
        return array

    @trace
    def zeros_like(self, other, update_host=True):
        """Create a new OffloadArray that is bound to an empty numpy.ndarray
           that matches shape and type of an existing OffloadArray or
           numpy.ndarray.  If update_host is True (the default), then the empty
           array is automatically transferred to the host numpy.ndarray.  If
           set to False, the data transfer is avoided and the data in the host
           array is in undefined state.

           Parameters
           ----------
           other       : numpy.ndarray
               Template array used to determine shape, data type, and storage
               order
           update_host : bool, optional, default True
               Control if the host array is updated with the array data
               during construction of the OffloadArray

           Returns
           -------
           out : OffloadArray
               Instance of OffloadArray with all elements being zero

           See Also
           --------
           empty, empty_like, zeros, ones, ones_like, bcast, bcast_like

           Examples
           --------
           >>> a = numpy.ones((2,2))
           >>> stream.zeros_like(a)
           array([[ 0.,  0.],
                  [ 0.,  0.]])

           >>> o = stream.zeros_like(a, update_host=False)
           >>> o
           array([[  1.12110451e-316,   1.32102240e-312],
                  [  6.94088686e-310,   6.94088688e-310]])  # random data
           >>> o.update_host()
           array([[ 0.,  0.],
                  [ 0.,  0.]])
        """
        if (not isinstance(other, numpy.ndarray) 
                and not isinstance(other, pymic.OffloadArray)):
            raise ValueError("only numpy.ndarray can be used "
                             "with this function")
        return self.zeros(other.shape, other.dtype, get_order(other), 
                          update_host=update_host)

    @trace
    def ones(self, shape, dtype=numpy.float, order='C', update_host=True):
        """Create a new OffloadArray that is bound to a numpy.ndarray
           of the given shape and data type, and that is initialized with all
           elements set to one of the corresponding data type.  If update_host
           is True (the default), then the empty array is automatically
           transferred to the host numpy.ndarray.  If set to False, the
           data transfer is avoided and the data in the host array is in
           undefined state.

           Parameters
           ----------
           shape       : int or tuple of int
               Shape of the empty array
           dtype       : data-type, optional
               Desired output data-type of the empty array
           order       : {'C', 'F'}, optional, default 'C'
               Store multi-dimensional array data in row-major order 
               (C/C++, 'C') or column-major order (Fortran, 'F')
           update_host : bool, optional, default True
               Control if the host array is updated with the array data
               during construction of the OffloadArray

           Returns
           -------
           out : OffloadArray
               Instance of OffloadArray with all elements being one

           See Also
           --------
           empty, empty_like, zeros, zeros_like, ones_like, bcast, bcast_like

           Examples
           --------
           >>> stream.ones((2,2))
           array([[ 1.,  1.],
                  [ 1.,  1.]])

           >>> o = stream.ones((2,2), update_host=False)
           >>> o
           array([[  0.00000000e+000,  -3.13334375e-294],
                  [  2.77063169e-302,   1.16247550e-012]])  # random data
           >>> o.update_host()
           array([[ 1.,  1.],
                  [ 1.,  1.]])
        """


        array = pymic.OffloadArray(shape, dtype, order, device=self._device, 
                                   stream=self)
        array.one()
        if update_host:
            array.update_host()
        return array

    @trace
    def ones_like(self, other, update_host=True):
        """Create a new OffloadArray that is bound to an empty numpy.ndarray
           that matches shape and type of an existing OffloadArray or
           numpy.ndarray.  If update_host is True (the default), then the empty
           array is automatically transferred to the host numpy.ndarray.  If
           set to False, the data transfer is avoided and the data in the host
           array is in undefined state.

           Parameters
           ----------
           other       : numpy.ndarray
               Template array used to determine shape, data type, and storage
               order
           update_host : bool, optional, default True
               Control if the host array is updated with the array data
               during construction of the OffloadArray

           Returns
           -------
           out : OffloadArray
               Instance of OffloadArray with all elements being one

           See Also
           --------
           empty, empty_like, zeros, zeros_like, ones, bcast, bcast_like

           Examples
           --------
           >>> a = numpy.zeros((2,2))
           >>> stream.ones_like(a)
           array([[ 1.,  1.],
                  [ 1.,  1.]])

           >>> o = stream.ones_like(a, update_host=False)
           >>> o
           array([[  1.29083780e-316,   1.32102240e-312],
                  [  6.94088686e-310,   6.94088688e-310]])  # random data
           >>> o.update_host()
           array([[ 1.,  1.],
                  [ 1.,  1.]])
        """
        if (not isinstance(other, numpy.ndarray) and 
                not isinstance(other, pymic.OffloadArray)):
            raise ValueError("only numpy.ndarray can be used "
                             "with this function")
        return self.ones(other.shape, other.dtype, get_order(other), 
                         update_host=update_host)

    @trace
    def bcast(self, value, shape, dtype=numpy.float, order='C', 
              update_host=True):
        """Create a new OffloadArray that is bound to a numpy.ndarray
           of the given shape and data type, and that is initialized with all
           elements set to a given value of the corresponding data type.  If
           update_host is True (the default), then the empty array is
           automatically transferred to the host numpy.ndarray.  If set to
           False, the data transfer is avoided and the data in the host array
           is in undefined state.

           Parameters
           ----------
           value       : scalar type
               Value to broadcast to all elements of the created array
           shape       : int or tuple of int
               Shape of the empty array
           dtype       : data-type, optional
               Desired output data-type of the empty array
           order       : {'C', 'F'}, optional, default 'C'
               Store multi-dimensional array data in row-major order 
               (C/C++, 'C') or column-major order (Fortran, 'F')
           update_host : bool, optional, default True
               Control if the host array is updated with the array data
               during construction of the OffloadArray

           Returns
           -------
           out : OffloadArray
               Instance of OffloadArray with all elements being set to a
               given value

           See Also
           --------
           empty, empty_like, zeros, zeros_like, ones, ones_like, bcast_like

           Examples
           --------
           >>> stream.bcast(42.0, (2,2))
           array([[ 42.,  42.],
                  [ 42.,  42.]])

           >>> o = stream.bcast(42.0, (2,2), update_host=False)
           >>> o
           array([[  0.00000000e+000,   3.24370112e+178],
                  [  2.53946330e-292,   6.52391278e+091]])  # random data
           >>> o.update_host()
           array([[ 42.,  42.],
                  [ 42.,  42.]])
        """
        array = pymic.OffloadArray(shape, dtype, order, device=self._device, 
                                   stream=self)
        array.fill(value)  # TODO: here the stream might become an issue
        if update_host:
            array.update_host()
        return array

    @trace
    def bcast_like(self, value, other, update_host=True):
        """Create a new OffloadArray that is bound to an empty numpy.ndarray
           that matches shape and type of an existing OffloadArray or
           numpy.ndarray.  If update_host is True (the default), then the empty
           array is automatically transferred to the host numpy.ndarray.  If
           set to False, the data transfer is avoided and the data in the host
           array is in undefined state.

           Parameters
           ----------
           value       : scalar type
               Value to broadcast to all elements of the created array
           other       : numpy.ndarray
               Template array used to determine shape, data type, and storage
               order
           update_host : bool, optional, default True
               Control if the host array is updated with the array data
               during construction of the OffloadArray

           Returns
           -------
           out : OffloadArray
               Instance of OffloadArray with all elements being one

           See Also
           --------
           empty, empty_like, zeros, zeros_like, ones, bcast, bcast_like

           Examples
           --------
           >>> a = numpy.zeros((2,2))
           >>> stream.bcast_like(42.0, a)
           array([[ 1.,  1.],
                  [ 1.,  1.]])

           >>> o = stream.bcast_like(42.0, a, update_host=False)
           >>> o
           array([[  0.00000000e+000,   1.32102240e-312],
                  [  1.43679049e+161,   1.16250014e-012]])  # random data
           >>> o.update_host()
           array([[ 42.,  42.],
                  [ 42.,  42.]])
        """
        if (not isinstance(other, numpy.ndarray) and 
                not isinstance(other, pymic.OffloadArray)):
            raise ValueError("only numpy.ndarray can be used "
                             "with this function")
        return self.bcast(value, other.shape, other.dtype, 
                          get_order(other), update_host=update_host)

    def get_device(self):
        """Return the device this stream object is allocated for.
        
           Parameters
           ----------
           n/a
            
           Returns
           -------
           OffloadStream : 
               Instance of the OffloadStream object.

           See Also
           --------
           n/a
        """    
        return self._device
