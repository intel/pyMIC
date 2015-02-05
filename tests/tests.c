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
void test_offload_stream_empty(int argc, uintptr_t argptr[], size_t sizes[]) {
    // do nothing
}

PYMIC_KERNEL
void test_offload_stream_kernel_scalars(int argc, uintptr_t argptr[], size_t sizes[]) {
    int a             = *(long int*) argptr[0];
    double b          = *(double*)   argptr[1];
    int c             = *(long int*) argptr[2];
    double d          = *(int*)      argptr[3];
    
    long int* ival    = (long int*)  argptr[4];
    double* dval      = (double*)    argptr[5];
    long int* count   = (long int*)  argptr[6];
    
    ival[0] = a;
    ival[1] = c;
    dval[0] = b;
    dval[1] = d;
    count[0] = argc;
}

PYMIC_KERNEL
void test_offload_stream_kernel_arrays_int(int argc, uintptr_t argptr[], size_t sizes[]) {
    size_t i;

    long int*  a = (long int *) argptr[0];
    long int*  b = (long int *) argptr[1];
    long int*  c = (long int *) argptr[2];

    long int   d = *(long int *) argptr[3];
    long int   m = *(long int *) argptr[4];
    
    c[0] = sizes[0] / sizeof(long int);
    c[1] = sizes[1] / sizeof(long int);

    for (i = 0; i < (sizes[0] / sizeof(long int)); i++) {
        a[i] = a[i] / d;
    }

    for (i = 0; i < (sizes[1] / sizeof(long int)); i++) {
        b[i] = b[i] % m;
    }
}

PYMIC_KERNEL
void test_offload_stream_kernel_arrays_float(int argc, uintptr_t argptr[], size_t sizes[]) {
    size_t i;

    double*    a = (double *)   argptr[0];
    double*    b = (double *)   argptr[1];
    long int*  c = (long int *) argptr[2];

    double   p = *(double *) argptr[3];
    double   s = *(double *) argptr[4];
    
    c[0] = sizes[0] / sizeof(double);
    c[1] = sizes[1] / sizeof(double);

    for (i = 0; i < (sizes[0] / sizeof(double)); i++) {
        a[i] = a[i] + p;
    }

    for (i = 0; i < (sizes[1] / sizeof(double)); i++) {
        b[i] = b[i] - s;
    }
}

PYMIC_KERNEL
void test_check_pattern(int argc, uintptr_t argptr[], size_t sizes[]) {
    size_t i;
    
    long int* a       = (long int*) argptr[0];
    long int* r       = (long int*) argptr[1];
    
    long int  pattern = *(long int*) argptr[2];
    
    r[0] = 0;
    for (i = 0; i < sizes[0] / sizeof(long int); i++) {
        r[0] += a[i] == pattern;
    }
}

PYMIC_KERNEL
void test_set_pattern(int argc, uintptr_t argptr[], size_t sizes[]) {
    size_t i;
    
    long int* a       = (long int*) argptr[0];

    long int  pattern = *(long int*) argptr[1];
    
    for (i = 0; i < sizes[0] / sizeof(long int); i++) {
        a[i] = pattern;
    }
}

PYMIC_KERNEL
void test_sum_float(int argc, uintptr_t argptr[], size_t sizes[]) {
    size_t i;
    
    double *a = (double*) argptr[0];
    double *r = (double*) argptr[1];
    
    r[0] = 0.0;
    for (i = 0; i < sizes[0] / sizeof(double); i++) {
        r[0] += a[i];
    }
}

PYMIC_KERNEL
void test_kernel_dgemm(int argc, uintptr_t argptr[], size_t sizes[]) {
	double *     A = (double *)    argptr[0];
	double *     B = (double *)    argptr[1];
	double *     C = (double *)    argptr[2];
	int          m = *(long int *) argptr[3];
	int          n = *(long int *) argptr[4];
	int          k = *(long int *) argptr[5];
	double   alpha = *(double *)   argptr[6];
	double   beta  = *(double *)   argptr[7];

    cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                m, n, k, alpha, A, k, B, n, beta, C, n);
}

PYMIC_KERNEL
void test_kernel_zgemm(int argc, uintptr_t argptr[], size_t sizes[]) {
	double complex *     A =  (double complex *) argptr[0];
	double complex *     B =  (double complex *) argptr[1];
	double complex *     C =  (double complex *) argptr[2];
	int                  m = *(long int *)       argptr[3];
	int                  n = *(long int *)       argptr[4];
	int                  k = *(long int *)       argptr[5];
	double complex * alpha =  (double complex *) argptr[6];
	double complex * beta  =  (double complex *) argptr[7];

    cblas_zgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                m, n, k, alpha, A, k, B, n, beta, C, n);
}