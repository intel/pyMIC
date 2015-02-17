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
 
#include <mkl.h>
 
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
	
#if 0
	// just some debugging
	printf("m=%d n=%d k=%d alpha=%lf beta=%lf\n", m, n, k, alpha, beta);
	for (i = 0; i < m*k; i++) {
		printf("%lf %s", A[i], (i % k == (k-1)) ? "\n" : "");
	}
	printf("================================================================\n");
	for (i = 0; i < k*n; i++) {
		printf("%lf %s", B[i], (i % n == (n-1)) ? "\n" : "");
	}
#endif 
	
	cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
	            m, n, k, alpha, A, k, B, n, beta, C, n);

#if 0				
	printf("================================================================\n");
	for (i = 0; i < m*n; i++) {
		printf("%lf %s", C[i], (i % n == (n-1)) ? "\n" : "");
	}
#endif
}
