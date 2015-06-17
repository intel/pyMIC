/******************************************************************************
** Copyright (c) 2014-2015, Intel Corporation                                **
** All rights reserved.                                                      **
**                                                                           **
** Redistribution and use in source and binary forms, with or without        **
** modification, are permitted provided that the following conditions        **
** are met:                                                                  **
** 1. Redistributions of source code must retain the above copyright         **
**    notice, this list of conditions and the following disclaimer.          **
** 2. Redistributions in binary form must reproduce the above copyright      **
**    notice, this list of conditions and the following disclaimer in the    **
**    documentation and/or other materials provided with the distribution.   **
** 3. Neither the name of the copyright holder nor the names of its          **
**    contributors may be used to endorse or promote products derived        **
**    from this software without specific prior written permission.          **
**                                                                           **
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS       **
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT         **
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR     **
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT      **
** HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,    **
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED  **
** TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR    **
** PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF    **
** LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING      **
** NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS        **
** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.              **
******************************************************************************/
/* Hans Pabst (Intel Corp.)
******************************************************************************/
#include <libxstream_begin.h>
#include <stdexcept>
#include <algorithm>
#include <cstring>
#include <cstdio>
#if defined(_OPENMP)
# include <omp.h>
#endif
#include <libxstream_end.h>

//#define COPY_NO_SYNC


/**
 * This program is copying data to/from a copprocessor device (copy-in or copy-out).
 * A series of different sizes (up to a given / automatically selected amount) is
 * exercised in order to measure the bandwidth of the data transfers. The program
 * is multi-threaded (using only one thread by default) mainly to exercise the
 * thread-safety of LIBXSTREAM. There is no performance gain expected from using
 * multiple threads, remember that the number of streams determines the available
 * parallelism. The program is intentionally spartan (command line interface, etc.)
 * in order to keep it within bounds for an introductory code sample. The mechanism
 * selecting the stream to enqueue into as well as selecting the stream to be
 * synchronized shows the essence of the stream programing model.
 */
int main(int argc, char* argv[])
{
  try {
    size_t ndevices = 0;
    if (LIBXSTREAM_ERROR_NONE != libxstream_get_ndevices(&ndevices) || 0 == ndevices) {
      LIBXSTREAM_PRINT0(2, "No device found or device not ready!");
    }
    const int device = static_cast<int>(ndevices) - 1;
    size_t allocatable = 0;
    libxstream_mem_info(device, &allocatable, 0);
    allocatable >>= 20; // MB

    const bool copyin = 1 < argc ? ('o' != *argv[1]) : true;
    const size_t nstreams = std::min(std::max(2 < argc ? std::atoi(argv[2]) : 2, 1), LIBXSTREAM_MAX_NSTREAMS);
#if defined(_OPENMP)
    const int nthreads = std::min(std::max(3 < argc ? std::atoi(argv[3]) : 1, 1), omp_get_max_threads());
#else
    LIBXSTREAM_PRINT0(1, "OpenMP support needed for performance results!");
#endif
#if defined(LIBXSTREAM_OFFLOAD)
    const size_t nreserved = nstreams + 1;
#else
    const size_t nreserved = nstreams + 2;
#endif
    const size_t minsize = 8, maxsize = static_cast<size_t>(std::min(std::max(4 < argc ? std::atoi(argv[4]) : static_cast<int>(allocatable / nreserved), 1), 2048)) * (1 << 20);
    const int minrepeat = std::min(std::max(5 < argc ? std::atoi(argv[5]) :    8, 4), 2048);
    const int maxrepeat = std::min(std::max(6 < argc ? std::atoi(argv[6]) : 4096, minrepeat), 32768);

    int steps_repeat = 0, steps_size = 0;
    for (int nrepeat = minrepeat; nrepeat <= maxrepeat; nrepeat <<= 1) ++steps_repeat;
    for (size_t size = minsize; size <= maxsize; size <<= 1) ++steps_size;
    const int stride = 0 < steps_repeat ? (steps_size / steps_repeat + 1) : 1;

    struct {
      libxstream_stream* stream;
      void *mem_hst, *mem_dev;
    } copy[LIBXSTREAM_MAX_NSTREAMS];
    memset(copy, 0, sizeof(copy)); // some initialization (avoid false positives with tools)

    for (size_t i = 0; i < nstreams; ++i) {
      char name[128];
      LIBXSTREAM_SNPRINTF(name, sizeof(name), "Stream %i", static_cast<int>(i + 1));
      LIBXSTREAM_CHECK_CALL_THROW(libxstream_stream_create(&copy[i].stream, device, 0, name));
    }
    if (copyin) {
      LIBXSTREAM_CHECK_CALL_THROW(libxstream_mem_allocate(-1/*host*/, &copy[0].mem_hst, maxsize, 0));
      memset(copy[0].mem_hst, -1, maxsize); // some initialization (avoid false positives with tools)
      for (size_t i = 1; i < nstreams; ++i) {
        copy[i].mem_hst = copy[0].mem_hst;
      }
      for (size_t i = 0; i < nstreams; ++i) {
        LIBXSTREAM_CHECK_CALL_THROW(libxstream_mem_allocate(device, &copy[i].mem_dev, maxsize, 0));
      }
    }
    else { // copy-out
      LIBXSTREAM_CHECK_CALL_THROW(libxstream_mem_allocate(device, &copy[0].mem_dev, maxsize, 0));
      libxstream_memset_zero(copy[0].mem_dev, maxsize, copy[0].stream);
      for (size_t i = 1; i < nstreams; ++i) {
        copy[i].mem_dev = copy[0].mem_dev;
      }
      for (size_t i = 0; i < nstreams; ++i) {
        LIBXSTREAM_CHECK_CALL_THROW(libxstream_mem_allocate(-1/*host*/, &copy[i].mem_hst, maxsize, 0));
      }
    }
    // start benchmark with no pending work
    LIBXSTREAM_CHECK_CALL_THROW(libxstream_stream_wait(0));

    int n = 0, nrepeat = maxrepeat;
    double maxval = 0, sumval = 0, runlns = 0, duration = 0;
    for (size_t size = minsize; size <= maxsize; size <<= 1, ++n) {
      if (0 < n && 0 == (n % stride)) {
        nrepeat >>= 1;
      }

#if defined(_OPENMP)
      fprintf(stdout, "%lu Byte x %i: ", static_cast<unsigned long>(size), nrepeat);
      fflush(stdout); // make sure to show progress
      const double start = omp_get_wtime();
#     pragma omp parallel for num_threads(nthreads) schedule(dynamic)
#endif
      for (int i = 0; i < nrepeat; ++i) {
        const size_t n = std::min<size_t>(nstreams, nrepeat - i);

        for (size_t j = 0; j < n; ++j) { // enqueue work into streams
          if (copyin) {
            LIBXSTREAM_CHECK_CALL_ASSERT(libxstream_memcpy_h2d(copy[j].mem_hst, copy[j].mem_dev, size, copy[j].stream));
          }
          else { // copy-out
            LIBXSTREAM_CHECK_CALL_ASSERT(libxstream_memcpy_d2h(copy[j].mem_dev, copy[j].mem_hst, size, copy[j].stream));
          }
        }

#if !defined(COPY_NO_SYNC)
        for (size_t j = 0; j < n; ++j) { // synchronize streams
          const size_t k = n - j - 1; // j-reverse
          LIBXSTREAM_CHECK_CALL_ASSERT(libxstream_stream_wait(copy[k].stream));
        }
#endif
      }

#if defined(_OPENMP)
      const double iduration = omp_get_wtime() - start;
      if (0 < iduration) {
        const double bandwidth = (1.0 * size * nrepeat) / ((1ul << 20) * iduration);
        fprintf(stdout, "%.1f MB/s\n", bandwidth);
        maxval = std::max(maxval, bandwidth);
        sumval += bandwidth;
        runlns = (runlns + std::log(bandwidth)) * (0 < n ? 0.5 : 1.0);
        duration += iduration;
      }
#endif
    }

    if (copyin) {
      LIBXSTREAM_CHECK_CALL_THROW(libxstream_mem_deallocate(-1/*host*/, copy[0].mem_hst));
      for (size_t i = 0; i < nstreams; ++i) {
        LIBXSTREAM_CHECK_CALL_THROW(libxstream_mem_deallocate(device, copy[i].mem_dev));
      }
    }
    else { // copy-out
      LIBXSTREAM_CHECK_CALL_THROW(libxstream_mem_deallocate(device, copy[0].mem_dev));
      for (size_t i = 0; i < nstreams; ++i) {
        LIBXSTREAM_CHECK_CALL_THROW(libxstream_mem_deallocate(-1/*host*/, copy[i].mem_hst));
      }
    }
    for (size_t i = 0; i < nstreams; ++i) {
      LIBXSTREAM_CHECK_CALL_THROW(libxstream_stream_destroy(copy[i].stream));
    }

    fprintf(stdout, "\n");
    if (0 < n) {
      fprintf(stdout, "Finished after %.0f s\n", duration);
      fprintf(stdout, "max: %.0f MB/s\n", maxval);
      fprintf(stdout, "rgm: %.0f MB/s\n", std::exp(runlns) - 1.0);
      fprintf(stdout, "avg: %.0f MB/s\n", sumval / n);
    }
    else {
      fprintf(stdout, "Finished\n");
    }
  }
  catch(const std::exception& e) {
    fprintf(stderr, "Error: %s\n", e.what());
    return EXIT_FAILURE;
  }
  catch(...) {
    fprintf(stderr, "Error: unknown exception caught!\n");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
