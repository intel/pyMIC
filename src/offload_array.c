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

#include <stdio.h>
#include <math.h>
#include <complex.h>
#include <string.h>
#include <stdlib.h>

/* Data types, needs to match _data_type_map in _misc.py */
#define DTYPE_INT       0
#define DTYPE_FLOAT     1
#define DTYPE_COMPLEX   2
#define DTYPE_UINT64    3

#define print printf


PYMIC_KERNEL
void pymic_offload_array_add(const long int *dtype, const long int *n,
                             const void *x_, const long int *incx,
                             const void *y_, const long int *incy,
                             void *r_, const long int *incr) {
    /* pymic_offload_array_add(int n, type  *x, int incx, type  *y,
                         int incy, type  *r, int incr) */

    size_t i, ix, iy, ir;
    i = ix = iy = ir = 0;
    switch(*dtype) {
    case DTYPE_INT:
        {
            const long int *x = (const long int*)x_;
            const long int *y = (const long int*)y_;
            long int *r = (long int*)r_;
            for (; i < *n; i++, ix += *incx, iy += *incy, ir += *incr) {
                r[ir] = x[ix] + y[iy];
            }
        }
        break;
    case DTYPE_FLOAT:
        {
            const double *x = (const double*)x_;
            const double *y = (const double*)y_;
            double *r = (double*)r_;
            for (; i < *n; i++, ix += *incx, iy += *incy, ir += *incr) {
                r[ir] = x[ix] + y[iy];
            }
        }
        break;
    case DTYPE_COMPLEX:
        {
            const double complex *x = (const double complex*)x_;
            const double complex *y = (const double complex*)y_;
            double complex *r = (double complex*)r_;
            for (; i < *n; i++, ix += *incx, iy += *incy, ir += *incr) {
                r[ir] = x[ix] + y[iy];
            }
        }
        break;
    }
}


PYMIC_KERNEL
void pymic_offload_array_sub(const long int *dtype, const long int *n,
                             const void *x_, const long int *incx,
                             const void *y_, const long int *incy,
                             void *r_, const long int *incr) {
    /* pymic_offload_array_sub(int dtype, int n, type  *x, int incx, type  *y,
                         int incy, type  *result, int incr) */

    size_t i, ix, iy, ir;
    i = ix = iy = ir = 0;
    switch(*dtype) {
    case DTYPE_INT:
        {
            const long int *x = (const long int*)x_;
            const long int *y = (const long int*)y_;
            long int *r = (long int*)r_;
            for (; i < *n; i++, ix += *incx, iy += *incy, ir += *incr) {
                r[ir] = x[ix] - y[iy];
            }
        }
        break;
    case DTYPE_FLOAT:
        {
            const double *x = (const double *)x_;
            const double *y = (const double *)y_;
            double *r = (double*)r_;
             for (; i < *n; i++, ix += *incx, iy += *incy, ir += *incr) {
                r[ir] = x[ix] - y[iy];
            }
        }
        break;
    case DTYPE_COMPLEX:
        {
            const double complex *x = (const double complex *)x_;
            const double complex *y = (const double complex *)y_;
            double complex *r = (double complex *)r_;
            for (; i < *n; i++, ix += *incx, iy += *incy, ir += *incr) {
                r[ir] = x[ix] - y[iy];
            }
        }
        break;
    }
}


PYMIC_KERNEL
void pymic_offload_array_mul(const long int *dtype, const long int *n,
                             const void *x_, const long int *incx,
                             const void *y_, const long int *incy,
                             void *r_, const long int *incr) {
    /* pymic_offload_array_mul(int dtype, int n, type  *x, int incx, type  *y,
                         int incy, type  *result, int incr) */
    size_t i, ix, iy, ir;
    i = ix = iy = ir = 0;
    switch(*dtype) {
    case DTYPE_INT:
        {
            const long int *x = (const long int*)x_;
            const long int *y = (const long int*)y_;
            long int *r = (long int*)r_;
            for (; i < *n; i++, ix += *incx, iy += *incy, ir += *incr) {
                r[ir] = x[ix] * y[iy];
            }
        }
        break;
    case DTYPE_FLOAT:
        {
            const double *x = (const double*)x_;
            const double *y = (const double*)y_;
            double *r = (double*)r_;
            for (; i < *n; i++, ix += *incx, iy += *incy, ir += *incr) {
                r[ir] = x[ix] * y[iy];
            }
        }
        break;
    case DTYPE_COMPLEX:
        {
            const double complex *x = (const double complex*)x_;
            const double complex *y = (const double complex*)y_;
            double complex *r = (double complex*)r_;
            for (; i < *n; i++, ix += *incx, iy += *incy, ir += *incr) {
                r[ir] = x[ix] * y[iy];
            }
        }
        break;
    }
}


PYMIC_KERNEL
void pymic_offload_array_fill(const long int *dtype, const long int *n,
                              void *ptr, ...) {
    /* pymic_offload_array_fill(int dtype, int n,
                                type  *x, type value) */
    size_t i;

    const void *data = 0;
    libxstream_get_data(0, 3/*va_arg*/, &data);

    switch(*dtype) {
    case DTYPE_INT:
        {
            long int  *x  = (long int*)ptr;
            long int   v  = *(const long int*)data;
            for (i = 0; i < *n; i++) {
                x[i] = v;
            }
        }
        break;
    case DTYPE_FLOAT:
        {
            double  *x  = (double*)ptr;
            double   v  = *(const double*)data;
            for (i = 0; i < *n; i++) {
                x[i] = v;
            }
        }
        break;
    case DTYPE_COMPLEX:
        {
            double complex  *x  = (double complex*)ptr;
            double complex   v  = *(const double complex*)data;
            for (i = 0; i < *n; i++) {
                x[i] = v;
            }
        }
        break;
    }
}


PYMIC_KERNEL
void pymic_offload_array_fillfrom(void *dst, const void *src, const long int *nbytes) {
    /* pymic_offload_array_fillfrom(type  *dst, type  *src, int nbytes) */
    memcpy(dst, src, *nbytes);
}


PYMIC_KERNEL
void pymic_offload_array_setslice(const long int *dtype,
                                  const long int *lower, const long int *upper,
                                  void *dst, const void *src) {
    /* pymic_offload_array_setslice(int lower, int upper,
                                    type  *dst, type  *src) */
    int scale = 0;
    int nbytes;
    switch(*dtype) {
    case DTYPE_INT:
        scale = 8; /* bytes */
        break;
    case DTYPE_FLOAT:
        scale = 8; /* bytes */
        break;
    case DTYPE_COMPLEX:
        scale = 16; /* bytes */
        break;
    }
    nbytes = ((*upper) - (*lower)) * scale;
    memcpy(dst + ((*lower) * scale), src, nbytes);
}


PYMIC_KERNEL
void pymic_offload_array_abs(const long int *dtype, const long int *n,
                             const void *x_, void *r_) {
    /* pymic_offload_array_abs(int dtype, int n,
                               type  *x, type  *result) */
    size_t i;
    switch(*dtype) {
    case DTYPE_INT:
        {
            const long int *x = (const long int*)x_;
            long int *r = (long int*)r_;
            for (i = 0; i < *n; i++) {
                r[i] = labs(x[i]);
            }
        }
        break;
    case DTYPE_FLOAT:
        {
            const double *x = (const double*)x_;
            double *r = (double*)r_;
            for (i = 0; i < *n; i++) {
                r[i] = fabs(x[i]);
            }
        }
        break;
    case DTYPE_COMPLEX:
        {
            const double complex *x = (const double complex*)x_;
            double *r = (double*)r_;
            for (i = 0; i < *n; i++) {
                r[i] = cabs(x[i]);
            }
        }
        break;
    }
}


PYMIC_KERNEL
void pymic_offload_array_pow(const long int *dtype, const long int *n,
                             const void *x_, const long int *incx,
                             const void *y_, const long int *incy,
                             void *r_, const long int *incr) {
    /* pymic_offload_array_pow(int dtype, int n, type  *x, int incx, type  *y,
                               int incy, type  *result, int incr) */
    size_t i, ix, iy, ir;
    switch(*dtype) {
    case DTYPE_INT:
        {
            const long int *x = (const long int*)x_;
            const long int *y = (const long int*)y_;
            long int *r = (long int*)r_;
            i = ix = iy = ir = 0;
            for (; i < *n; i++, ix += *incx, iy += *incy, ir += *incr) {
                size_t j;
                r[ir] = 1;
                for (j = 0; j < y[iy]; j++) {
                    r[ir] *= x[ix];
                }
            }
        }
        break;
    case DTYPE_FLOAT:
        {
            const double *x = (const double*)x_;
            const double *y = (const double*)y_;
            double *r = (double*)r_;
            i = ix = iy = ir = 0;
            for (; i < *n; i++, ix += *incx, iy += *incy, ir += *incr) {
                r[ir] = pow(x[ix], y[iy]);
            }
        }
        break;
    case DTYPE_COMPLEX:
        {
            const double complex *x = (const double complex*)x_;
            const double complex *y = (const double complex*)y_;
            double complex *r = (double complex*)r_;
            i = ix = iy = ir = 0;
            for (; i < *n; i++, ix += *incx, iy += *incy, ir += *incr) {
                r[ir] = cpow(x[ix], y[iy]);
            }
        }
        break;
    }
}


PYMIC_KERNEL
void pymic_offload_array_reverse(const long int *dtype, const long int *n,
                                 const void *x_, void *r_) {
    /* pymic_offload_array_dreverse(int dtype, int n,
                                    type  *x, type  *result) */
    size_t i;
    switch(*dtype) {
    case DTYPE_INT:
        {
            const long int *x = (const long int*)x_;
            long int *r = (long int*)r_;
            for (i = 0; i < *n; i++) {
                r[i] = x[*n - i - 1];
            }
        }
        break;
    case DTYPE_FLOAT:
        {
            const double *x = (const double*)x_;
            double *r = (double*)r_;
            for (i = 0; i < *n; i++) {
                r[i] = x[*n - i - 1];
            }
        }
        break;
    case DTYPE_COMPLEX:
        {
            const double complex *x = (const double complex*)x_;
            double complex *r = (double complex*)r_;
            for (i = 0; i < *n; i++) {
                r[i] = x[*n - i - 1];
            }
        }
        break;
    }
}
