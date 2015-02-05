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

/* Data types, needs to match _data_type_map in _misc.py */
#define DTYPE_INT       0
#define DTYPE_FLOAT     1
#define DTYPE_COMPLEX   2

PYMIC_KERNEL
void pymic_offload_array_add(int argc, uintptr_t argptr[], size_t sizes[]) {
    /* pymic_offload_array_add(int n, type * x, int incx, type * y, 
                         int incy, type * result, int incr) */
	
	size_t i, ix, iy, ir;
	
    int dtype   = *(long int *) argptr[0];
	int n       = *(long int *) argptr[1];
	int incx    = *(long int *) argptr[3];
	int incy    = *(long int *) argptr[5];
	int incr    = *(long int *) argptr[7];

	i = ix = iy = ir = 0;		   
    switch(dtype) {
    case DTYPE_INT:
        {
            long int * x = (long int *) argptr[2];
            long int * y = (long int *) argptr[4];
            long int * r = (long int *) argptr[6];
            for (; i < n; i++, ix += incx, iy += incy, ir += incr) {
                r[ir] = x[ix] + y[iy];
            }
        }
        break;
    case DTYPE_FLOAT:
        {
            double * x = (double *) argptr[2];
            double * y = (double *) argptr[4];
            double * r = (double *) argptr[6];
            for (; i < n; i++, ix += incx, iy += incy, ir += incr) {
                r[ir] = x[ix] + y[iy];
            }
        }
        break;
    case DTYPE_COMPLEX:
        {
            double complex * x = (double complex *) argptr[2];
            double complex * y = (double complex *) argptr[4];
            double complex * r = (double complex *) argptr[6];
            for (; i < n; i++, ix += incx, iy += incy, ir += incr) {
                r[ir] = x[ix] + y[iy];
            }
        }
        break;
    }
}

PYMIC_KERNEL
void pymic_offload_array_sub(int argc, uintptr_t argptr[], size_t sizes[]) {
    /* pymic_offload_array_sub(int dtype, int n, type * x, int incx, type * y, 
                         int incy, type * result, int incr) */
	
	size_t i, ix, iy, ir;
	
    int dtype   = *(long int *) argptr[0];
	int n       = *(long int *) argptr[1];
	int incx    = *(long int *) argptr[3];
	int incy    = *(long int *) argptr[5];
	int incr    = *(long int *) argptr[7];

	i = ix = iy = ir = 0;		   
    switch(dtype) {
    case DTYPE_INT:
        {
            long int * x = (long int *) argptr[2];
            long int * y = (long int *) argptr[4];
            long int * r = (long int *) argptr[6];
            for (; i < n; i++, ix += incx, iy += incy, ir += incr) {
                r[ir] = x[ix] - y[iy];
            }
        }
        break;
    case DTYPE_FLOAT:
        {
            double * x = (double *) argptr[2];
            double * y = (double *) argptr[4];
            double * r = (double *) argptr[6];
            for (; i < n; i++, ix += incx, iy += incy, ir += incr) {
                r[ir] = x[ix] - y[iy];
            }
        }
        break;
    case DTYPE_COMPLEX:
        {
            double complex * x = (double complex *) argptr[2];
            double complex * y = (double complex *) argptr[4];
            double complex * r = (double complex *) argptr[6];
            for (; i < n; i++, ix += incx, iy += incy, ir += incr) {
                r[ir] = x[ix] - y[iy];
            }
        }
        break;
    }
}

PYMIC_KERNEL
void pymic_offload_array_mul(int argc, uintptr_t argptr[], size_t sizes[]) {
    /* pymic_offload_array_mul(int dtype, int n, type * x, int incx, type * y, 
                         int incy, type * result, int incr) */
	size_t i, ix, iy, ir;
	
    int dtype   = *(long int *) argptr[0];
	int n       = *(long int *) argptr[1];
	int incx    = *(long int *) argptr[3];
	int incy    = *(long int *) argptr[5];
	int incr    = *(long int *) argptr[7];

	i = ix = iy = ir = 0;		   
    switch(dtype) {
    case DTYPE_INT:
        {
            long int * x = (long int *) argptr[2];
            long int * y = (long int *) argptr[4];
            long int * r = (long int *) argptr[6];
            for (; i < n; i++, ix += incx, iy += incy, ir += incr) {
                r[ir] = x[ix] * y[iy];
            }
        }
        break;
    case DTYPE_FLOAT:
        {
            double * x = (double *) argptr[2];
            double * y = (double *) argptr[4];
            double * r = (double *) argptr[6];
            for (; i < n; i++, ix += incx, iy += incy, ir += incr) {
                r[ir] = x[ix] * y[iy];
            }
        }
        break;
    case DTYPE_COMPLEX:
        {
            double complex * x = (double complex *) argptr[2];
            double complex * y = (double complex *) argptr[4];
            double complex * r = (double complex *) argptr[6];
            for (; i < n; i++, ix += incx, iy += incy, ir += incr) {
                r[ir] = x[ix] * y[iy];
            }
        }
        break;
    }
}

PYMIC_KERNEL
void pymic_offload_array_fill(int argc, uintptr_t argptr[], size_t sizes[]) {
	/* pymic_offload_array_fill(int dtype, int n, type * x, type value) */
    size_t i;
	
    int dtype = *(long int *) argptr[0];
	int n     = *(long int *) argptr[1];

    switch(dtype) {
    case DTYPE_INT:
        {
            long int * x  =  (long int *) argptr[2];
            long int   v  = *(long int *) argptr[3];
            for (i = 0; i < n; i++) {
                x[i] = v;
            }
        }
        break;
    case DTYPE_FLOAT:
        {
            double * x  =  (double *) argptr[2];
            double   v  = *(double *) argptr[3];
            for (i = 0; i < n; i++) {
                x[i] = v;
            }
        }
        break;
    case DTYPE_COMPLEX:
        {
            double complex * x  =  (double complex *) argptr[2];
            double complex   v  = *(double complex *) argptr[3];
            for (i = 0; i < n; i++) {
                x[i] = v;
            }
        }
        break;
    }
}

PYMIC_KERNEL
void pymic_offload_array_fillfrom(int argc, uintptr_t argptr[], size_t sizes[]) {
	/* pymic_offload_array_fillfrom(char* x, char *y, int nbytes) */
	
	char * x   =  (char *)     argptr[0];
	char * y   =  (char *)     argptr[1];
	int nbytes = *(long int *) argptr[2];
	
	memcpy(x, y, nbytes);
}

PYMIC_KERNEL
void pymic_offload_array_setslice(int argc, uintptr_t argptr[], size_t sizes[]) {
	/* pymic_offload_array_setslice(int lower, int upper, 
                                    type * x, type * y) */
	
    int      dtype = *(long int *) argptr[0];
    long int lower = *(long int *) argptr[1];
    long int upper = *(long int *) argptr[2];
	
    char *       x =  (char *)     argptr[3];
    char *       y =  (char *)     argptr[4];
    
    int scale = 0;
    int nbytes;
    switch(dtype) {
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
    nbytes = (upper - lower) * scale;
    
	memcpy(x + (lower * scale), y, nbytes);
}

PYMIC_KERNEL
void pymic_offload_array_abs(int argc, uintptr_t argptr[], size_t sizes[]) {
	/* pymic_offload_array_abs(int dtype, int n, type * x, type * result) */
    
    size_t i;
    
    int dtype = *(long int *) argptr[0];
	int n     = *(long int *) argptr[1];

    switch(dtype) {
    case DTYPE_INT:
        {
            long int * x = (long int *) argptr[2];
            long int * r = (long int *) argptr[3];
            for (i = 0; i < n; i++) {
                r[i] = labs(x[i]);
            }
        }
        break;
    case DTYPE_FLOAT:
        {
            double * x = (double *) argptr[2];
            double * r = (double *) argptr[3];
            for (i = 0; i < n; i++) {
                r[i] = fabs(x[i]);
            }
        }
        break;
    case DTYPE_COMPLEX:
        {
            double complex * x = (double complex *) argptr[2];
            double * r = (double *) argptr[3];
            for (i = 0; i < n; i++) {
                r[i] = cabs(x[i]);
            }
        }
        break;
    }
}

PYMIC_KERNEL
void pymic_offload_array_pow(int argc, uintptr_t argptr[], size_t sizes[]) {
    /* pymic_offload_array_pow(int dtype, int n, double * x, int incx, type * y, 
                               int incy, type * result, int incr) */
	
	size_t i, ix, iy, ir;

    int dtype = *(long int *) argptr[0];
	int n     = *(long int *) argptr[1];
	int incx  = *(long int *) argptr[3];
	int incy  = *(long int *) argptr[5];
	int incr  = *(long int *) argptr[7];

    switch(dtype) {
    case DTYPE_INT:
        {
            long int * x = (long int *) argptr[2];
            long int * y = (long int *) argptr[4];
            long int * r = (long int *) argptr[6];
            
            i = ix = iy = ir = 0;		   
            for (; i < n; i++, ix += incx, iy += incy, ir += incr) {
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
            double * x = (double *) argptr[2];
            double * y = (double *) argptr[4];
            double * r = (double *) argptr[6];
            
            i = ix = iy = ir = 0;		   
            for (; i < n; i++, ix += incx, iy += incy, ir += incr) {
                r[ir] = pow(x[ix], y[iy]);
            }
        }
        break;
    case DTYPE_COMPLEX:
        {
            double complex * x = (double complex *) argptr[2];
            double complex * y = (double complex *) argptr[4];
            double complex * r = (double complex *) argptr[6];
            
            i = ix = iy = ir = 0;		   
            for (; i < n; i++, ix += incx, iy += incy, ir += incr) {
                r[ir] = cpow(x[ix], y[iy]);
            }
        }
        break;
    }
}

PYMIC_KERNEL
void pymic_offload_array_reverse(int argc, uintptr_t argptr[], size_t sizes[]) {
	/* pymic_offload_array_dreverse(int dtype, int n, type * x, type * result) */
	
    size_t i;
    
    int dtype = *(long int *) argptr[0];
	int n =     *(long int *) argptr[1];
    
    switch(dtype) {
    case DTYPE_INT:
        {
            long int * x = (long int *) argptr[2];
            long int * r = (long int *) argptr[3];
            for (i = 0; i < n; i++) {
                r[i] = x[n - i - 1];
            }
        }
        break;
    case DTYPE_FLOAT:
        {
            double * x = (double *) argptr[2];
            double * r = (double *) argptr[3];
            for (i = 0; i < n; i++) {
                r[i] = x[n - i - 1];
            }
        }
        break;
    case DTYPE_COMPLEX:
        {
            double complex * x = (double complex *) argptr[2];
            double complex * r = (double complex *) argptr[3];
            for (i = 0; i < n; i++) {
                r[i] = x[n - i - 1];
            }
        }
        break;
    }
}
