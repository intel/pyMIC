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

#include <stdlib.h>
 
#include <mkl.h>

PYMIC_KERNEL
void empty(int argc, uintptr_t argptr[], size_t sizes[]) {
#pragma omp parallel
    {
        // do nothing
    }
}
 
PYMIC_KERNEL
void dgemm_kernel(int argc, uintptr_t argptr[], size_t sizes[]) {
	int i;

	double *A = (double*) argptr[0];
	double *B = (double*) argptr[1];
	double *C = (double*) argptr[2];

	int m = *(long int*) argptr[3];
	int n = *(long int*) argptr[4];
	int k = *(long int*) argptr[5];
	
	double alpha = *(double*) argptr[6];
	double beta = *(double*) argptr[7];
	
	cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
	            m, n, k, alpha, A, k, B, n, beta, C, n);
}

PYMIC_KERNEL
void svd_reconstruct(int argc, uintptr_t argptr[], size_t sizes[]) {
	int i;

	double *U     = (double*) argptr[0];
	double *sigma = (double*) argptr[1];
	double *V     = (double*) argptr[2];
    double *res   = (double*) argptr[3];

	int x = *(long int*) argptr[4];
	int y = *(long int*) argptr[5];
	int z = *(long int*) argptr[6];
	
    double *tmp   = (double*) malloc(sizeof(double) * x * z);
    
    /* tmp = U * sigma*/
    cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                x, z, z, 1.0, U, z, sigma, z, 0.0, tmp, z);
    
    /* res = tmp * V */
    cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                x, y, z, 1.0, tmp, z, V, y, 0.0, res, y);
    
    free(tmp);
}
