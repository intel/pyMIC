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

#include <stdio.h> 
#include <sys/time.h>
 
#include <mkl.h>

int limiter(int ds) {
    if (ds < 128) 
        return 10000;
    if (ds < 1024)
        return 1000;
    if (ds < 8192)
        return 100;
    return 10;
}

int main(int argc, char* argv[]) {
    int ds;
    FILE *fh = NULL;
    
    fh = fopen("dgemm_c.csv", "w");
    if (!fh) {
        fprintf(stderr, "ERROR: Ccould not open file\n");
        exit(1);
    }
    
    fprintf(fh, "benchmark;elements;avg time;flops;gflops\n");
    for (ds = 128; ds <= 8192; ds += 128) {
        struct timeval t1, t2;
        double* a;
        double* b;
        double* c;
        double alpha = 1.0;
        double beta = 0.0;
        int m, n, k;
        int i;
        int nrep = limiter(ds);
        double flops;
        double t;
        double gflops;
        
        m = k = n = ds;
        a = (double*) mkl_malloc(m*k*sizeof(*a), 16);
        b = (double*) mkl_malloc(m*k*sizeof(*b), 16);
        c = (double*) mkl_malloc(m*k*sizeof(*c), 16);
        if (a == NULL || b == NULL || c == NULL) {
            fprintf(stderr, "ERROR: Can't allocate memory for matrices\n");
            mkl_free(a);
            mkl_free(b);
            mkl_free(c);
            exit(1);
        }
        printf("Measuring %dx%d (repeating %d)\n", ds, ds, nrep);
        gettimeofday(&t1, NULL);
        for (i = 0; i < nrep; ++i) {
            cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                        m, n, k, alpha, a, k, b, n, beta, c, n);
        }
        gettimeofday(&t2, NULL);
        
        flops = 2.0 * ((double) ds) * ((double) ds) * ((double) ds);
        t = (t2.tv_sec - t1.tv_sec) +
            ((t2.tv_usec - t1.tv_usec) / 1000.0 / 1000.0);
        t = t / nrep;
        gflops = ((double)flops) / (1000*1000*1000) / t;
        // fprintf(fh, "benchmark;elements;avg time;flops;gflops\n");
        fprintf(fh, "dgemm_c;%d;%lf;%lf;%lf\n", ds, t, flops, gflops);
        
        mkl_free(a);
        mkl_free(b);
        mkl_free(c);
    }    
    
    fclose(fh);
    
    return 0;
}
