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

#include <pymic_kernel.h>
#include <stdlib.h>
#include <stdio.h>
#include <mkl.h>

PYMIC_KERNEL
void empty(int argc, uintptr_t argptr[], size_t sizes[]) {
#pragma omp parallel
    {
        // do nothing
    }
}

PYMIC_KERNEL
void dgemm_kernel(const double *A, const int64_t *trA, 
                  const double *B, const int64_t *trB,
                  double *C,
                  const int64_t *m, const int64_t *n, const int64_t *k,
                  const double *alpha, const double *beta) {
    int i;
    
    int lda, ldb;
    int transA, transB;
    
    /* check if we need to transpose A */
    lda = *k;
    transA = CblasNoTrans;
    if (*trA) {
        lda = *m;
        transA = CblasTrans;
    }
    
    /* check if we need to transpose B */
    ldb = *n;
    transB = CblasNoTrans;
    if (*trB) {
        ldb = *k;
        transB = CblasTrans;
    }
    
    cblas_dgemm(CblasRowMajor, transA, transB,
                (int) *m, (int) *n, (int) *k, 
                *alpha, A, lda, 
                B, ldb, 
                *beta, C, (int) *n);
}
