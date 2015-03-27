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
 
#ifndef PYMIC_MISC_H
#define PYMIC_MISC_H

#include <string>
#include <stdint.h>

#include <exception>

namespace pymic {

enum dtype {
    dtype_int     = 0,
    dtype_float   = 1,
    dtype_complex = 2,
    dtype_uint64  = 3,
};

#if ! PYMIC_USE_XSTREAM
int get_number_of_devices();
#endif

void target_load_library(int device, const std::string &, 
                              std::string &, uintptr_t &);

void target_unload_library(int device, const std::string &, uintptr_t);

uintptr_t find_kernel(int device, uintptr_t handle, const std::string &kernel_name);

struct internal_exception : public std::exception {
    const char* file_;
    const int line_;
    const std::string reason_;
    
    internal_exception(const std::string &reason, const char *file, const int line) :
        reason_(reason), file_(file), line_(line) {
        // left blank
    }

    internal_exception(const char *file, const int line) :
        reason_("unknown reason"), file_(file), line_(line) {
        // left blank
    }
    
    virtual ~internal_exception() throw() {
        // left blank
    }
    
    const std::string reason() const {
        return reason_;
    }
    
    const char* file() const {
        return file_;
    }
    
    int line() const {
        return line_;
    }
}; // struct internal_exception

} // namespace pymic

#endif 

