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
	int no_of_devs = get_number_of_devices();
	debug(10, "detected %d devices", no_of_devs);
	debug_leave();
	return Py_BuildValue("I", no_of_devs);
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

PYMIC_INTERFACE
PyObject* pymic_impl_buffer_allocate(PyObject* self, PyObject* args) {
	int device;
	char *payload = NULL;
	size_t size = 0;
	PyArrayObject *array = NULL;
	debug_enter();
	if (!PyArg_ParseTuple(args, "iOI", &device, &array, &size)) 
		return NULL;
	if (!PyArray_Check(array)) 
		return NULL;
	payload = PyArray_BYTES(array);
	debug(10, "(CPU->MIC %d) allocating buffer with %d bytes with array %p, backing store at %p", 
	      device, size, (void*) array, (void*) payload);
	buffer_allocate(device, payload, size);
	Py_INCREF(array);
	debug_leave();
	Py_RETURN_NONE;
}

PYMIC_INTERFACE
PyObject* pymic_impl_buffer_release(PyObject* self, PyObject* args) {
	int device;
	char *payload = NULL;
	size_t size = 0;
	PyArrayObject *array = NULL;
	debug_enter();
	if (!PyArg_ParseTuple(args, "iOI", &device, &array, &size)) 
		return NULL;
	if (!PyArray_Check(array)) 
		return NULL;
	payload = PyArray_BYTES(array);
	debug(10, "(CPU->MIC %d) releasing buffer with %d bytes with array %p, backing store at %p", 
	      device, size, (void*) array, (void*) payload);
	buffer_release(device, payload, size);
	Py_DECREF(array);
	debug_leave();
	Py_RETURN_NONE;
}

PYMIC_INTERFACE
PyObject* pymic_impl_buffer_update_on_target(PyObject* self, PyObject* args) {
	int device;
	char *payload = NULL;
	size_t size = 0;
	PyArrayObject *array = NULL;
	debug_enter();
	if (!PyArg_ParseTuple(args, "iOI", &device, &array, &size)) 
		return NULL;
	if (!PyArray_Check(array)) 
		return NULL;
	payload = PyArray_BYTES(array);
	debug(10, "(CPU->MIC %d) updating buffer with %d bytes with array %p, backing store at %p", 
	      device, size, (void*) array, (void*) payload);
	buffer_update_on_target(device, payload, size);
	debug_leave();
	Py_RETURN_NONE;
}

PYMIC_INTERFACE
PyObject* pymic_impl_buffer_update_on_host(PyObject* self, PyObject* args) {
	int device;
	char *payload = NULL;
	size_t size = 0;
	PyArrayObject *array = NULL;
	debug_enter();
	if (!PyArg_ParseTuple(args, "iOI", &device, &array, &size)) 
		return NULL;
	if (!PyArray_Check(array)) 
		return NULL;
	payload = PyArray_BYTES(array);
	debug(10, "(MIC %d->CPU) updating buffer with %d bytes with array %p, backing store at %p", 
	      device, size, (void*) array, (void*) payload);
	buffer_update_on_host(device, payload, size);
	debug_leave();
	Py_RETURN_NONE;
}

PYMIC_INTERFACE
PyObject* pymic_impl_buffer_alloc_and_copy_to_target(PyObject* self, PyObject* args) {
	int device;
	char *payload = NULL;
	size_t size = 0;
	PyArrayObject *array = NULL;
	debug_enter();
	if (!PyArg_ParseTuple(args, "iOI", &device, &array, &size)) 
		return NULL;
	if (!PyArray_Check(array)) 
		return NULL;
	payload = PyArray_BYTES(array);
	debug(10, "(CPU->MIC %d) copying buffer with %d bytes with array %p, backing store at %p", 
	      device, size, (void*) array, (void*) payload);
	buffer_alloc_and_copy_to_target(device, payload, size);
	Py_INCREF(array);
	debug_leave();
	Py_RETURN_NONE;
}

PYMIC_INTERFACE
PyObject* pymic_impl_buffer_copy_from_target_and_release(PyObject* self, PyObject* args) {
	int device;
	char *payload = NULL;
	size_t size = 0;
	PyArrayObject *array = NULL;
	debug_enter();
	if (!PyArg_ParseTuple(args, "iOI", &device, &array, &size)) 
		return NULL;
	if (!PyArray_Check(array)) 
		return NULL;
	payload = PyArray_BYTES(array);
	debug(10, "(MIC %d->CPU) copying buffer with %d bytes with array %p, backing store at %p", 
	      device, size, (void*) array, (void*) payload);
	buffer_copy_from_target_and_release(device, payload, size);
	Py_DECREF(array);
	debug_leave();
	Py_RETURN_NONE;
}

PYMIC_INTERFACE
PyObject* pymic_impl_invoke_kernel(PyObject* self, PyObject* args) {
	int device;
    uintptr_t funcptr = 0;
	PyObject* varargs = NULL;
	int varargc = 0;
	debug_enter();
	
	if (! PyArg_ParseTuple(args, "ikO", &device, &funcptr, &varargs))
		return NULL;
	
	if (!PyTuple_Check(varargs)) 
		return NULL;
	varargc = PyTuple_Size(varargs);
	debug(100, "kernel %x: %d extra argument%s", funcptr, varargc, ((varargc != 1) ? "s" : ""));
	
	// retrieve payload and size information from the arguments
	std::vector<std::pair<char *,size_t> > arguments;
	for (int i = 0; i < varargc; ++i) {
        // we expect different kinds of array: numpy.ndarray or OffloadArray
        PyObject* array_obj = PyTuple_GetItem(varargs, i);
		debug(100, "retrieving extra argument %d gave %p", i, array_obj);

		if (PyArray_Check(array_obj)) {
            debug(100, "argument %d is numpy.ndarray", i);
            // if this a numpy.ndarray, we can directly use it
			PyArrayObject* array = reinterpret_cast<PyArrayObject*>(array_obj);
			char *payload = PyArray_BYTES(array);
			size_t size = PyArray_NBYTES(array);
			arguments.push_back(std::pair<char *, int>(payload, size));
		}
        else {
            // this is an OffloadArray, we have to get the ndarray first
            // TODO: we might want to do some safety checks here
            PyObject* array = PyObject_GetAttrString(array_obj, "array");
            if (!array)
                return NULL;
            if(PyArray_Check(array)) {
                debug(100, "argument %d is OffloadArray w/ encapsulated numpy.ndarray", i);
                PyArrayObject* nparray = reinterpret_cast<PyArrayObject*>(array);
                char* payload = PyArray_BYTES(nparray);
                size_t size = PyArray_NBYTES(nparray);
                arguments.push_back(std::pair<char *, int>(payload, size));
            }
            else {
                debug(100, "unrecognized argument object for argument %d, bailing out", i);
                return NULL;
            }
        }
	}
	
	// invoke the kernel on the remote side
    target_invoke_kernel(device, funcptr, arguments);
	
	debug_leave();
	Py_RETURN_NONE;
}


PYMIC_INTERFACE
PyObject* pymic_impl_test(PyObject* self, PyObject* args) {
	Py_RETURN_NONE;
}

// these are the mappings from the Python interface to the C entry points
static PyMethodDef methods[] = {
	// generic support functions
	PYMIC_ENTRYPOINT(pymic_impl_offload_number_of_devices, "Determine the number of offload devices available."),
	PYMIC_ENTRYPOINT(pymic_impl_load_library, "Load a library with kernels to the target."),
	PYMIC_ENTRYPOINT(pymic_impl_unload_library, "Unload a loaded library with kernels on the target."),
	PYMIC_ENTRYPOINT(pymic_impl_find_kernel, "Find a kernel by its name in a loaded library."),
	
	// data management functions
	PYMIC_ENTRYPOINT(pymic_impl_buffer_allocate, "Allocate a buffer on the target device."),
	PYMIC_ENTRYPOINT(pymic_impl_buffer_release, "Release (deallocate) a buffer on the target device."),
	PYMIC_ENTRYPOINT(pymic_impl_buffer_update_on_target, "Update an existing buffer with data from the host."),
	PYMIC_ENTRYPOINT(pymic_impl_buffer_update_on_host, "Update an existing buffer with data from the target."),
	PYMIC_ENTRYPOINT(pymic_impl_buffer_alloc_and_copy_to_target, "Create a new buffer on the target and copy data to it."),
	PYMIC_ENTRYPOINT(pymic_impl_buffer_copy_from_target_and_release, "Copy data from target to host and release buffer."),
	
	// kernel invocation
	PYMIC_ENTRYPOINT(pymic_impl_invoke_kernel, "Invoke a kernel and pass additional data to target."),
	
	PYMIC_ENTRYPOINT(pymic_impl_test, "Test function for some testing"),
	
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
