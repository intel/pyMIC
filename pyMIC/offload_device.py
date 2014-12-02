# Copyright (c) 2014, Intel Corporation All rights reserved. 
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

# general support functions
from _pyMICimpl import _pymic_impl_offload_number_of_devices
from _pyMICimpl import _pymic_impl_load_library

# data management functions
from _pyMICimpl import _pymic_impl_buffer_allocate
from _pyMICimpl import _pymic_impl_buffer_release
from _pyMICimpl import _pymic_impl_buffer_update_on_target
from _pyMICimpl import _pymic_impl_buffer_update_on_host
from _pyMICimpl import _pymic_impl_buffer_alloc_and_copy_to_target
from _pyMICimpl import _pymic_impl_buffer_copy_from_target_and_release

# kernel invocation functions
from _pyMICimpl import _pymic_impl_invoke_kernel

from _misc import _debug as debug
from _misc import _get_order as get_order
import pyMIC

import numpy

def number_of_devices():
    """Returns the number of devices available for offloading.
    
       Parameters
       ----------
              
       Returns
       -------
       out : int
           Number of offload devices in the system
       
       See Also
       --------
              
       Examples
       --------
       >>> pyMIC.number_of_devices()
       2
    """
    return _pymic_impl_offload_number_of_devices()

class offload_device:
    def __init__(self, device_id):
        self.device_id = device_id

    def __eq__(self, other):
        return self.device_id == device_id
        
    def __repr__(self):
        if self.device_id == any:
            s = "any"
        else:
            s = str(self.device_id)
        return "offload_device({0})".format(s)
        
    def __str__(self):
        if self.device_id == any:
            s = "any"
        else:
            s = str(self.device_id)
        return "target(mic:{0})".format(s)
    
    def _is_real():
        return self.device_id != any
    
    def _map_dev_id(self):
        if self.device_id == any:
            return -1
        else:
            return int(self.device_id)
    
    def buffer_allocate(self, *arrays):
        if len(arrays) == 0:
            raise ValueError("no argument given")
        # if called from wrapper, actual arguments are wrapped in an extra tuple
        if type(arrays[0]) == tuple:
            arrays = arrays[0]
        for array in arrays:
            nbytes = int(array.nbytes)
            debug(2, "(device {0}) allocating buffer with {1} byte(s)".format(self._map_dev_id(), nbytes))
            _pymic_impl_buffer_allocate(self._map_dev_id(), array, nbytes)
        return None
        
    def buffer_release(self, *arrays):
        if len(arrays) == 0:
            raise ValueError("no argument given")
        # if called from wrapper, actual arguments are wrapped in an extra tuple
        if type(arrays[0]) == tuple:
            arrays = arrays[0]
        for array in arrays:
            nbytes = int(array.nbytes)
            debug(2, "(device {0}) releasing buffer with {1} byte(s)".format(self._map_dev_id(), nbytes))
            _pymic_impl_buffer_release(self._map_dev_id(), array, nbytes)
        return None
        
    def buffer_update_on_target(self, *arrays):
        if len(arrays) == 0:
            raise ValueError("no argument given")
        # if called from wrapper, actual arguments are wrapped in an extra tuple
        if type(arrays[0]) == tuple:
            arrays = arrays[0]
        for array in arrays:
            nbytes = int(array.nbytes)
            debug(1, "(host -> device {0}) transferring {1} byte(s)".format(self._map_dev_id(), nbytes))
            _pymic_impl_buffer_update_on_target(self._map_dev_id(), array, nbytes)
        return None
        
    def buffer_update_on_host(self, *arrays):
        if len(arrays) == 0:
            raise ValueError("no argument given")
        # if called from wrapper, actual arguments are wrapped in an extra tuple
        if type(arrays[0]) == tuple:
            arrays = arrays[0]
        for array in arrays:
            nbytes = int(array.nbytes)
            debug(1, "(device {0} -> host) transferring {1} byte(s)".format(self._map_dev_id(), nbytes))
            _pymic_impl_buffer_update_on_host(self._map_dev_id(), array, nbytes)
        return None
        
    def copy_to_target(self, *arrays):
        if len(arrays) == 0:
            raise ValueError("no argument given")
        # if called from wrapper, actual arguments are wrapped in an extra tuple
        if type(arrays[0]) == tuple:
            arrays = arrays[0]
        for array in arrays:
            nbytes = int(array.nbytes)
            debug(2, "(device {0}) allocating buffer with {1} byte(s)".format(self._map_dev_id(), nbytes))
            debug(1, "(host -> device {0}) transferring {1} byte(s)".format(self._map_dev_id(), nbytes))
            _pymic_impl_buffer_alloc_and_copy_to_target(self._map_dev_id(), array, nbytes)
        return None
        
    def copy_from_target(self, *arrays):
        if len(arrays) == 0:
            raise ValueError("no argument given")
        # if called from wrapper, actual arguments are wrapped in an extra tuple
        if type(arrays[0]) == tuple:
            arrays = arrays[0]
        for array in arrays:
            nbytes = int(array.nbytes)
            debug(1, "(device {0} -> host) transferring {1} byte(s)".format(self._map_dev_id(), nbytes))
            debug(2, "(device {0}) releasing buffer with {1} byte(s)".format(self._map_dev_id(), nbytes))
            _pymic_impl_buffer_copy_from_target_and_release(self._map_dev_id(), array, nbytes)
        return None
    
    @staticmethod
    def _convert_argument_to_array(argument):
        """Internal helper function to convert scalar arguments to 
           single-element numpy.ndarray.
        """
        
        # convert the argument to a single-element array
        if (not isinstance(argument, numpy.ndarray)) and (not isinstance(argument, pyMIC.offload_array)):
            cvtd = numpy.asarray(argument)
            return cvtd
        else:
            return argument

    def load_library(self, *libraries):
        """Load one or multiple shared-object library that each contains one 
           or multiple native kernels for invocation through invoke_kernel.
           
           Parameters
           ----------
           libraries : str, typle of str, or list of str
               Names of the shared-object libraries to load on the device
           
           Returns
           -------
           None
           
           See Also
           --------
           invoke_kernel
           
           Examples
           --------
           >>> device.load_library("kernels1")
           >>> device.load_library("blas", "mykernels")
        """   
        
        if len(libraries) == 0:
            raise ValueError("no argument given")
        # if called from wrapper, actual arguments are wrapped in an extra tuple
        if type(libraries[0]) == tuple:
            libraries = libraries[0]
        for library in libraries:
            debug(5, "loading '{0}' on device {0}".format(library))
            _pymic_impl_load_library(self._map_dev_id(), library)
        return None
    
    def invoke_kernel(self, name, *args):
        """Invoke a native kernel on the target device.  The kernel is found
           by the name given in the name argument of invoke_kernel.  The kernel
           function needs to be in a shared-object library that has been 
           loaded by calling the load_library function before invoke_kernel.
           
           The additional arguments of invoke_kernel can be either instances
           of offload_array, numpy.ndarray, or scalar data.  For numpy.ndarray 
           or scalar arguments, invoke_kernel automatically performs copy-in and
           copy-out of the argument, that is, before the kernel is invoked, 
           the argument is automatically transferred to the target device and
           transferred back after the kernel has finished.  
           
           Parameters
           ----------
           name : str
              Name of the kernel to invoke
           args : offload_array, numpy.ndarray, or scalar type
              Arguments to be passed to the kernel function
           
           See Also
           --------
           load_library
           
           Returns
           ------- 
           None
           
           Examples
           --------
           >>> device.invoke_kernel("dgemm", A, B, C, n, m, k)
        """
        
        # if called from wrapper, actual arguments are wrapped in an extra tuple
        if len(args):
            if type(args[0]) == tuple:
                args = args[0]
        # for i in args:
            # print "##> " + str(type(i))
        converted = map(offload_device._convert_argument_to_array, args)
        # for i in converted:
            # print "--> " + str(type(i))
        debug(1, "(device {0}) invoking kernel '{1}' with {2} argument(s)".format(self._map_dev_id(), name, len(args)))
        _pymic_impl_invoke_kernel(self._map_dev_id(), name, tuple(converted))
        return None

    def bind(self, array, update_device=True):
        """Create a new offload_array that is bound to an existing 
           numpy.ndarray.  If update_device is False, then one needs to call
           update_device() to issue a data transfer to the target device.  
           Otherwise, the offload_array will be in unspecified state on 
           the target device.  Data transfers to the host overwrite the 
           data in the bound numpy.ndarray.
           
           Parameters
           ----------
           array         : numpy.ndarray
               Array to be bound to a new offload_array
           update_device : bool, optional, default True
               Control if the device buffer is updated with the array data
           
           Returns
           -------
           out : offload_array
               Instance of offload_array bound to the input array
           
           See Also
           --------
           copy
           
           Examples
           --------
           >>> a = numpy.zeros((2,2))
           >>> oa = bind(a)

           >>> b = numpy.ones((1,1))
           >>> ob = device.bind(b, update_device=False)
           >>> ob.update_device()
        """
        
        if not isinstance(array, numpy.ndarray):
            raise ValueError("only numpy.ndarray can be associated with offload_array")
        
        # detect the order of storage for 'array'
        if array.flags.c_contiguous:
            order = "C"
        elif array.flags.f_contiguous:
            order = "F"
        else:
            raise ValueError("could not detect storage order")
        
        # construct and return a new offload_array
        bound = pyMIC.offload_array(array.shape, array.dtype, order, False, device=self)
        bound.array = array
        
        # allocate the buffer on the device (and update data)
        if update_device:
            self.copy_to_target(bound.array)
        else:
            self.buffer_allocate(bound.array)
        
        return bound
    
    def copy(self, array, update_device=True):
        """Create a new offload_array that is a copy of an existing 
           numpy.ndarray.  If update_device is False, then one needs to call
           update_device() to issue a data transfer to the target device.  
           Otherwise, the offload_array will be in unspecified state on 
           the target device.  Once the copy has been made, the original 
           numpy.ndarray will not be modified by any data transfer operations.
           
           Parameters
           ----------
           array         : numpy.ndarray
               Array to be bound to a new offload_array
           update_device : bool, optional, default True
               Control if the device buffer is updated with the array data
           
           Returns
           -------
           out : offload_array
               Instance of offload_array with copied array
           
           See Also
           --------
           bind
           
           Examples
           --------
           >>> a = numpy.zeros((2,2))
           >>> oa = copy(a)

           >>> b = numpy.zeros(...)
           >>> ob = copy(b, False)
           >>> ob.update_device()
        """
    
        if not isinstance(array, numpy.ndarray):
            raise ValueError("only numpy.ndarray can be associated with offload_array")
        
        # detect the order of storage for 'array'
        if array.flags.c_contiguous:
            order = "C"
        elif array.flags.f_contiguous:
            order = "F"
        else:
            raise ValueError("could not detect storage order")

        array_copy = numpy.empty_like(array)
        array_copy[:] = array[:]
        return self.bind(array_copy, update_device)
    
    def empty(self, shape, dtype=numpy.float, order='C', update_host=True):
        """Create a new offload_array that is bound to an empty numpy.ndarray
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
               during construction of the offload_array

           Returns
           -------
           out : offload_array
               Instance of offload_array bound to the input array
               
           See Also
           --------
           empty_like, zeros, zeros_like, ones, ones_like, bcast, bcast_like
           
           Examples
           --------
           >>> device.empty((2,2))
           array([[  4.41694687e-321,   6.92646058e-310],
                  [  4.71338626e-321,   6.92646058e-310]])  # random data
           
           >>> o = device.empty((2,2), update_host=False)
           >>> o
           array([[  0.00000000e+000,  -3.13334375e-294],
                  [  2.77063169e-302,   1.76125728e-312]])  # random data
           >>> o.update_host()
           array([[  4.26872718e-321,   6.92050581e-310],
                  [  4.34283703e-321,   6.92050581e-310]])  # random data
        """
        array = pyMIC.offload_array(shape, dtype, order, device=self)
        if update_host:
            array.update_host()
        return array
        
    def empty_like(self, other, update_host=True):
        """Create a new offload_array that is bound to an empty numpy.ndarray
           that matches shape and type of an existing offload_array or 
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
               during construction of the offload_array

           See Also
           --------
           empty, zeros, zeros_like, ones, ones_like, bcast, bcast_like
           
           Examples
           --------
           >>> a = numpy.ones((2,2))
           >>> device.empty_like(a)
           array([[  4.26872718e-321,   6.91990916e-310],
                  [  4.34283703e-321,   6.91990916e-310]])  # random data
                  
           >>> o = device.empty_like(a, update_host=False)
           >>> o
           array([[  1.12110451e-316,   1.32102240e-312],
                  [  6.94088686e-310,   6.94088688e-310]])  # random data
           >>> o.update_host()
           array([[  4.26872718e-321,   6.95150287e-310],
                  [  4.34283703e-321,   6.95150287e-310]])  # random data
        """
        if not isinstance(other, numpy.ndarray) and not isinstance(other, pyMIC.offload_array):
            raise ValueError("only numpy.ndarray can be used with this function")
        return self.empty(other.shape, other.dtype, get_order(other), update_host)
            
    def zeros(self, shape, dtype=numpy.float, order='C', update_host=True):
        """Create a new offload_array that is bound to a numpy.ndarray
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
               Store multi-dimensional array data in row-major order (C/C++, 'C')
               or column-major order (Fortran, 'F')
           update_host : bool, optional, default True
               Control if the host array is updated with the array data
               during construction of the offload_array

           Returns
           -------
           out : offload_array
               Instance of offload_array with all elements being zero
               
           See Also
           --------
           empty, empty_like, zeros_like, ones, ones_like, bcast, bcast_like
           
           Examples
           --------
           >>> device.zeros((2,2))
           array([[ 0.,  0.],
                  [ 0.,  0.]])

           >>> o = device.zeros((2,2), update_host=False)
           >>> o
           array([[  0.00000000e+000,  -3.13334375e-294],
                  [  2.77063169e-302,   1.16247550e-012]])  # random data
           >>> o.update_host()
           array([[ 0.,  0.],
                  [ 0.,  0.]])
        """
        # TODO: how can we get rid of the explicit cast when calling "fill"
        array = pyMIC.offload_array(shape, dtype, order, device=self)
        array.fill(float(0.0))
        if update_host:
            array.update_host()
        return array

    def zeros_like(self, other, update_host=True):
        """Create a new offload_array that is bound to an empty numpy.ndarray
           that matches shape and type of an existing offload_array or 
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
               during construction of the offload_array
               
           Returns
           -------
           out : offload_array
               Instance of offload_array with all elements being zero
               
           See Also
           --------
           empty, empty_like, zeros, ones, ones_like, bcast, bcast_like

           Examples
           --------
           >>> a = numpy.ones((2,2))
           >>> device.zeros_like(a)
           array([[ 0.,  0.],
                  [ 0.,  0.]])
                  
           >>> o = device.zeros_like(a, update_host=False)
           >>> o
           array([[  1.12110451e-316,   1.32102240e-312],
                  [  6.94088686e-310,   6.94088688e-310]])  # random data
           >>> o.update_host()
           array([[ 0.,  0.],
                  [ 0.,  0.]])
        """
        if not isinstance(other, numpy.ndarray) and not isinstance(other, pyMIC.offload_array):
            raise ValueError("only numpy.ndarray can be used with this function")
        return self.zeros(other.shape, other.dtype, get_order(other), update_host)
        
    def ones(self, shape, dtype=numpy.float, order='C', update_host=True):
        """Create a new offload_array that is bound to a numpy.ndarray
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
               Store multi-dimensional array data in row-major order (C/C++, 'C')
               or column-major order (Fortran, 'F')
           update_host : bool, optional, default True
               Control if the host array is updated with the array data
               during construction of the offload_array

           Returns
           -------
           out : offload_array
               Instance of offload_array with all elements being one
               
           See Also
           --------
           empty, empty_like, zeros, zeros_like, ones_like, bcast, bcast_like
           
           Examples
           --------
           >>> device.ones((2,2))
           array([[ 1.,  1.],
                  [ 1.,  1.]])

           >>> o = device.ones((2,2), update_host=False)
           >>> o
           array([[  0.00000000e+000,  -3.13334375e-294],
                  [  2.77063169e-302,   1.16247550e-012]])  # random data
           >>> o.update_host()
           array([[ 1.,  1.],
                  [ 1.,  1.]])
        """
        # TODO: how can we get rid of the explicit cast when calling "fill"
        array = pyMIC.offload_array(shape, dtype, order, device=self)
        array.fill(float(1.0))
        if update_host:
            array.update_host()
        return array
        
    def ones_like(self, other, update_host=True):
        """Create a new offload_array that is bound to an empty numpy.ndarray
           that matches shape and type of an existing offload_array or 
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
               during construction of the offload_array
               
           Returns
           -------
           out : offload_array
               Instance of offload_array with all elements being one
               
           See Also
           --------
           empty, empty_like, zeros, zeros_like, ones, bcast, bcast_like

           Examples
           --------
           >>> a = numpy.zeros((2,2))
           >>> device.ones_like(a)
           array([[ 1.,  1.],
                  [ 1.,  1.]])
                  
           >>> o = device.ones_like(a, update_host=False)
           >>> o
           array([[  1.29083780e-316,   1.32102240e-312],
                  [  6.94088686e-310,   6.94088688e-310]])  # random data
           >>> o.update_host()
           array([[ 1.,  1.],
                  [ 1.,  1.]])
        """
        if not isinstance(other, numpy.ndarray) and not isinstance(other, pyMIC.offload_array):
            raise ValueError("only numpy.ndarray can be used with this function")
        return self.ones(other.shape, other.dtype, get_order(other), update_host)
        
    def bcast(self, value, shape, dtype=numpy.float, order='C', update_host=True):
        """Create a new offload_array that is bound to a numpy.ndarray
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
               Store multi-dimensional array data in row-major order (C/C++, 'C')
               or column-major order (Fortran, 'F')
           update_host : bool, optional, default True
               Control if the host array is updated with the array data
               during construction of the offload_array

           Returns
           -------
           out : offload_array
               Instance of offload_array with all elements being set to a
               given value
               
           See Also
           --------
           empty, empty_like, zeros, zeros_like, ones, ones_like, bcast_like
           
           Examples
           --------
           >>> device.bcast(42.0, (2,2))
           array([[ 42.,  42.],
                  [ 42.,  42.]])

           >>> o = device.bcast(42.0, (2,2), update_host=False)
           >>> o
           array([[  0.00000000e+000,   3.24370112e+178],
                  [  2.53946330e-292,   6.52391278e+091]])  # random data
           >>> o.update_host()
           array([[ 42.,  42.],
                  [ 42.,  42.]])
        """
        # TODO: how can we get rid of the explicit cast when calling "fill"
        array = pyMIC.offload_array(shape, dtype, order, device=self)
        array.fill(float(value))
        if update_host:
            array.update_host()
        return array
        
    def bcast_like(self, value, other, update_host=True):
        """Create a new offload_array that is bound to an empty numpy.ndarray
           that matches shape and type of an existing offload_array or 
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
               during construction of the offload_array
               
           Returns
           -------
           out : offload_array
               Instance of offload_array with all elements being one
               
           See Also
           --------
           empty, empty_like, zeros, zeros_like, ones, bcast, bcast_like

           Examples
           --------
           >>> a = numpy.zeros((2,2))
           >>> device.bcast_like(42.0, a)
           array([[ 1.,  1.],
                  [ 1.,  1.]])
                  
           >>> o = device.bcast_like(42.0, a, update_host=False)
           >>> o
           array([[  0.00000000e+000,   1.32102240e-312],
                  [  1.43679049e+161,   1.16250014e-012]])  # random data
           >>> o.update_host()
           array([[ 42.,  42.],
                  [ 42.,  42.]])
        """
        if not isinstance(other, numpy.ndarray) and not isinstance(other, pyMIC.offload_array):
            raise ValueError("only numpy.ndarray can be used with this function")
        return self.bcast(value, other.shape, other.dtype, get_order(other), update_host)

def _init_devices():
    """Internal function that is used to initialize the Python mapping of 
       physical offload targets to a Python dictionary.
    """
    no_of_devices = number_of_devices()
    debug(5, "found {0} device(s)".format(no_of_devices))
    device_dict = {any: offload_device(any)}
    for d in range(0, no_of_devices):
        device_dict[d] = offload_device(d)
    return device_dict

devices = _init_devices()           
