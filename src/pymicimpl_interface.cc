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
#include "pymicimpl_buffers.h"
#include "pymicimpl_invoke.h"

#include "debug.h"

using namespace pymic;

PyObject* throw_exception(const char* format, ...) {
    va_list vargs;

    const size_t bufsz = 256;
    char buffer[bufsz];

    PyObject* pymic = NULL;
    PyObject* modules_dict = NULL;
    PyObject* exception_type = NULL;
    
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
    PyObject* exception = PyErr_Format(exception_type, buffer);
    return exception;
}


PYMIC_INTERFACE 
PyObject* pymic_impl_offload_number_of_devices(PyObject* self, PyObject* args) {
	debug_enter();
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
    size_t no_of_devs = 0;
	no_of_devs = get_number_of_devices();
	debug(10, "detected %d devices", no_of_devs);
	debug_leave();
	return Py_BuildValue("I", static_cast<int>(no_of_devs));
}

PYMIC_INTERFACE
PyObject* pymic_impl_load_library(PyObject* self, PyObject* args) {
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
PyObject* pymic_impl_unload_library(PyObject* self, PyObject* args) {
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
PyObject * pymic_impl_stream_create(PyObject * self, PyObject * args) {
    debug_enter();
    int lowest, greatest;
    int device;
    char * name;
    
	if (! PyArg_ParseTuple(args, "is", &device, &name))
		return NULL;

    // in the native implementation, we make everything synchronous and
    // we use the same stream (= device id)
    debug(10, "using default stream for device %d", device);
        
    debug_leave();
    return Py_BuildValue("k", static_cast<uintptr_t>(device));
}


PYMIC_INTERFACE
PyObject * pymic_impl_stream_destroy(PyObject * self, PyObject * args) {
    debug_enter();
    int device;
    uintptr_t stream_id;

	if (! PyArg_ParseTuple(args, "ik", &device, &stream_id))
		return NULL;
    
    debug_leave();
    Py_RETURN_NONE;
}


PYMIC_INTERFACE
PyObject * pymic_impl_stream_allocate(PyObject * self, PyObject * args) {
    debug_enter();
    int device;
    uintptr_t stream_id;
    size_t size;
    int alignment;
    unsigned char * device_ptr;

 	if (! PyArg_ParseTuple(args, "ikki", &device, &stream_id, &size, &alignment))
		return NULL;
    device_ptr = buffer_allocate(device, size, alignment);
    
    debug(10, "allocated %ld bytes on device %d with pointer %p",
          size, device, device_ptr);
    debug_leave();
	return Py_BuildValue("k", reinterpret_cast<uintptr_t>(device_ptr));
}


PYMIC_INTERFACE
PyObject * pymic_impl_stream_deallocate(PyObject * self, PyObject * args) {
    debug_enter();
    int device;
    uintptr_t stream_id;
    unsigned char * device_ptr;

 	if (! PyArg_ParseTuple(args, "ikk", &device, &stream_id, &device_ptr))
		return NULL;
    buffer_release(device, device_ptr);

    debug(10, "deallocated pointer %p on device %d",
          device_ptr, device);
    debug_leave();
	Py_RETURN_NONE;
}


PYMIC_INTERFACE
PyObject * pymic_impl_stream_memcpy_h2d(PyObject * self, PyObject * args) {
    debug_enter();
    uintptr_t host_ptr;
    uintptr_t device_ptr;
    size_t size;
    size_t offset_host;
    size_t offset_device;
    uintptr_t stream_id;
    unsigned char * host_mem;
    unsigned char * dev_mem;
    
 	if (! PyArg_ParseTuple(args, "kkkkkk", &stream_id, &host_ptr, &device_ptr, 
                           &size, &offset_host, &offset_device))
		return NULL;
    host_mem = reinterpret_cast<unsigned char *>(host_ptr);
    dev_mem = reinterpret_cast<unsigned char *>(device_ptr);

    int device = static_cast<int>(stream_id);
    buffer_copy_to_target(device, host_mem, dev_mem, 
                          size, offset_host, offset_device);

    debug_leave();
    Py_RETURN_NONE;
}


PYMIC_INTERFACE
PyObject * pymic_impl_stream_memcpy_d2h(PyObject * self, PyObject * args) {
    debug_enter();
    uintptr_t host_ptr;
    uintptr_t device_ptr;
    size_t size;
    size_t offset_host;
    size_t offset_device;
    uintptr_t stream_id;
    unsigned char * host_mem;
    unsigned char * dev_mem;
    
 	if (! PyArg_ParseTuple(args, "kkkkkk", &stream_id, &device_ptr, &host_ptr,  
                           &size, &offset_device, &offset_host))
		return NULL;
    dev_mem = reinterpret_cast<unsigned char *>(device_ptr);
    host_mem = reinterpret_cast<unsigned char *>(host_ptr);
    
    int device = static_cast<int>(stream_id);
    buffer_copy_to_host(device, dev_mem, host_mem, 
                        size, offset_device, offset_host);

    debug_leave();
    Py_RETURN_NONE;
}


PYMIC_INTERFACE
PyObject * pymic_impl_stream_memcpy_d2d(PyObject * self, PyObject * args) {
    debug_enter();
    uintptr_t src;
    uintptr_t dst;
    size_t size;
    size_t offset_device_src;
    size_t offset_device_dst;
    uintptr_t stream_id;
    unsigned char * src_mem;
    unsigned char * dst_mem;
    
 	if (! PyArg_ParseTuple(args, "kkkkkk", &stream_id, &src, &dst,  
                           &size, &offset_device_src, &offset_device_dst))
		return NULL;
    src_mem = reinterpret_cast<unsigned char *>(src);
    dst_mem = reinterpret_cast<unsigned char *>(dst);
    
    int device = static_cast<int>(stream_id);
    buffer_copy_on_device(device, src_mem, dst_mem, 
                          size, offset_device_src, offset_device_dst);

    debug_leave();
    Py_RETURN_NONE;
}


PYMIC_INTERFACE
PyObject * pymic_impl_stream_ptr_translate(PyObject * self, PyObject * args) {
    debug_enter();
    uintptr_t device_ptr;
    uintptr_t translated;
    
 	if (! PyArg_ParseTuple(args, "k", &device_ptr))
		return NULL;

    translated = buffer_translate_pointer(reinterpret_cast<unsigned char *>(device_ptr));

    debug_leave();
    return Py_BuildValue("k", translated);
}


PYMIC_INTERFACE
PyObject* pymic_impl_find_kernel(PyObject* self, PyObject* args) {
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


PyObject * pymic_impl_invoke_kernel(PyObject * self, PyObject * args) {
    int device;
    uintptr_t funcptr = 0;
    PyObject * argv = NULL;
    PyObject * argsz = NULL;
    int argc = 0;
    debug_enter();

    if (! PyArg_ParseTuple(args, "ikOOi", &device, &funcptr,
                           &argv, &argsz, &argc))
    return NULL;

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

    // new data management functions
    PYMIC_ENTRYPOINT(pymic_impl_stream_allocate, "Allocate device memory"),
    PYMIC_ENTRYPOINT(pymic_impl_stream_deallocate, "Deallocate device memory"),
    PYMIC_ENTRYPOINT(pymic_impl_stream_memcpy_h2d, "Copy from device to host"),
    PYMIC_ENTRYPOINT(pymic_impl_stream_memcpy_d2h, "Copy from device to host"),
    PYMIC_ENTRYPOINT(pymic_impl_stream_memcpy_d2d, "Copy from device to device"),
    PYMIC_ENTRYPOINT(pymic_impl_stream_ptr_translate, "Pointer translation"),

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
