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

#include "pymic_internal.h"
#include "pymicimpl_misc.h"

#include "debug.h"

#include <stddef.h>
#include <stdio.h> 
#include <stdint.h>
#include <string.h>

#include <string>

extern "C"
int pymic_internal_invoke_kernel(int device, libxstream_stream *stream, 
                                 void *funcptr, size_t argc, 
                                 const int64_t *dims, const int64_t *types, 
                                 void **ptrs, const size_t *sizes) {
    debug_enter();
    // generate the proper function signature for this kernel
    libxstream_argument * signature = NULL;
    libxstream_fn_signature(&signature);
    for (int i = 0; i < argc; ++i) {
        int64_t dim = dims[i];
        int64_t type = types[i];
        void *ptr = ptrs[i];
        size_t size = static_cast<size_t>(sizes[i]);
        libxstream_type scalar_type;
        size_t scalar_size = 1;
        switch (dim) {
        case 0: // argument is a scalar value
            // in case of a scalar value, the ptr is a host pointer
            switch(type) {
            case pymic::dtype_int:
                scalar_type = LIBXSTREAM_TYPE_I64;
                break;
            case pymic::dtype_float:
                scalar_type = LIBXSTREAM_TYPE_F64;
                break;
            case pymic::dtype_complex:
                scalar_type = LIBXSTREAM_TYPE_C64;
                break;
            case pymic::dtype_uint64:
                scalar_type = LIBXSTREAM_TYPE_U64;
                break;
            default:
                printf("WHOOOOP at %s:%d\n", __FUNCTION__, __LINE__);
                debug_leave();
                return 1;
            }
            if (libxstream_fn_input(signature, i, ptr, scalar_type, dim, NULL) != LIBXSTREAM_ERROR_NONE) {
                printf("WHOOOOP at %s:%d\n", __FUNCTION__, __LINE__);
                debug_leave();
                return 1;
            };
            break;
        case 1: // argument is an array
            // in case of an array, the ptr is a fake pointer
            if (libxstream_fn_inout(signature, i, ptr, LIBXSTREAM_TYPE_BYTE, dim, &size) != LIBXSTREAM_ERROR_NONE) {
                printf("WHOOOOP at %s:%d\n", __FUNCTION__, __LINE__);
                debug_leave();
                return 1;
            }
            break;
        default:
            printf("WHOOOOP at %s:%d\n", __FUNCTION__, __LINE__);
            debug_leave();
            return 1;
        }
    }
    
    // invoke function and clear function signature
    if (libxstream_fn_call(reinterpret_cast<libxstream_function>(funcptr), signature, 
                           stream, LIBXSTREAM_CALL_DEFAULT | LIBXSTREAM_CALL_NATIVE) != LIBXSTREAM_ERROR_NONE) {
        printf("WHOOOOP at %s:%d\n", __FUNCTION__, __LINE__);
        debug_leave();
        return 1;
    }

    if (libxstream_fn_clear_signature(signature) != LIBXSTREAM_ERROR_NONE) {
        printf("WHOOOOP at %s:%d\n", __FUNCTION__, __LINE__);
        debug_leave();
        return 1;
    }
       
    debug_leave();
    return 0;
}                                     


extern "C"
int pymic_internal_load_library(int device, const char *filename, int64_t *handle, char **tempfile) {
    debug_enter();
    std::string tempname;
    uintptr_t library_handle;
    try {
        pymic::target_load_library(device, filename, tempname, library_handle);
        memcpy(handle, &library_handle, sizeof(*handle));
    }
    catch (pymic::internal_exception *exc) {
        // catch and convert exception to return code
        debug(100, "internal exception raised at %s:%d, raising Python exception", 
                   exc->file(), exc->line());
        std::string reason = exc->reason();
        delete exc;
        debug_leave();
        return 1;
    }
    *tempfile = new char[tempname.length() + 1];
    memcpy(*tempfile, tempname.c_str(), tempname.length() + 1);
    debug_leave();
    return 0;
}


extern "C"
int pymic_internal_unload_library(int device, int64_t handle, const char *tempfile) {
    debug_enter();
    std::string tempname(tempfile);
    try {
        pymic::target_unload_library(device, tempname, handle);
	}
    catch (pymic::internal_exception *exc) {
        // catch and convert exception to return code
        debug(100, "internal exception raised at %s:%d, raising Python exception", 
                   exc->file(), exc->line());
        std::string reason = exc->reason();
        delete exc;
        debug_leave();
        return 1;
    }
    debug_leave();
    return 0;
}


extern "C"
int pymic_internal_find_kernel(int device, int64_t handle, const char *kernel_name, int64_t *kernel_ptr) {
    std::string func(kernel_name);
    uintptr_t funcptr = 0;
    funcptr = pymic::find_kernel(device, handle, func);
    if (! funcptr) {
        return 1;
    }
    memcpy(kernel_ptr, &funcptr, sizeof(*kernel_ptr));
    return 0;
}

// this is a workaround for a problem with the Intel LEO compiler
const libxstream_function translate_pointer_func_workaround = reinterpret_cast<libxstream_function>(pymic::pymic_translate_pointer);

extern "C"
int pymic_internal_translate_pointer(int device, libxstream_stream *stream,
                                         uintptr_t pointer, int64_t *translated) {
    debug_enter();
    char *dev_ptr = reinterpret_cast<char*>(pointer);
    int64_t ret = 0;
    libxstream_argument *signature = NULL;
    if (libxstream_fn_signature(&signature) != LIBXSTREAM_ERROR_NONE) {
        printf("WHOOOOP at %s:%d\n", __FUNCTION__, __LINE__);
        debug_leave();
        return 1;
    }
    if (libxstream_fn_input(signature, 0, dev_ptr, LIBXSTREAM_TYPE_VOID, 1, 0) != LIBXSTREAM_ERROR_NONE) {
        printf("WHOOOOP at %s:%d\n", __FUNCTION__, __LINE__);
        debug_leave();
        return 1;
    }
    if (libxstream_fn_output(signature, 1, &ret, libxstream_map_to<int64_t>::type(), 0, 0) != LIBXSTREAM_ERROR_NONE) {
        printf("WHOOOOP at %s:%d\n", __FUNCTION__, __LINE__);
        debug_leave();
        return 1;
    }
    if (libxstream_fn_call(translate_pointer_func_workaround, signature, stream, LIBXSTREAM_CALL_WAIT) != LIBXSTREAM_ERROR_NONE) {
        printf("WHOOOOP at %s:%d\n", __FUNCTION__, __LINE__);
        debug_leave();
        return 1;
    }
    if (libxstream_fn_clear_signature(signature) != LIBXSTREAM_ERROR_NONE) {
        printf("WHOOOOP at %s:%d\n", __FUNCTION__, __LINE__);
        debug_leave();
        return 1;
    }
    // memcpy(translated, &ret, sizeof(int64_t));
    *translated = ret;
    debug_leave();
    return 0;
}
