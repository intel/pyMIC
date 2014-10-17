/* Copyright (c) 2014, Intel Corporation All rights reserved. 
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
#include <string.h>


PYMIC_KERNEL
void offload_array_dadd(int argc, uintptr_t argptr[], size_t sizes[]) {
    /* offload_array_dadd(int n, double* x, int incx, double *y, int incy, double* result, int incr) */
	
	int n = *((int*) argptr[0]);
	double *x = (double*) argptr[1];
	int incx = *((int*) argptr[2]);
	double *y = (double*) argptr[3];
	int incy = *((int*) argptr[4]);;
	double *r = (double*) argptr[5];
	int incr = *((int*) argptr[6]);

	int i, ix, iy, ir;
	
    i = ix = iy = ir = 0;		   
	for (; i < n; i++, ix += incx, iy += incy, ir += incr) {
		r[ir] = x[ix] + y[iy];
	}
}

PYMIC_KERNEL
void offload_array_dsub(int argc, uintptr_t argptr[], size_t sizes[]) {
    /* offload_array_dsub(int n, double* x, int incx, double *y, int incy, double* result, int incr) */
	
	int n = *((int*) argptr[0]);
	double *x = (double*) argptr[1];
	int incx = *((int*) argptr[2]);
	double *y = (double*) argptr[3];
	int incy = *((int*) argptr[4]);
	double *r = (double*) argptr[5];
	int incr = *((int*) argptr[6]);

	int i, ix, iy, ir;
	
    i = ix = iy = ir = 0;		   
	for (; i < n; i++, ix += incx, iy += incy, ir += incr) {
		r[ir] = x[ix] - y[iy];
	}
}

PYMIC_KERNEL
void offload_array_dmul(int argc, uintptr_t argptr[], size_t sizes[]) {
    /* offload_array_dmul(int n, double* x, int incx, double *y, int incy, double* result, int incr) */
	
	int n = *((int*) argptr[0]);
	double *x = (double*) argptr[1];
	int incx = *((int*) argptr[2]);
	double *y = (double*) argptr[3];
	int incy = *((int*) argptr[4]);
	double *r = (double*) argptr[5];
	int incr = *((int*) argptr[6]);

	int i, ix, iy, ir;
	
    i = ix = iy = ir = 0;		   
	for (; i < n; i++, ix += incx, iy += incy, ir += incr) {
		r[ir] = x[ix] * y[iy];
	}
}

PYMIC_KERNEL
void offload_array_dfill(int argc, uintptr_t argptr[], size_t sizes[]) {
	/* offload_array_dfill(int n, double* x, double value) */
	
	int n = *((int*) argptr[0]);
	double *x = (double*) argptr[1];
	double v = *((double*) argptr[2]);
	
	int i;
	
	i = 0;
	for (; i < n; i++) {
		x[i] = v;
	}
}

PYMIC_KERNEL
void offload_array_fillfrom(int argc, uintptr_t argptr[], size_t sizes[]) {
	/* offload_array_fillfrom(char* x, char *y, int nbytes) */
	
	char *x = (char*) argptr[0];
	char *y = (char*) argptr[1];
	int nbytes = *((int*) argptr[2]);
	
	memcpy(x, y, nbytes);
}

PYMIC_KERNEL
void offload_array_dabs(int argc, uintptr_t argptr[], size_t sizes[]) {
	/* offload_array_dabs(int n, double* x, double *result) */
	
	int n = *((int*) argptr[0]);
	double *x = (double*) argptr[1];
	double *r = (double*) argptr[2];
	
	int i;
	
	i = 0;
	for (; i < n; i++) {
		r[i] = fabs(x[i]);
	}
}

PYMIC_KERNEL
void offload_array_dpow(int argc, uintptr_t argptr[], size_t sizes[]) {
    /* offload_array_dpow(int n, double* x, int incx, double *y, int incy, double* result, int incr) */
	
	int n = *((int*) argptr[0]);
	double *x = (double*) argptr[1];
	int incx = *((int*) argptr[2]);
	double *y = (double*) argptr[3];
	int incy = *((int*) argptr[4]);
	double *r = (double*) argptr[5];
	int incr = *((int*) argptr[6]);

	int i, ix, iy, ir;
	
    i = ix = iy = ir = 0;		   
	for (; i < n; i++, ix += incx, iy += incy, ir += incr) {
		r[ir] = pow(x[ix], y[iy]);
	}
}

PYMIC_KERNEL
void offload_array_dreverse(int argc, uintptr_t argptr[], size_t sizes[]) {
	/* offload_array_dreverse(int n, double* x, double *result) */
	
	int n = *((int*) argptr[0]);
	double *x = (double*) argptr[1];
	double *r = (double*) argptr[2];
	
	int i;
	
	i = 0;
	for (; i < n; i++) {
		r[i] = x[n - i - 1];
	}
}

