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
void test_offload_stream_empty(void) {
    // do nothing
}

PYMIC_KERNEL
void test_offload_stream_nullptr_1(const void *pointer, int64_t *result)  {
    *result = (int64_t) pointer;
}

PYMIC_KERNEL
void test_offload_stream_nullptr_2(const void *dummy1, const void *pointer,
                                  int64_t *result)  {
    *result = (int64_t) pointer;
}

PYMIC_KERNEL
void test_offload_stream_nullptr_3(const void *dummy1, const void *pointer,
                                   const void *dummy2, int64_t *result)  {
    *result = (int64_t) pointer;
}

PYMIC_KERNEL
void test_offload_stream_nullptr_4(const void *dummy1, const void *pointer1,
                                   const void *pointer2, const void *dummy2,
                                   int64_t *result)  {
    *result = (int64_t) pointer1 + (int64_t) pointer2;
}

PYMIC_KERNEL
void test_offload_stream_nullptr_5(const void *dummy1, const void *pointer1,
                                   const void *dummy2, const void *pointer2,
                                   const void *dummy3, int64_t *result)  {
    *result = (int64_t) pointer1 + (int64_t) pointer2;
}

PYMIC_KERNEL
void test_offload_stream_kernel_scalars(const long int *a, 
                                        const double *b, 
                                        const long int *c, 
                                        const double *d, 
                                        long int *ival, 
                                        double *dval) {
    ival[0] = *a;
    ival[1] = *c;
    dval[0] = *b;
    dval[1] = *d;
}

PYMIC_KERNEL
void test_offload_stream_kernel_arrays_int(long int *a, long int *b, 
                                           const long int *na, 
                                           const long int *nb,
                                           const long int *d, 
                                           const long int *m) {
    size_t i;
    for (i = 0; i < *na; i++) {
        a[i] = a[i] / *d;
    }
    for (i = 0; i < *nb; i++) {
        b[i] = b[i] % *m;
    }
}

PYMIC_KERNEL
void test_offload_stream_kernel_arrays_float(double *a, double *b, 
                                             long int *na, long int *nb, 
                                             double *p, double *s) {
    size_t i;
    for (i = 0; i < *na; i++) {
        a[i] = a[i] + *p;
    }
    for (i = 0; i < *nb; i++) {
        b[i] = b[i] - *s;
    }
}

PYMIC_KERNEL
void test_check_pattern(const long int *a, const long int *n, 
                        long int *r, const uint64_t *pattern) {
    size_t i;
    long int cnt = 0;
    for (i = 0; i < *n; i++) {
        cnt += ((uint64_t) a[i]) == *pattern;
    }
    r[0] = cnt;
}

PYMIC_KERNEL
void test_set_pattern(long int *a, long int *n, long int *pattern) {
    size_t i;
    for (i = 0; i < *n; i++) {
        a[i] = *pattern;
    }
}

PYMIC_KERNEL
void test_sum_float(const double *a, long int *n, double *r) {
    size_t i;
    r[0] = 0.0;
    for (i = 0; i < *n; i++) {
        r[0] += a[i];
    }
}

PYMIC_KERNEL
void test_kernel_dgemm(const double *A, const double *B, double *C, 
                       const long int *m, const long int *n, const long int *k,
                       const double *alpha, const double *beta) {
    cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                *m, *n, *k, *alpha, A, *k, B, *n, *beta, C, *n);
}

PYMIC_KERNEL
void test_kernel_zgemm(const double complex *A, const double complex *B, 
                       double complex *C, const long int *m, const long int *n, 
                       const long int *k, const double complex *alpha, 
                       const double complex *beta) {
    cblas_zgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                *m, *n, *k, alpha, A, *k, B, *n, beta, C, *n);
}
