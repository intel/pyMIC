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

from cpython.version cimport PY_MAJOR_VERSION

from pymic.offload_error import OffloadError

from libc.stdint cimport int64_t


cdef extern from "libxstream/include/libxstream.h":
    ctypedef void libxstream_stream 
    int libxstream_get_ndevices(size_t *ndevices)
    int libxstream_stream_create(libxstream_stream **stream, int device, int priority, const char* name)
    int libxstream_stream_destroy(const libxstream_stream* stream)
    int libxstream_stream_wait(libxstream_stream *stream)
    int libxstream_mem_allocate(int device, void **memory, size_t size, size_t alignment)
    int libxstream_mem_deallocate(int device, const void *memory)
    int libxstream_memcpy_h2d(const void *host_mem, void *dev_mem, size_t size, libxstream_stream* stream)
    int libxstream_memcpy_d2h(const void *dev_mem, void *host_mem, size_t size, libxstream_stream* stream)
    int libxstream_memcpy_d2d(const void *src, void *dst, size_t size, libxstream_stream* stream)

cdef extern from "pymic_internal.h":
    ctypedef void libxstream_stream
    int pymic_internal_invoke_kernel(int device, libxstream_stream *stream, void *funcptr, size_t argc, const int64_t *dims, const int64_t *types, void **ptrs, const size_t *sizes)
    int pymic_internal_load_library(int device, const char *filename, int64_t *handle, char **tempfile)
    int pymic_internal_unload_library(int device, int64_t handle, const char *tempfile)
    int pymic_internal_find_kernel(int device, int64_t handle, const char *kernel_name, int64_t *kernel_ptr)
    int pymic_internal_translate_pointer(int device, libxstream_stream *stream, int64_t pointer, int64_t *translated)

################################################################################
cdef _c_pymic_get_ndevices():
    """Wrapper for libxstream_get_ndevices."""
    cdef size_t ndevices
    libxstream_get_ndevices(&ndevices)
    return <int>ndevices

def pymic_get_ndevices():
    return _c_pymic_get_ndevices()

################################################################################
cdef _c_pymic_stream_create(int device_id, const char *stream_name):
    cdef libxstream_stream *stream
    cdef int err
    err = libxstream_stream_create(&stream, device_id, 0, stream_name)
    if err != 0:
        raise OffloadError('Could not create stream for device {0}'.format(device_id))
    return <int64_t>stream
    
def pymic_stream_create(device_id, stream_name = 'stream'):
    if PY_MAJOR_VERSION > 2:
        if isinstance(stream_name, str):
            stream_name = bytes(stream_name, 'ascii')
    return _c_pymic_stream_create(device_id, stream_name)

################################################################################
cdef _c_pymic_stream_destroy(int device_id, int64_t stream_id):
    cdef const libxstream_stream *stream
    cdef int err
    stream = <const libxstream_stream *>stream_id
    err = libxstream_stream_destroy(stream)
    if err != 0:
        raise OffloadError('Could not destroy stream 0x{0:x} on device {1}'.format(stream_id, device_id))
    return None

def pymic_stream_destroy(device_id, stream_id):
    _c_pymic_stream_destroy(device_id, stream_id)
    return None
    
################################################################################
cdef _c_pymic_stream_sync(int device_id, int64_t stream_id):
    cdef libxstream_stream *stream
    stream = <libxstream_stream *>stream_id
    err = libxstream_stream_wait(stream)
    if err != 0:
        raise OffloadError('Could not sync stream 0x{0:x} on device {1}'.format(stream_id, device_id))
    return None

def pymic_stream_sync(device_id, stream_id):
    _c_pymic_stream_sync(device_id, stream_id)
    return None
    
################################################################################
cdef _c_pymic_stream_allocate(int device_id, size_t nbytes, size_t alignment):
    cdef void *device_ptr
    cdef int err
    err = libxstream_mem_allocate(device_id, &device_ptr, nbytes, alignment)
    if err != 0:
        raise OffloadError('Could not allocate memory on device {0}'.format(device_id))
    return <int64_t>device_ptr

def pymic_stream_allocate(device_id, stream_id, nbytes, alignment):
    return _c_pymic_stream_allocate(device_id, nbytes, alignment)

################################################################################
cdef _c_pymic_stream_deallocate(int device_id, int64_t device_ptr):
    cdef void *ptr
    cdef err
    ptr = <void *>device_ptr
    err = libxstream_mem_deallocate(device_id, ptr)
    if err != 0:
        raise OffloadError('Could not deallocate memory on device {0}'.format(device_id))
    return None

def pymic_stream_deallocate(device_id, stream_id, device_ptr):
    _c_pymic_stream_deallocate(device_id, device_ptr)
    return None
    
################################################################################
cdef _c_pymic_stream_translate_device_pointer(int device_id, int64_t stream_id, int64_t device_ptr):
    cdef libxstream_stream *stream
    cdef int64_t translated
    cdef err
    stream = <libxstream_stream *>stream_id
    err = pymic_internal_translate_pointer(device_id, stream, device_ptr, &translated)
    if err != 0:
        raise OffloadError('Could not translate pointer for device {0}'.format(device_id))
    return translated

def pymic_stream_translate_device_pointer(device_id, stream_id, device_ptr):
    return _c_pymic_stream_translate_device_pointer(device_id, stream_id, device_ptr)

################################################################################
cdef _c_pymic_stream_memcpy_h2d(int device_id, int64_t stream_id, int64_t host_ptr, int64_t device_ptr, size_t nbytes):
    cdef libxstream_stream *stream
    cdef int err
    stream = <libxstream_stream *>stream_id
    err = libxstream_memcpy_h2d(<void *>host_ptr, <void *>device_ptr, nbytes, stream)
    if err != 0:
        raise OffloadError('Could not copy memory from host to device through stream 0x{0:x} on device {1}'.format(stream_id, device_id))
    return None

def pymic_stream_memcpy_h2d(device_id, stream_id, host_ptr, device_ptr,
                            nbytes, offset_host, offset_device):
    _c_pymic_stream_memcpy_h2d(device_id, stream_id, host_ptr + offset_host, device_ptr + offset_device, nbytes)
    return None

    
################################################################################
cdef _c_pymic_stream_memcpy_d2h(int device_id, int64_t stream_id, int64_t device_ptr, int64_t host_ptr, size_t nbytes):
    cdef libxstream_stream *stream
    cdef int err
    stream = <libxstream_stream *>stream_id
    err = libxstream_memcpy_d2h(<void *>device_ptr, <void *>host_ptr, nbytes, stream)
    if err != 0:
        raise OffloadError('Could not copy memory from device to host through stream 0x{0:x} on device {1}'.format(stream_id, device_id))
    return None

def pymic_stream_memcpy_d2h(device_id, stream_id, device_ptr, host_ptr,
                            nbytes, offset_device, offset_host):
    _c_pymic_stream_memcpy_d2h(device_id, stream_id, device_ptr + offset_device, host_ptr + offset_host, nbytes)
    return None
                            
    
################################################################################
cdef _c_pymic_stream_memcpy_d2d(int device_id, int64_t stream_id, int64_t device_ptr_src, int64_t device_ptr_dst, size_t nbytes):
    cdef libxstream_stream *stream
    cdef int err
    stream = <libxstream_stream *>stream_id
    err = libxstream_memcpy_d2d(<void *>device_ptr_src, <void *>device_ptr_dst, nbytes, stream)
    if err != 0:
        raise OffloadError('Could not copy memory from device to device through stream 0x{0:x} on device {1}'.format(stream_id, device_id))
    return None

def pymic_stream_memcpy_d2d(device_id, stream_id, device_ptr_src,
                            device_ptr_dst, nbytes, offset_device_src,
                            offset_device_dst):
    _c_pymic_stream_memcpy_d2d(device_id, stream_id, device_ptr_src + offset_device_src, device_ptr_dst + offset_device_dst, nbytes)
    return None
    
################################################################################
cdef _c_pymic_stream_invoke_kernel(int device_id, int64_t stream_id, int64_t kernel, 
                                   size_t argc, const int64_t *dims, const int64_t *types, void **ptrs, const size_t *sizes):
    cdef int err
    cdef libxstream_stream *stream
    stream = <libxstream_stream *>stream_id
    err = pymic_internal_invoke_kernel(device_id, stream, <void *>kernel, argc, dims, types, ptrs, sizes)
    if err != 0:
        raise OffloadError('Could not invoke kernel through stream 0x{0:x} on device {1}'.format(stream_id, device_id))
    return None

def pymic_stream_invoke_kernel(device_id, stream_id, kernel, argc, 
                               arg_dims, arg_types, arg_ptrs, arg_sizes):
    cdef int64_t arg_dims_ptr 
    cdef int64_t arg_types_ptr
    cdef int64_t arg_ptrs_ptr
    cdef int64_t arg_sizes_ptr
    
    arg_dims_ptr = arg_dims.ctypes.data
    arg_types_ptr = arg_types.ctypes.data
    arg_ptrs_ptr = arg_ptrs.ctypes.data
    arg_sizes_ptr = arg_sizes.ctypes.data
    
    _c_pymic_stream_invoke_kernel(device_id, stream_id, kernel, argc, <int64_t*>arg_dims_ptr, <int64_t*>arg_types_ptr, <void**>arg_ptrs_ptr, <size_t*>arg_sizes_ptr)
    return None
    
################################################################################
cdef _c_pymic_library_load(int device, const char *filename):
    cdef int err
    cdef int64_t handle
    cdef char *tempfile
    handle = 0
    err = pymic_internal_load_library(device, filename, &handle, &tempfile)
    if err != 0:
        raise OffloadError("Could not load library '{0}' on device {1}".format(<bytes>filename, device))
    return (handle, <bytes> tempfile)
    
def pymic_library_load(device_id, filename):
    if PY_MAJOR_VERSION > 2:
        if isinstance(filename, str):
            filename = bytes(filename, 'ascii')
    return _c_pymic_library_load(device_id, filename)

################################################################################
cdef _c_pymic_library_unload(int device_id, int64_t library_handle, const char *tempfile):
    cdef int err 
    err = pymic_internal_unload_library(device_id, library_handle, tempfile)
    if err != 0:
        raise OffloadError('Could not unload library on device {1}'.format(device_id))
    return None
    
def pymic_library_unload(device_id, library_handle, tempfile):
    if PY_MAJOR_VERSION > 2:
        if isinstance(tempfile, str):
            tempfile = bytes(tempfile, 'ascii')
    _c_pymic_library_unload(device_id, library_handle, tempfile)
    return None

################################################################################
cdef _c_pymic_library_find_kernel(int device_id, int64_t library_handle, const char *kernel_name):
    cdef int err
    cdef int64_t kernel_ptr
    kernel_ptr = 0
    err = pymic_internal_find_kernel(device_id, library_handle, kernel_name, &kernel_ptr)
    if err != 0:
        raise OffloadError("Could not find kernel '{0}' on device {1}".format(kernel_name, device_id))
    return kernel_ptr

def pymic_library_find_kernel(device_id, library_handle, kernel_name):
    if PY_MAJOR_VERSION > 2:
        if isinstance(kernel_name, str):
            kernel_name = bytes(kernel_name, 'ascii')
    return _c_pymic_library_find_kernel(device_id, library_handle, kernel_name)
