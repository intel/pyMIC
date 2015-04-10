/* Copyright (c) 2014-2015, Intel Corporation All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are 
 * met: 
 *
 * 1. Redistributions of source code must retain the above copyright 
 * notice, this list of conditions and the following disclaimer. 
 * 
 * 2. Redistributions in binary form must reproduce the above copyright 
 * notice, this list of conditions and the following disclaimer in the 
 * documentation and/or other materials provided with the distribution. 
 *
 * 3. Neither the name of the copyright holder nor the names of its 
 * contributors may be used to endorse or promote products derived from 
 * this software without specific prior written permission. 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS 
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED 
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A 
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED 
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <Python.h>
#include <numpy/arrayobject.h>

#include <cstdio>
#include <cstdarg>
#include <string>
#include <vector>
#include <utility>

#include "pymicimpl_common.h"
#include "pymicimpl_misc.h"

#include <libxstream.h>

#include "debug.h"

using namespace pymic;

PyObject *throw_exception(const char *format, ...) {
    va_list vargs;

    const size_t bufsz = 256;
    char buffer[bufsz];

    PyObject *pymic = NULL;
    PyObject *modules_dict = NULL;
    PyObject *exception_type = NULL;
    
    // get the modules dictionary
    modules_dict = PySys_GetObject("modules");
    if (!modules_dict) {
        Py_RETURN_NONE;
    }
    
    // find the pymic module
    pymic = PyDict_GetItemString(modules_dict, "pymic");
    if (!pymic) {
        Py_RETURN_NONE;
    }
    
    // now, find the OffloadException class
    exception_type = PyObject_GetAttrString(pymic, "OffloadError");
    if (!exception_type) {
        Py_RETURN_NONE;
    }

    // assemble the message
    va_start(vargs, format);
    vsnprintf(buffer, bufsz, format, vargs);
    
    // create (throw/register) the exception 
    PyObject *exception = PyErr_Format(exception_type, buffer);
    return exception;
}


PYMIC_INTERFACE 
PyObject *pymic_impl_offload_number_of_devices(PyObject *self, PyObject *args) {
	debug_enter();
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
    size_t no_of_devs = 0;
    if (libxstream_get_ndevices(&no_of_devs)) {
        // TODO: error handling
        printf("WOOOPS: error case at %s:%d\n", __FILE__, __LINE__);
    }
	debug(10, "detected %d devices", no_of_devs);
	debug_leave();
	return Py_BuildValue("I", static_cast<int>(no_of_devs));
}


PYMIC_INTERFACE
PyObject *pymic_impl_load_library(PyObject *self, PyObject *args) {
	int device;
    const char * filename_cstr = NULL;
    std::string filename;
    std::string tempname;
    uintptr_t handle = 0;
    debug_enter();
    
	if (! PyArg_ParseTuple(args, "is", &device, &filename_cstr))
		return NULL;
	filename = filename_cstr;
    
    try {
        target_load_library(device, filename, tempname, handle);
    }
    catch (internal_exception *exc) {
        // catch and convert exception (re-throw as a Python exception)
        debug(100, "internal exception raised at %s:%d, raising Python exception", 
                   exc->file(), exc->line());
        std::string reason = exc->reason();
        delete exc;
        debug_leave();
        return throw_exception("Could not transfer library to target. Reason: %s.",
                               reason.c_str());;
    }
    
    debug_leave();
    return Py_BuildValue("(sk)", tempname.c_str(), handle);
}

PYMIC_INTERFACE
PyObject *pymic_impl_unload_library(PyObject *self, PyObject *args) {
	int device;
    uintptr_t handle = 0;
    char * tempname_cstr;
    std::string tempname;
	debug_enter();
	
	if (! PyArg_ParseTuple(args, "isk", &device, &tempname_cstr, &handle))
		return NULL;
    tempname = tempname_cstr;
	
    try {
        target_unload_library(device, tempname, handle);
	}
    catch (internal_exception *exc) {
        // catch and convert exception (re-throw as a Python exception)
        debug(100, "internal exception raised at %s:%d, raising Python exception", 
                   exc->file(), exc->line());
        std::string reason = exc->reason();
        delete exc;
        debug_leave();
        return throw_exception("Could not unload library on target. Reason: %s.",
                               reason.c_str());;
    }
	
    debug_leave();
	Py_RETURN_NONE;
}


PYMIC_INTERFACE
PyObject *pymic_impl_stream_create(PyObject *self, PyObject *args) {
    debug_enter();
    int lowest, greatest;
    int device;
    char * name;
    libxstream_stream * stream = NULL;
    uintptr_t stream_id = 0;
    
	if (! PyArg_ParseTuple(args, "is", &device, &name))
		return NULL;

    if (libxstream_stream_create(&stream, device, 0, 0, name) != LIBXSTREAM_ERROR_NONE) {
        printf("WHOOOOP at %s:%d\n", __FUNCTION__, __LINE__);
        throw internal_exception("XSTREAM FAILED", __FILE__, __LINE__);
    }
    stream_id = reinterpret_cast<uintptr_t>(stream);

    debug(10, "using default stream for device %d", device);
    debug_leave();
    return Py_BuildValue("k", static_cast<uintptr_t>(stream_id));
}


PYMIC_INTERFACE
PyObject *pymic_impl_stream_destroy(PyObject *self, PyObject *args) {
    debug_enter();
    int device;
    uintptr_t stream_id;
    libxstream_stream * stream = NULL;

	if (! PyArg_ParseTuple(args, "ik", &device, &stream_id))
		return NULL;

    stream = reinterpret_cast<libxstream_stream *>(stream_id);
    if (libxstream_stream_destroy(stream) != LIBXSTREAM_ERROR_NONE) {
        printf("WHOOOOP at %s:%d\n", __FUNCTION__, __LINE__);
        throw internal_exception("XSTREAM FAILED", __FILE__, __LINE__);
    }
    
    debug_leave();
    Py_RETURN_NONE;
}


PYMIC_INTERFACE
PyObject *pymic_impl_stream_sync(PyObject *self, PyObject *args) {
    debug_enter();
    int device;
    uintptr_t stream_id;
    libxstream_stream * stream = NULL;

	if (! PyArg_ParseTuple(args, "ik", &device, &stream_id))
		return NULL;

    stream = reinterpret_cast<libxstream_stream *>(stream_id);
    if (libxstream_stream_sync(stream) != LIBXSTREAM_ERROR_NONE) {
        printf("WHOOOOP at %s:%d\n", __FUNCTION__, __LINE__);
        throw internal_exception("XSTREAM FAILED", __FILE__, __LINE__);
    }
    
    debug_leave();
    Py_RETURN_NONE;
}


PYMIC_INTERFACE
PyObject *pymic_impl_stream_allocate(PyObject *self, PyObject *args) {
    debug_enter();
    int device;
    uintptr_t stream_id;
    size_t size;
    int alignment;
    void *device_ptr = NULL;

 	if (! PyArg_ParseTuple(args, "ikki", &device, &stream_id, &size, &alignment))
		return NULL;

    if (libxstream_mem_allocate(device, &device_ptr, size, alignment) != LIBXSTREAM_ERROR_NONE) {
        printf("WHOOOOP at %s:%d\n", __FUNCTION__, __LINE__);
        throw internal_exception("XSTREAM FAILED", __FILE__, __LINE__);
    }
    
    debug(10, "allocated %ld bytes on device %d with pointer %p",
          size, device, device_ptr);
    debug_leave();
	return Py_BuildValue("k", reinterpret_cast<uintptr_t>(device_ptr));
}


PYMIC_INTERFACE
PyObject *pymic_impl_stream_deallocate(PyObject *self, PyObject *args) {
    debug_enter();
    int device;
    uintptr_t stream_id;
    unsigned char * device_ptr;

 	if (! PyArg_ParseTuple(args, "ikk", &device, &stream_id, &device_ptr))
		return NULL;

    if (libxstream_mem_deallocate(device, device_ptr) != LIBXSTREAM_ERROR_NONE) {
        printf("WHOOOOP at %s:%d\n", __FUNCTION__, __LINE__);
        throw internal_exception("XSTREAM FAILED", __FILE__, __LINE__);
    }
    
    debug(10, "deallocated pointer %p on device %d",
          device_ptr, device);
    debug_leave();
	Py_RETURN_NONE;
}


PYMIC_INTERFACE
PyObject *pymic_impl_stream_memcpy_h2d(PyObject *self, PyObject *args) {
    debug_enter();
    uintptr_t host_ptr;
    uintptr_t device_ptr;
    size_t size;
    size_t offset_host;
    size_t offset_device;
    int device;
    uintptr_t stream_id;
    libxstream_stream *stream = NULL;
    unsigned char *host_mem;
    unsigned char *dev_mem;
    
 	if (! PyArg_ParseTuple(args, "ikkkkkk", &device, &stream_id, &host_ptr, &device_ptr, 
                           &size, &offset_host, &offset_device))
		return NULL;
    host_mem = reinterpret_cast<unsigned char *>(host_ptr);
    dev_mem = reinterpret_cast<unsigned char *>(device_ptr);
    stream = reinterpret_cast<libxstream_stream *>(stream_id);

    unsigned char * src_offs = host_mem + offset_host;
    unsigned char * dst_offs = dev_mem + offset_device;
    debug(100, "%s: transferring %ld bytes from host pointer 0x%lp into device pointer 0x%lx",
          __FUNCTION__, size, hptr, dptr);
    if (libxstream_memcpy_h2d(src_offs, dst_offs, size, stream) != LIBXSTREAM_ERROR_NONE) {
        printf("WHOOOOP at %s:%d\n", __FUNCTION__, __LINE__);
        throw internal_exception("XSTREAM FAILED", __FILE__, __LINE__);
    }
                          
    debug_leave();
    Py_RETURN_NONE;
}


PYMIC_INTERFACE
PyObject *pymic_impl_stream_memcpy_d2h(PyObject *self, PyObject *args) {
    debug_enter();
    uintptr_t host_ptr;
    uintptr_t device_ptr;
    size_t size;
    size_t offset_host;
    size_t offset_device;
    int device;
    uintptr_t stream_id;
    libxstream_stream *stream = NULL;
    unsigned char *host_mem;
    unsigned char *dev_mem;
    
 	if (! PyArg_ParseTuple(args, "ikkkkkk", &device, &stream_id, &device_ptr, &host_ptr,  
                           &size, &offset_device, &offset_host))
		return NULL;
    dev_mem = reinterpret_cast<unsigned char *>(device_ptr);
    host_mem = reinterpret_cast<unsigned char *>(host_ptr);
    stream = reinterpret_cast<libxstream_stream *>(stream_id);
    
    unsigned char * src_offs = dev_mem + offset_device;
    unsigned char * dst_offs = host_mem + offset_host;
    debug(100, "%s: transferring %ld bytes from device pointer 0x%lp into host pointer 0x%lx",
          __FUNCTION__, size, dptr, hptr);
    if (libxstream_memcpy_d2h(src_offs, dst_offs, size, stream) != LIBXSTREAM_ERROR_NONE) {
        printf("WHOOOOP at %s:%d\n", __FUNCTION__, __LINE__);
        throw internal_exception("XSTREAM FAILED", __FILE__, __LINE__);
    }

    debug_leave();
    Py_RETURN_NONE;
}


PYMIC_INTERFACE
PyObject *pymic_impl_stream_memcpy_d2d(PyObject *self, PyObject *args) {
    debug_enter();
    uintptr_t src;
    uintptr_t dst;
    size_t size;
    size_t offset_device_src;
    size_t offset_device_dst;
    int device;
    uintptr_t stream_id;
    libxstream_stream *stream = NULL;
    unsigned char *src_mem;
    unsigned char *dst_mem;
    
 	if (! PyArg_ParseTuple(args, "ikkkkkk", &device, &stream_id, &src, &dst,  
                           &size, &offset_device_src, &offset_device_dst))
		return NULL;
    src_mem = reinterpret_cast<unsigned char *>(src);
    dst_mem = reinterpret_cast<unsigned char *>(dst);
    stream = reinterpret_cast<libxstream_stream *>(stream_id);
    
    // buffer_copy_on_device(device, stream, src_mem, dst_mem, 
                          // size, offset_device_src, offset_device_dst);
    unsigned char * src_ptr = src_mem + offset_device_src;
    unsigned char * dst_ptr = dst_mem + offset_device_dst;
    if (libxstream_memcpy_d2d(src_ptr, dst_ptr, size, stream) != LIBXSTREAM_ERROR_NONE) {
        printf("WHOOOOP at %s:%d\n", __FUNCTION__, __LINE__);
        throw internal_exception("XSTREAM FAILED", __FILE__, __LINE__);
    }
                          
    debug_leave();
    Py_RETURN_NONE;
}


PYMIC_INTERFACE
PyObject *pymic_impl_find_kernel(PyObject *self, PyObject *args) {
    int device;
    uintptr_t funcptr = 0;
    uintptr_t handle = 0;
    const char* func_cstr = NULL;
    std::string func;
    
    debug_enter();
    
    if (! PyArg_ParseTuple(args, "iks", &device, &handle, &func_cstr))
        return NULL;
    func = func_cstr;
    
    funcptr = find_kernel(device, handle, func);
    if (! funcptr) {
        return throw_exception("Could not locate kernel '%s' in library %p.",
                               func_cstr, handle);;
    }
    
    return Py_BuildValue("k", funcptr);
}


PyObject *pymic_impl_invoke_kernel(PyObject *self, PyObject *args) {
    int device;
    uintptr_t stream_id;
    libxstream_stream * stream;
    uintptr_t funcptr = 0;
    PyObject *argd = NULL;    // dimensionality of the arguments
    PyObject *argt = NULL;    // types of the arguments (int, double, complex)
    PyObject *argv = NULL;    // values of the arguments (host/device pointers)
    PyObject *argsz = NULL;   // number of bytes for the argument
    int argc = 0;
    debug_enter();

    if (! PyArg_ParseTuple(args, "ikkOOOOi", &device, &stream_id, &funcptr,
                           &argd, &argt, &argv, &argsz, &argc))
        return NULL;

#if PYMIC_USE_XSTREAM
    stream = reinterpret_cast<libxstream_stream *>(stream_id);
    if (!PyArray_Check(argd))
        return NULL;
    if (!PyArray_Check(argv))
        return NULL;
    if (!PyArray_Check(argsz))
        return NULL;

    PyArrayObject * array_argd = reinterpret_cast<PyArrayObject *>(argd);
    PyArrayObject * array_argt = reinterpret_cast<PyArrayObject *>(argt);
    PyArrayObject * array_argv = reinterpret_cast<PyArrayObject *>(argv);
    PyArrayObject * array_argsz = reinterpret_cast<PyArrayObject *>(argsz);
    uintptr_t * dims = reinterpret_cast<uintptr_t *>(PyArray_BYTES(array_argd));
    int64_t * types = reinterpret_cast<int64_t *>(PyArray_BYTES(array_argt));
    uintptr_t * ptrs = reinterpret_cast<uintptr_t *>(PyArray_BYTES(array_argv));
    int64_t * sizes = reinterpret_cast<int64_t *>(PyArray_BYTES(array_argsz));

    // generate the proper function signature for this kernel
    libxstream_argument * signature = NULL;
    libxstream_fn_signature(&signature);
    for (int i = 0; i < argc; ++i) {
        uintptr_t dim = dims[i];
        int64_t type = types[i];
        uintptr_t ptr = ptrs[i];
        size_t size = static_cast<size_t>(sizes[i]);
        libxstream_type scalar_type;
        size_t scalar_size = 1;
        switch (dim) {
        case 0: // argument is a scalar value
            // in case of a scalar value, the ptr is a host pointer
            switch(type) {
            case dtype_int:
                scalar_type = LIBXSTREAM_TYPE_I64;
                break;
            case dtype_float:
                scalar_type = LIBXSTREAM_TYPE_F64;
                break;
            case dtype_complex:
                scalar_type = LIBXSTREAM_TYPE_C64;
                break;
            case dtype_uint64:
                scalar_type = LIBXSTREAM_TYPE_U64;
                break;
            default:
                printf("WHOOOOP at %s:%d\n", __FUNCTION__, __LINE__);
                throw internal_exception("XSTREAM FAILED", __FILE__, __LINE__);
                break;
            }
            if (libxstream_fn_input(signature, i, reinterpret_cast<void *>(ptr), scalar_type, dim, NULL) != LIBXSTREAM_ERROR_NONE) {
                printf("WHOOOOP at %s:%d\n", __FUNCTION__, __LINE__);
                throw internal_exception("XSTREAM FAILED", __FILE__, __LINE__);
            };
            break;
        case 1: // argument is an array
            // in case of an array, the ptr is a fake pointer
            if (libxstream_fn_inout(signature, i, reinterpret_cast<void *>(ptr), LIBXSTREAM_TYPE_BYTE, dim, &size) != LIBXSTREAM_ERROR_NONE) {
                printf("WHOOOOP at %s:%d\n", __FUNCTION__, __LINE__);
                throw internal_exception("XSTREAM FAILED", __FILE__, __LINE__);
            }
            break;
        default:
            // TODO: throw Python exception
            throw internal_exception("XSTREAM FAILED", __FILE__, __LINE__);
            return NULL;
        }
    }
    
    // invoke function and clear function signature
    if (libxstream_fn_call(reinterpret_cast<libxstream_function>(funcptr), signature, 
                           stream, LIBXSTREAM_CALL_DEFAULT | LIBXSTREAM_CALL_NATIVE) != LIBXSTREAM_ERROR_NONE) {
        printf("WHOOOOP at %s:%d\n", __FUNCTION__, __LINE__);
        throw internal_exception("XSTREAM FAILED", __FILE__, __LINE__);
    }
#else
    // retrieve device pointers from the argument list
    std::vector< std::pair<uintptr_t, size_t> > arguments;
    if (!PyArray_Check(argv))
        return NULL;
    if (!PyArray_Check(argsz))
        return NULL;
    PyArrayObject * array_argv = reinterpret_cast<PyArrayObject *>(argv);
    PyArrayObject * array_argsz = reinterpret_cast<PyArrayObject *>(argsz);
    uintptr_t * ptrs = reinterpret_cast<uintptr_t *>(PyArray_BYTES(array_argv));
    uintptr_t * sizes = reinterpret_cast<uintptr_t *>(PyArray_BYTES(array_argsz));
    for (int i = 0; i < argc; ++i) {
        uintptr_t dev_ptr = ptrs[i];
        size_t size = sizes[i];
        arguments.push_back(std::pair<uintptr_t, size_t>(dev_ptr, size));
    }

    // invoke the kernel on the remote side
    target_invoke_kernel(device, funcptr, arguments);
#endif

    debug_leave();
    Py_RETURN_NONE;
}


// these are the mappings from the Python interface to the C entry points
static PyMethodDef methods[] = {
    // generic support functions
    PYMIC_ENTRYPOINT(pymic_impl_offload_number_of_devices, "Determine the number of offload devices available."),
    PYMIC_ENTRYPOINT(pymic_impl_load_library, "Load a library with kernels to the target."),
    PYMIC_ENTRYPOINT(pymic_impl_unload_library, "Unload a loaded library with kernels on the target."),

    // stream functions
    PYMIC_ENTRYPOINT(pymic_impl_stream_create, "Create a stream and return its ID."),
    PYMIC_ENTRYPOINT(pymic_impl_stream_destroy, "Destroy a stream with ID on a device."),
    PYMIC_ENTRYPOINT(pymic_impl_stream_sync, "Wait for all outstanding requests in the queue."),

    // new data management functions
    PYMIC_ENTRYPOINT(pymic_impl_stream_allocate, "Allocate device memory"),
    PYMIC_ENTRYPOINT(pymic_impl_stream_deallocate, "Deallocate device memory"),
    PYMIC_ENTRYPOINT(pymic_impl_stream_memcpy_h2d, "Copy from device to host"),
    PYMIC_ENTRYPOINT(pymic_impl_stream_memcpy_d2h, "Copy from device to host"),
    PYMIC_ENTRYPOINT(pymic_impl_stream_memcpy_d2d, "Copy from device to device"),

    // kernel invocation
    PYMIC_ENTRYPOINT(pymic_impl_find_kernel, "Find a kernel by its name in a loaded library."),
    PYMIC_ENTRYPOINT(pymic_impl_invoke_kernel, "Invoke a kernel and pass additional data to target."),

    // these are the entries points that need to be refactored
    PYMIC_ENTRYPOINT_TERMINATOR
};



extern "C" 
void init_pymicimpl() {
	debug_enter();
	(void) Py_InitModule("_pymicimpl", methods);
	import_array();
	debug_leave();
}
