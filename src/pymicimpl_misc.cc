/* Copyright (c) 2014-2016, Intel Corporation All rights reserved. 
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

#pragma offload_attribute(push, target(mic))
#include <string>
#pragma offload_attribute(pop)
#include <algorithm>

#include <stdint.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#if defined(_WIN32)
#else
#include <unistd.h>
#include <dlfcn.h>
#endif
 
#include "pymicimpl_misc.h"

#include <pymic_kernel.h>

#include "debug.h"

#if defined(_WIN32)
// define ssize_t on Windows
typedef int ssize_t;
#endif

// VS 2013 does not know snprintf yet
#if defined(_MSC_VER) && _MSC_VER < 1900
#define snprintf _snprintf
#endif

namespace pymic {

#if ! PYMIC_USE_XSTREAM
int get_number_of_devices() {
	return _Offload_number_of_devices();
}
#endif

#if defined(_WIN32)
__declspec(target(mic))
#else
__attribute__((target(mic)))
#endif
uintptr_t target_load_library_device(char *buffer, size_t bufsz, char *data, 
                                     ssize_t size, char *tempname_cstr, 
                                     size_t tempname_cstr_sz) {
#if defined(__MIC__)
    uintptr_t handle_device_ptr = 0;
    int fd;
    int ret;
    const char *tmplt = "/tmp/pymic-lib-XXXXXX";
    void *handle = NULL;

    // write the library's binary image to a temporary file
    memset(tempname_cstr, 0, tempname_cstr_sz);
    strncpy(tempname_cstr, tmplt, tempname_cstr_sz);
    fd = mkstemp(tempname_cstr);
    if (fd == -1) {
        snprintf(buffer, bufsz, "mkstemp failed ('%s')", dlerror());
        goto error_label_target_load_library;
    }
    ret = write(fd, data, size);
    if (ret != size) {
        snprintf(buffer, bufsz, "write failed ('%s')", dlerror());
        close(fd);
        goto error_label_target_load_library;
    }
    ret = close(fd);
    if (ret) {
        snprintf(buffer, bufsz, "close failed ('%s')", dlerror());
        goto error_label_target_load_library;
    }

    // now load the library
    dlerror();
    handle = dlopen(tempname_cstr, RTLD_NOW | RTLD_GLOBAL);
    if (!handle) {
        // prepare the error message for an exception to be thrown on host
        snprintf(buffer, bufsz, "dlopen failed ('%s')", dlerror());
    }
    handle_device_ptr = reinterpret_cast<uintptr_t>(handle);
error_label_target_load_library:
    ;
    return handle_device_ptr;
#else
    // this function does not make sense on the host
    fprintf(stderr, "THIS SHOULD NOT HAPPEN!\n");
    exit(1);
    return NULL;
#endif
}

void target_load_library(int device, const std::string &filename, 
                             std::string &tempname, uintptr_t &handle) {
    debug_enter();
    
    int target = device;
	uintptr_t handle_device_ptr = 0;
    ssize_t ret;
    FILE *file = NULL;
    struct stat st;
    ssize_t size;
    char *data;
	const size_t bufsz = 256;
	char buffer[bufsz];
    char tempname_cstr[64];

    // get the file size
    errno = 0;
    ret = stat(filename.c_str(), &st);
    if (ret) {
        snprintf(buffer, bufsz, "Cannot load %s: %s", 
                 filename.c_str(), strerror(errno));
        throw new internal_exception(buffer, __FILE__, __LINE__);
    }
    
    // allocate buffer, safety net: files cannot be more than 1 GB 
    // (should be enough for now)
    size = st.st_size;
    if (size >= 1*1024*2014*1024) {
        snprintf(buffer, bufsz, "Cannot load %s: library is larger than 1GB",
                 filename.c_str());
        throw new internal_exception(buffer, __FILE__, __LINE__);
    }
    data = new char[size];
    
    // the file into the buffer
    file = fopen(filename.c_str(), "rb");
    if (!file) {
        delete[] data;
        snprintf(buffer, bufsz, "Cannot load %s: %s",
                 filename.c_str(), strerror(errno));
        throw new internal_exception(buffer, __FILE__, __LINE__);
    }
    if (fread(data, 1, size, file) != size) {
        delete[] data;
        snprintf(buffer, bufsz, "Cannot load %s: %s",
                 filename.c_str(), strerror(errno));
        fclose(file);
        throw new internal_exception(buffer, __FILE__, __LINE__);
    }
    if (fclose(file) != 0) {
        delete[] data;
        snprintf(buffer, bufsz, "Cannot load %s: %s",
                 filename.c_str(), strerror(errno));
        throw new internal_exception(buffer, __FILE__, __LINE__);
    }

    debug(10, "transferring %ld bytes to device %d", static_cast<long int>(size), target);
    size_t tempname_cstr_sz = sizeof(tempname_cstr);
    int64_t size_in = size;  // make this 64 bits for the transfer
#pragma offload target(mic:target) in(size_in) in(data:length(size_in)) in(bufsz) out(buffer) out(tempname_cstr) out(handle_device_ptr)
    {
        handle_device_ptr = target_load_library_device(buffer, bufsz,
                                                       data, size_in,
                                                       tempname_cstr,
                                                       tempname_cstr_sz);
    }

    tempname = tempname_cstr;
    
    delete[] data;
    
    if (!handle_device_ptr) {
		debug(10, "library load for '%s' failed on target: %s", 
              filename.c_str(), buffer);
        throw new internal_exception(buffer, __FILE__, __LINE__);
	}
    debug(10, "library load for '%s' succeeded on target, handle %p",
          filename.c_str(), handle_device_ptr);
    handle = handle_device_ptr;
    debug_leave();
}

#if defined(_WIN32)
__declspec(target(mic))
#else
__attribute__((target(mic)))
#endif
int target_unload_library_device(char *buffer, size_t bufsz, uintptr_t handle,
                                 const char *tempname_cstr) {
#if defined(__MIC__)
    // unload the library
    int errorcode = 0;
    void *library = reinterpret_cast<void *>(handle);
    dlerror();
    errorcode = dlclose(library);
    if (errorcode) {
        snprintf(buffer, bufsz, "dlclose failed ('%s')", dlerror());
        goto error_label_target_unload_library;
    }

    // delete temporary file
    errno = 0;
    errorcode = unlink(tempname_cstr);
    if (errorcode) {
        snprintf(buffer, bufsz, "dlclose failed ('%s')", dlerror());
        goto error_label_target_unload_library;
    }
error_label_target_unload_library:
    ;
    return errorcode;
#else
    // this function does not make sense on the host
    fprintf(stderr, "THIS SHOULD NOT HAPPEN!\n");
    exit(1);
    return 0;
#endif
}


void target_unload_library(int device, const std::string &tempname, uintptr_t handle) {
    debug_enter();

    int target = device;
    const char * tempname_cstr;
    const size_t bufsz = 256;
    char buffer[bufsz];
    int errorcode;
 
    debug(10, "unloading library 0x%x on target", handle);

    tempname_cstr = tempname.c_str();
#pragma offload target(mic:target) in(handle) in(bufsz) in(tempname_cstr:length(strlen(tempname_cstr)+1)) out(buffer) out(errorcode)
    {
        errorcode = target_unload_library_device(buffer, bufsz, handle,
                                                 tempname_cstr);
    }

    if (errorcode) {
        debug(10, "unloading library 0x%x failed on target: %s", handle, buffer);
        throw new internal_exception(buffer, __FILE__, __LINE__);
    }

    debug_leave();
}


#if defined(_WIN32)
__declspec(target(mic))
#else
__attribute__((target(mic)))
#endif
uintptr_t find_kernel_device(uintptr_t handle, const char *kernel_cstr) {
#if __MIC__
    uintptr_t fct_ptr = NULL;
    void *handle_device_ptr = reinterpret_cast<void*>(handle);
    void *ptr = NULL;
    dlerror();
    ptr = dlsym(handle_device_ptr, kernel_cstr);
    dlerror();
    fct_ptr = reinterpret_cast<uintptr_t>(ptr);
    if (!ptr) {
        dlerror();
    }
    return fct_ptr;
#else
    // this function does not make sense on the host
    return 0;
#endif
}


uintptr_t find_kernel(int device, uintptr_t handle, const std::string &kernel_name) {
	int target = device;
	const size_t bufsz = 256;
	char buffer[bufsz];
	const char *kernel_cstr = kernel_name.c_str();
	size_t kernel_sz = strlen(kernel_cstr) + 1;
	uintptr_t fct_ptr = NULL;
	
    // iterate through all loaded libraries for this device and try to find
    // the kernel function
    debug(100, "looking for '%s' library with handle %p", kernel_cstr, handle);
#pragma offload target(mic:target) in(kernel_cstr:length(kernel_sz)) in(handle) out(fct_ptr) 
    {
        fct_ptr = find_kernel_device(handle, kernel_cstr);
    }
	debug(100, "function pointer for '%s': %p", kernel_cstr, fct_ptr);

	return fct_ptr;
}


LIBXSTREAM_TARGET(mic)
void pymic_translate_pointer(const void *pointer, uintptr_t *translated) {
    /* pymic_translate_pointers(type *pointer, int *translated) */
    *translated = reinterpret_cast<uintptr_t>(pointer);
}

} // namespace pymic