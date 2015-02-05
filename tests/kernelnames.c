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

#include <pymic_kernel.h>

#include <complex.h>
#include <mkl.h>

PYMIC_KERNEL 
void kernel_underscores(int argc, uintptr_t argptr[], size_t sizes[]) {
    // do nothing
}

PYMIC_KERNEL
void a(int argc, uintptr_t argptr[], size_t sizes[]) {
    // do nothing
}

PYMIC_KERNEL
void bb(int argc, uintptr_t argptr[], size_t sizes[]) {
    // do nothing
}

PYMIC_KERNEL
void _bb(int argc, uintptr_t argptr[], size_t sizes[]) {
    // do nothing
}

PYMIC_KERNEL
void a123(int argc, uintptr_t argptr[], size_t sizes[]) {
    // do nothing
}

PYMIC_KERNEL
void test_offload_library_get(int argc, uintptr_t argptr[], size_t sizes[]) {
    uintptr_t * pointers = (uintptr_t *) argptr[0];
    
    pointers[0] = (uintptr_t) kernel_underscores;
    pointers[1] = (uintptr_t) a;
    pointers[2] = (uintptr_t) bb;
    pointers[3] = (uintptr_t) _bb;
    pointers[4] = (uintptr_t) a123;
}
