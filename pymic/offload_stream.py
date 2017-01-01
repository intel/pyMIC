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

from pymic.offload_error import OffloadError

from pymic._engine import pymic_stream_create
from pymic._engine import pymic_stream_destroy
from pymic._engine import pymic_stream_sync
from pymic._engine import pymic_stream_allocate
from pymic._engine import pymic_stream_deallocate
from pymic._engine import pymic_stream_translate_device_pointer
from pymic._engine import pymic_stream_memcpy_h2d
from pymic._engine import pymic_stream_memcpy_d2h
from pymic._engine import pymic_stream_memcpy_d2d
from pymic._engine import pymic_stream_invoke_kernel

from pymic._misc import _debug as debug
from pymic._misc import _get_order as get_order
from pymic._misc import _DeviceAllocation as DeviceAllocation
from pymic._misc import _map_data_types as map_data_types
from pymic._tracing import _trace as trace

import pymic
import numpy


class OffloadStream:
    """
    """

    def __init__(self, device=None):
        # save a reference to the device
        assert device is not None
        self._device = device
        self._device_id = device.device_id

        # construct the stream
        self._stream_id = pymic_stream_create(self._device_id, 'stream')
        debug(1,
              'created stream 0x{0:x} for device {1}'.format(self._stream_id,
                                                             self._device_id))

    def __del__(self):
        debug(1,
              'destroying stream 0x{0:0x} for device {1}',
              self._stream_id, self._device_id)
        if self._device_id is not None:
            pymic_stream_destroy(self._device_id, self._stream_id)

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
           n/a

           See Also
           --------
           n/a
        """
        debug(2, 'syncing stream 0x{0:x} on device {1}',
                 self._stream_id, self._device_id)
        pymic_stream_sync(self._device_id, self._stream_id)
        return None

    def __eq__(self, other):
        return (self._device == other.device and
                self._stream_id == other.stream_id)

    def __repr__(self):
        return "OffloadStream({0}) for {1}".format(self._stream_id,
                                                   self._device.__repr__())

    def __str__(self):
        return "stream({0}) for {1}".format(self._stream_id, str(self._device))

    def allocate_device_memory(self, nbytes, alignment=64, sticky=False):
        """Allocate device memory on device associated with the invoking
           stream object.  Though it is part of the stream interface,
           the operation is synchronous.

           Caution: this is a low-level function, do not use it unless you
                    have a very specific reason to do so.  Better use the
                    high-level interfaces of OffloadArray instead.

           Parameters
           ----------
           nbytes : int
              Number of bytes to allocate
           alignment : int
              Alignment of the data on the target device.

           See Also
           --------
           deallocate_device_memory, transfer_host2device,
           transfer_device2host, transfer_device2device

           Returns
           -------
           out : int
              Fake pointer that identifies the allocated memory

           Examples
           --------
           >>> ptr = stream.allocate_device_memory(4096)
           >>> print ptr
           140297169571840
        """

        device = self._device_id

        if nbytes <= 0:
            raise ValueError('Cannot allocate negative amount of '
                             'memory: {0}'.format(nbytes))

        device_ptr = pymic_stream_allocate(device, self._stream_id,
                                           nbytes, alignment)
        device_ptr = DeviceAllocation(self, device, device_ptr, sticky)
        debug(2, 'allocated {0} bytes on device {1} at {2}'
                 ', alignment {3}',
                 nbytes, device, device_ptr, alignment)
        return device_ptr

    def deallocate_device_memory(self, device_ptr):
        """Deallocate device memory previously allocated through
           allocate_device_memory.  Though it is part of the stream
           interface, the operation is synchronous.

           Caution: this is a low-level function, do not use it unless you
                    have a very specific reason to do so.  Better use the
                    high-level interfaces of OffloadArray instead.

           Parameters
           ----------
           device_ptr : int
              Fake pointer of memory do deallocate

           See Also
           --------
           allocate_device_memory, transfer_host2device,
           transfer_device2host, transfer_device2device

           Returns
           -------
           None

           Examples
           --------
           >>> ptr = stream.allocate_device_memory(4096)
           >>> stream.deallocate_device_memory(ptr)
        """

        device = self._device

        if device_ptr is None:
            raise ValueError('Cannot deallocate None pointer')
        if not isinstance(device_ptr, DeviceAllocation):
            raise ValueError('Wrong argument, no device pointer given')
        # TODO: add more safety checks here (e.g., pointer from right device
        #       and stream)

        pymic_stream_deallocate(self._device_id, self._stream_id,
                                device_ptr._device_ptr)
        debug(2, 'deallocated pointer {0} on device {1}',
                 device_ptr, device)

        return None

    def transfer_host2device(self, host_ptr, device_ptr,
                             nbytes, offset_host=0, offset_device=0):
        """Transfer data from a host memory location (identified by its
           raw pointer (i.e., a C pointer) to a memory region (identified
           by its fake pointer) on the target device.  The operation is
           executed asynchronously with stream semantics.

           Caution: this is a low-level function, do not use it unless you
                    have a very specific reason to do so.  Better use the
                    high-level interfaces of OffloadArray instead.

           Parameters
           ----------
           host_ptr : int
              Pointer to the data on the host
           device_ptr : int
              Fake pointer of the destination
           nbytes : int
              Number of bytes to copy
           offset_host : int, optional, default 0
              Transfer offset (bytes) to be added to raw host pointer
           offset_device : int, optional, default 0
              Transfer offset (bytes) to be added to the address of the device
              memory.

           See Also
           --------
           transfer_device2host, transfer_device2device,
           allocate_device_memory, deallocate_device_memory

           Returns
           -------
           None

           Examples
           --------
           >>> a = numpy.arange(0.0, 16.0)
           >>> nbytes = a.dtype.itemsize * a.size
           >>> ptr_a_host = a.ctypes.data
           >>> device_ptr = stream.allocate_device_memory(nbytes)
           >>> stream.transfer_host2device(ptr_a_host, device_ptr, nbytes)
           >>> b = numpy.empty_like(a)
           >>> print b
           [  6.90762927e-310   7.73120247e-317   3.60667921e-322
              0.00000000e+000   0.00000000e+000   0.00000000e+000
              0.00000000e+000   0.00000000e+000   4.94065646e-324
              9.76815212e-317   7.98912845e-317   0.00000000e+000
              5.53353523e-322   1.58101007e-322   0.00000000e+000
              7.38839996e-317]
           # random data
           >>> ptr_b_host = b.ctypes.data
           >>> stream.transfer_device2host(device_ptr, ptr_b_host, nbytes)
           >>> stream.sync()
           >>> print b
           [  0.   1.   2.   3.   4.   5.   6.   7.   8.   9.  10.  11.
             12.  13.  14.  15.]
        """

        if not isinstance(device_ptr, DeviceAllocation):
            raise ValueError('Wrong argument, no device pointer given')
        # TODO: add more safety checks here (e.g., pointer from right device
        #       and stream)
        if offset_host < 0:
            raise ValueError("Negative offset passed for offset_host")
        if offset_device < 0:
            raise ValueError("Negative offset passed for offset_device")
        if device_ptr is None:
            raise ValueError('Invalid None device pointer')
        if host_ptr is None:
            raise ValueError('Invalid None host pointer')
        if nbytes <= 0:
            raise ValueError('Invalid byte count: {0}'.format(nbytes))

        debug(1, '(host -> device {0}) transferring {1} bytes '
                 '(host ptr 0x{2:x}, device ptr {3})',
                 self._device_id, nbytes, host_ptr, device_ptr)
        device_ptr = device_ptr._device_ptr
        pymic_stream_memcpy_h2d(self._device_id, self._stream_id,
                                host_ptr, device_ptr,
                                nbytes, offset_host, offset_device)
        return None

    def transfer_device2host(self, device_ptr, host_ptr,
                             nbytes, offset_device=0, offset_host=0):
        """Transfer data from a device memory location (identified by its
           fake pointer) to a host memory region identified by its raw pointer
           (i.e., a C pointer)on the target device. The operation is executed
           asynchronously with stream semantics.

           Caution: this is a low-level function, do not use it unless you
                    have a very specific reason to do so.  Better use the
                    high-level interfaces of OffloadArray instead.

           Parameters
           ----------
           host_ptr : int
              Pointer to the data on the host
           device_ptr : int
              Fake pointer of the destination
           nbytes : int
              Number of bytes to copy
           offset_device : int, optional, default 0
              Transfer offset (bytes) to be added to the address of the device
              memory.
           offset_host : int, optional, default 0
              Transfer offset (bytes) to be added to raw host pointer

           See Also
           --------
           transfer_host2device, transfer_device2device,
           allocate_device_memory, deallocate_device_memory

           Returns
           -------
           None

           Examples
           --------
           >>> a = numpy.arange(0.0, 16.0)
           >>> nbytes = a.dtype.itemsize * a.size
           >>> ptr_a_host = a.ctypes.data
           >>> device_ptr = stream.allocate_device_memory(nbytes)
           >>> stream.transfer_host2device(ptr_a_host, device_ptr, nbytes)
           >>> b = numpy.empty_like(a)
           >>> print b
           [  6.90762927e-310   7.73120247e-317   3.60667921e-322
              0.00000000e+000   0.00000000e+000   0.00000000e+000
              0.00000000e+000   0.00000000e+000   4.94065646e-324
              9.76815212e-317   7.98912845e-317   0.00000000e+000
              5.53353523e-322   1.58101007e-322   0.00000000e+000
              7.38839996e-317]
           # random data
           >>> ptr_b_host = b.ctypes.data
           >>> stream.transfer_device2host(device_ptr, ptr_b_host, nbytes)
           >>> stream.sync()
           >>> print b
           [  0.   1.   2.   3.   4.   5.   6.   7.   8.   9.  10.  11.
             12.  13.  14.  15.]
        """

        if not isinstance(device_ptr, DeviceAllocation):
            raise ValueError('Wrong argument, no device pointer given')
        # TODO: add more safety checks here (e.g., pointer from right device
        #       and stream)
        if offset_device < 0:
            raise ValueError("Negative offset passed for offset_device")
        if offset_host < 0:
            raise ValueError("Negative offset passed for offset_host")
        if device_ptr is None:
            raise ValueError('Invalid None device pointer')
        if host_ptr is None:
            raise ValueError('Invalid None host pointer')
        if nbytes <= 0:
            raise ValueError('Invalid byte count: {0}'.format(nbytes))

        debug(1, '(device {0} -> host) transferring {1} bytes '
                 '(device ptr {2}, host ptr 0x{3:x})',
                 self._device_id, nbytes, device_ptr, host_ptr)
        device_ptr = device_ptr._device_ptr
        pymic_stream_memcpy_d2h(self._device_id, self._stream_id,
                                device_ptr, host_ptr,
                                nbytes, offset_device, offset_host)
        return None

    def transfer_device2device(self, device_ptr_src, device_ptr_dst,
                               nbytes, offset_device_src=0,
                               offset_device_dst=0):
        """Transfer data from a device memory location (identified by its
           fake pointer) to another memory region on the same device. The
           operation is executed asynchronously with stream semantics.

           Caution: this is a low-level function, do not use it unless you
                    have a very specific reason to do so.  Better use the
                    high-level interfaces of OffloadArray instead.

           Parameters
           ----------
           device_ptr_src : int
              Fake pointer to the source memory location
           device_ptr_dst : int
              Fake pointer to the destination memory location
           nbytes : int
              Number of bytes to copy
           offset_device_src : int, optional, default 0
              Transfer offset (bytes) to be added to the address of the device
              memory (source).
           offset_device_dst : int, optional, default 0
              Transfer offset (bytes) to be added to the address of the device
              memory (destination).

           See Also
           --------
           transfer_host2device, allocate_device_memory,
           deallocate_device_memory

           Returns
           -------
           None

           Examples
           --------
           >>> a = numpy.arange(0.0, 16.0)
           >>> nbytes = a.dtype.itemsize * a.size
           >>> ptr_a_host = a.ctypes.data
           >>> device_ptr_1 = stream.allocate_device_memory(nbytes)
           >>> stream.transfer_host2device(ptr_a_host, device_ptr_1, nbytes)
           >>> device_ptr_2 = stream.allocate_device_memory(nbytes)
           >>> stream.transfer_device2device(device_ptr_1, device_ptr_2,
                                             nbytes)
           >>> b = numpy.empty_like(a)
           [  6.95303066e-310   6.83874600e-317   3.95252517e-322
              0.00000000e+000   9.31741387e+242   0.00000000e+000
              0.00000000e+000   0.00000000e+000   4.94065646e-324
              3.30519641e-317   1.72409659e+212   1.20070123e-089
              5.05907223e-085   4.87883721e+199   0.00000000e+000
              6.78545805e-317]
           # random data
           >>> print b
           >>> ptr_b_host = b.ctypes.data
           >>> stream.transfer_device2host(device_ptr_2, ptr_b_host, nbytes)
           >>> stream.sync()
           >>> print b
           [  0.   1.   2.   3.   4.   5.   6.   7.   8.   9.  10.  11.
             12.  13.  14.  15.]
        """

        if not isinstance(device_ptr_src, DeviceAllocation):
            raise ValueError('Wrong argument, no device pointer given')
        # TODO: add more safety checks here (e.g., pointer from right device
        #       and stream)
        if not isinstance(device_ptr_dst, DeviceAllocation):
            raise ValueError('Wrong argument, no device pointer given')
        # TODO: add more safety checks here (e.g., pointer from right device
        #       and stream)
        if offset_device_src < 0:
            raise ValueError("Negative offset passed for offset_device_src")
        if offset_device_dst < 0:
            raise ValueError("Negative offset passed for offset_device_dst")
        if device_ptr_src is None:
            raise ValueError('Invalid None device pointer')
        if device_ptr_dst is None:
            raise ValueError('Invalid None host pointer')
        if nbytes <= 0:
            raise ValueError('Invalid byte count: {0}'.format(nbytes))

        device_ptr_src = device_ptr_src._device_ptr
        device_ptr_dst = device_ptr_dst._device_ptr
        debug(1, '(device {0} -> device {0}) transferring {1} bytes '
                 '(source ptr {2}, destination ptr {3})',
                 self._device_id, nbytes, device_ptr_src, device_ptr_dst)
        pymic_stream_memcpy_d2d(self._device_id, self._stream_id,
                                device_ptr_src, device_ptr_dst,
                                nbytes, offset_device_src,
                                offset_device_dst)
        return None

    @trace
    def translate_device_pointer(self, device_ptr):
        """Translate a fake pointer to a real raw pointer on the target device.
           Though it is part of the stream interface, the operation is
           synchronous and automatically invokes OffloadStream.sync().

           Caution: this is a low-level function, do not use it unless you
                    have a very specific reason to do so.  Better use the
                    high-level interfaces of OffloadArray instead.

           Parameters
           ----------
           device_ptr : fake pointer
              Fake pointer to the memory location

           See Also
           --------
           allocate_device_memory, deallocate_device_memory

           Returns
           -------
           out : int
              Translated pointer

           Examples
           --------
           >>> device_ptr = stream.allocate_device_memory(nbytes)
           >>> print device_ptr
           0x7ff74c04d000+0    # random data
           >>> translated = stream.translate_device_pointer(device_ptr)
           >>> print translated
           140702047573768     # random data
        """

        if device_ptr is None:
            return 0

        device_id = device_ptr._stream._device_id
        if device_id != self._device_id:
            raise OffloadError('Device pointer for device {0} does not belong '
                               'to device {1}.'.format(device_id,
                                                       self._device_id))

        ptr = device_ptr._device_ptr
        translated = pymic_stream_translate_device_pointer(self._device_id,
                                                           self._stream_id,
                                                           ptr)
        self.sync()

        return translated

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

           All operations (copy in/copy out and invocation) are enqueued into
           the stream object and complete asynchronously.

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

        # throw an exception if the number of kernel arguments is more than
        # 16 (that's a limitation of libxstream at the moment)
        if len(args) > 16:
            raise ValueError("Kernels with more than 16 arguments "
                             "are not supported")

        # safety check: avoid invoking a kernel if it's library has been loaded
        # on a different device
        if kernel[2] is not self._device:
            raise OffloadError("Cannot invoke kernel, "
                               "library not loaded on device")

        # determine the types of the arguments (scalar vs arrays);
        # we store the device pointers as 64-bit integers in an ndarray
        arg_dims = numpy.empty((len(args),), dtype=numpy.int64)
        arg_type = numpy.empty((len(args),), dtype=numpy.int64)
        arg_ptrs = numpy.empty((len(args),), dtype=numpy.int64)
        arg_size = numpy.empty((len(args),), dtype=numpy.int64)
        copy_in_out = []
        scalars = []
        for i, a in enumerate(args):
            if a is None:
                # this is a None object, so we pass a nullptr to kernel
                arg_dims[i] = 1
                arg_type[i] = -1    # magic number to mark nullptrs
                arg_ptrs[i] = 0     # nullptr
                arg_size[i] = 0
                debug(3,
                      "(device {0}, stream 0x{1:x}) kernel '{2}' "
                      "arg {3} is None (device pointer 'nullptr')"
                      "".format(self._device_id, self._stream_id,
                                kernel[0], i))
            elif isinstance(a, pymic.OffloadArray):
                # get the device pointer of the OffloadArray and
                # pass it to the kernel
                arg_dims[i] = 1
                arg_type[i] = map_data_types(a.dtype)
                arg_ptrs[i] = a._device_ptr._device_ptr  # fake pointer
                arg_size[i] = a._nbytes
                debug(3,
                      "(device {0}, stream 0x{1:x}) kernel '{2}' "
                      "arg {3} is offload array (device pointer "
                      "{4})".format(self._device_id, self._stream_id,
                                    kernel[0], i, a._device_ptr))
            elif isinstance(a, numpy.ndarray):
                # allocate device buffer on the target of the invoke
                # and mark the numpy.ndarray for copyin/copyout semantics
                host_ptr = a.ctypes.data  # raw C pointer to host data
                nbytes = a.dtype.itemsize * a.size
                dev_ptr = self.allocate_device_memory(nbytes)
                copy_in_out.append((host_ptr, dev_ptr, nbytes, a))
                arg_dims[i] = 1
                arg_type[i] = map_data_types(a.dtype)
                arg_ptrs[i] = dev_ptr._device_ptr    # fake pointer
                arg_size[i] = nbytes
                debug(3,
                      "(device {0}, stream 0x{1:x}) kernel '{2}' "
                      "arg {3} is copy-in/-out array (host pointer {4}, "
                      "device pointer "
                      "{5})".format(self._device_id, self._stream_id,
                                    kernel[0], i, host_ptr, dev_ptr))
            else:
                # this is a hack, but let's wrap scalars as numpy arrays
                cvtd = numpy.asarray(a)
                host_ptr = cvtd.ctypes.data  # raw C pointer to host data
                nbytes = cvtd.dtype.itemsize * cvtd.size
                scalars.append(cvtd)
                arg_dims[i] = 0
                arg_type[i] = map_data_types(cvtd.dtype)
                arg_ptrs[i] = host_ptr
                arg_size[i] = nbytes
                debug(3,
                      "(device {0}, stream 0x{1:x}) kernel '{2}' "
                      "arg {3} is scalar {4} (host pointer "
                      "{5})".format(self._device_id, self._stream_id,
                                    kernel[0], i, a, host_ptr))
        debug(1, "(device {0}, stream 0x{1:x}) invoking kernel '{2}' "
                 "(pointer 0x{3:x}) with {4} "
                 "argument(s) ({5} copy-in/copy-out, {6} scalars)",
                 self._device_id, self._stream_id, kernel[0], kernel[1],
                 len(args), len(copy_in_out), len(scalars))
        # iterate over the copyin arguments and transfer them
        for c in copy_in_out:
            self.transfer_host2device(c[0], c[1], c[2])
        pymic_stream_invoke_kernel(self._device_id, self._stream_id, kernel[1],
                                   len(args), arg_dims, arg_type, arg_ptrs,
                                   arg_size)
        # iterate over the copyout arguments, transfer them back
        for c in copy_in_out:
            self.transfer_device2host(c[1], c[0], c[2])
        if len(copy_in_out) != 0:
            self.sync()

    @trace
    def bind(self, array, update_device=True):
        """Create a new OffloadArray that is bound to an existing
           numpy.ndarray.  If update_device is False, then one needs to call
           update_device() to issue a data transfer to the target device.
           Otherwise, the OffloadArray will be in unspecified state on
           the target device.  Data transfers to the host overwrite the
           data in the bound numpy.ndarray.

           The operation is enqueued into the stream object and completes
           asynchronously.

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
        bound._device_ptr = self.allocate_device_memory(bound._nbytes)
        if update_device:
            bound.update_device()

        return bound

    @trace
    def copy(self, array, update_device=True):
        """Create a new OffloadArray that is a copy of an existing
           numpy.ndarray.  If update_device is False, then one needs to call
           update_device() to issue a data transfer to the target device.
           Otherwise, the OffloadArray will be in unspecified state on
           the target device.  Once the copy has been made, the original
           numpy.ndarray will not be modified by any data transfer operations.

           The operation is enqueued into the stream object and completes
           asynchronously.

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

           The operation is enqueued into the stream object and completes
           asynchronously.

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

           The operation is enqueued into the stream object and completes
           asynchronously.

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

           The operation is enqueued into the stream object and completes
           asynchronously.

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

           The operation is enqueued into the stream object and completes
           asynchronously.

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

           The operation is enqueued into the stream object and completes
           asynchronously.

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

           The operation is enqueued into the stream object and completes
           asynchronously.

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

           The operation is enqueued into the stream object and completes
           asynchronously.

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

           The operation is enqueued into the stream object and completes
           asynchronously.

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
