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

# our own imports 
import offload_device as od
from _misc import _debug as debug
from _misc import _deprecated as deprecated

# stuff that we depend on 
import numpy as np

# TODO:
#   - find out how to easily copy the whole np.array interface
#   - support async operations

class offload_array(object):
    """An offloadable array structure to perform array-based computation
       on an Intel(R) Xeon Phi(tm) Coprocessor
       
       The interface is largely numpy-alike.  All operators execute their
       respective operation in an element-wise fashion.
    """   
    
    array = None
    
    def __init__(self, shape, dtype, order="C", alloc_arr=True, base=None, device=None):
        # allocate the array on the coprocessor
        self.order = order
        self.dtype = np.dtype(dtype)
        self.base = base
    
        # save a reference to the device
        assert device is not None
        self.device = device
    
        # determine size of the array from its shape
        try:
            size = 1
            for d in shape:
                size *= d
        except TypeError:
            assert isinstance(shape, (int, long, np.integer))
            size = shape
            shape = (shape,)
        self.size = size
        self.shape = shape
        self.nbytes = self.dtype.itemsize * self.size
        
        if base is not None:
            self.array = base.array.reshape(shape)
        else:
            if alloc_arr:
                self.array = np.empty(self.shape, self.dtype, self.order)
                self.device.buffer_allocate(self.array)
        
    def __del__(self):
        # deallocate storage in the target if this array goes away
        if self.base is None:
            self.device.buffer_release(self.array)
        
    def __str__(self):
        return str(self.array)
    
    def __repr__(self):
        return repr(self.array)
    
    def __hash__(self):
        raise TypeError("An offload_array is not hashable.")
    
    def update_device(self):
        self.device.buffer_update_on_target(self.array)
        return None
        
    def update_host(self):
        self.device.buffer_update_on_host(self.array)
        return self
        
    def __add__(self, other):
        """Add an array or scalar to an array."""

        n = int(self.size)
        x = self.array
        incx = int(1)
        if isinstance(other, offload_array):
            print self.array.shape
            print other.array.shape
            if self.array.shape != other.array.shape:
                raise ValueError("shapes of the arrays need to match ("
                                 + str(self.array.shape) + " != " 
                                 + str(other.array.shape) + ")")
            y = other.array
            incy = int(1)
            incr = int(1)
        else:
            # array + scalar
            if other == 0:
                return self
            else:
                y = float(other)
                incy = int(0)
                incr = int(1)
        result = offload_array(self.shape, self.dtype, device=self.device)
        self.device.invoke_kernel("offload_array_dadd", 
                             n, x, incx, y, incy, result.array, incr)
        return result

    def __sub__(self, other):
        """Subtract an array or scalar from an array."""
        
        n = int(self.size)
        x = self.array
        incx = int(1)
        if isinstance(other, offload_array):
            if self.array.shape != other.array.shape:
                raise ValueError("shapes of the arrays need to match ("
                                 + str(self.array.shape) + " != " 
                                 + str(other.array.shape) + ")")
            y = other.array
            incy = int(1)
            incr = int(1)
        else:
            # array + scalar
            if other == 0:
                return self
            else:
                y = float(other)
                incy = int(0)
                incr = int(1)
        result = offload_array(self.shape, self.dtype, device=self.device)
        self.device.invoke_kernel("offload_array_dsub", 
                             n, x, incx, y, incy, result.array, incr)
        return result

    def __mul__(self, other):
        """Multiply an array or a scalar with an array."""

        n = int(self.size)
        x = self.array
        incx = int(1)
        if isinstance(other, offload_array):
            if self.array.shape != other.array.shape:
                raise ValueError("shapes of the arrays need to match ("
                                 + str(self.array.shape) + " != " 
                                 + str(other.array.shape) + ")")
            y = other.array
            incy = int(1)
            incr = int(1)
        else:
            # array + scalar
            if other == 0:
                return self
            else:
                y = float(other)
                incy = int(0)
                incr = int(1)
        result = offload_array(self.shape, self.dtype, device=self.device)
        self.device.invoke_kernel("offload_array_dmul", 
                             n, x, incx, y, incy, result.array, incr)
        return result

    def fill(self, value):
        """Fill an array with the specified value."""
        
        n = int(self.size)
        x = self
        value = float(value)
        
        self.device.invoke_kernel("offload_array_dfill", n, x, value);
        return self    
    
    def fillfrom(self, array):
        """Fill an array from a numpy.ndarray."""
        
        if not isinstance(array, np.ndarray):
            raise TypeError("only numpy.ndarray supported")
        
        if self.shape != array.shape:
            print self.shape
            print array.shape
            raise TypeError("shapes of arrays to not match")

        # update the host part of the buffer and then update the device
        self.array[:] = array[:]
        self.update_device()
        
        return self
    
    def zero(self):
        """Fill the array with zeros."""
        return fill(float(0))
    
    def __len__(self):
        """Return the of size of the leading dimension."""
        if len(self.shape):
            return self.shape[0]
        else:
            return 1
        
    def __abs__(self):
        """Return a new offload_array with the absolute values of the elements of
           `self`."""
        
        n = int(self.array.size)
        x = self.array
        result = np.empty_like(self.array)
        
        self.device.buffer_allocate(result)
        self.device.invoke_kernel("offload_array_dabs", n, x, result);
        return offload_array(result, False, device=self.device)
        
    def __pow__(self, other):
        """Element-wise pow() function."""
        
        n = int(self.array.size)
        x = self.array
        incx = int(1)
        if isinstance(other, offload_array):
            assert self.array.shape == other.array.shape
            # array + array
            assert other.array.ndim == self.array.ndim
            assert other.array.size == self.array.size
            y = other.array
            incy = int(1)
            result = np.empty_like(self.array)
            incr = int(1)
        else:
            # array + scalar
            if other == 0:
                return self
            else:
                y = float(other)
                incy = int(0)
                result = np.empty_like(self.array)
                incr = int(1)
        self.device.buffer_allocate(result)
        self.device.invoke_kernel("offload_array_dpow", n, x, incx, y, incy, result, incr)
        return offload_array(result, False, device=self.device)
        
    def reverse(self):
        """Return a new offload_array with all elements in reverse order."""
        
        n = int(self.array.size)
        x = self.array
        result = np.empty_like(self.array)
        
        self.device.buffer_allocate(result)
        self.device.invoke_kernel("offload_array_dreverse", n, x, result);
        return offload_array(result, False, device=self.device)

    def reshape(self, *shape):
        if isinstance(shape[0], tuple) or isinstance(shape[0], list):
            shape = tuple(shape[0])
        # determine size of the array from its shape
        try:
            size = 1
            for d in shape:
                size *= d
        except TypeError:
            assert isinstance(shape, (int, long, np.integer))
            size = shape
            shape = (shape,)
        if size != self.size:
            raise ValueError("total size of reshaped array must be unchanged")
        return offload_array(shape, self.dtype, self.order, 
                             False, self, device=self.device)

    def ravel(self):
        return self.reshape(self.size)
        
if __name__ == '__main__':
    # some simple tests, remove them ASAP
    arr1 = np.arange(1.0,17.0)

    offl_arr1 = offload_array(16, np.float64)
    offl_arr2 = offload_array(16, np.float64)
    offl_arr1.fillfrom(arr1)
    offl_arr2.fillfrom(arr1)
    print offl_arr1.update_host()
    
    offl_arr3 = offl_arr1 + 1.0
    print offl_arr3.update_host()

    offl_arr4 = offl_arr1 * 2.0
    print offl_arr4.update_host()
    
    offl_arr5 = offl_arr4.reshape(4,4)
    print offl_arr5
    
    offl_arr6 = offl_arr5 - 0.5
    # print offl_arr6.update_host()

    offl_arr7 = offl_arr6.ravel()
    print offl_arr7.update_host()
    
    offl_arr8 = offl_arr7 + offl_arr2
    print offl_arr8.update_host()