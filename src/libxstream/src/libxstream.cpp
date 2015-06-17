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
#if defined(LIBXSTREAM_EXPORTED) || defined(__LIBXSTREAM)
#include "libxstream.hpp"
#include "libxstream_alloc.hpp"
#include "libxstream_workitem.hpp"
#include "libxstream_context.hpp"
#include "libxstream_event.hpp"
#include "libxstream_offload.hpp"

#include <libxstream_begin.h>
#include <algorithm>
#include <cstring>
#include <cstdarg>
#include <limits>
#include <cstdio>
#if defined(LIBXSTREAM_STDFEATURES)
# include <thread>
# include <atomic>
# define LIBXSTREAM_STDMUTEX
# if defined(LIBXSTREAM_STDMUTEX)
#   include <mutex>
# endif
#endif
#include <libxstream_end.h>

#if defined(_OPENMP)
# include <omp.h>
#endif

#if defined(__GNUC__)
# include <pthread.h>
#endif

#if defined(__MKL)
# include <mkl.h>
#endif

#if defined(LIBXSTREAM_OFFLOAD) && (0 != LIBXSTREAM_OFFLOAD)
# include <offload.h>
#endif

#if defined(_WIN32)
# include <windows.h>
#else
# include <unistd.h>
#endif

#if !defined(YieldProcessor)
# if defined(__MIC__)
#   define YieldProcessor() _mm_delay_32(100)
# else
#   define YieldProcessor() _mm_pause()
# endif
#endif

#define LIBXSTREAM_RDTSC __rdtsc

/** Pin allocated memory. */
#if defined(__INTEL_COMPILER) && (1600 <= __INTEL_COMPILER) && (20150501 <= __INTEL_COMPILER_BUILD_DATE)
//# define LIBXSTREAM_ALLOC_PINNED
#endif
/** Enables runtime-sleep. */
#define LIBXSTREAM_RUNTIME_SLEEP


namespace libxstream_internal {

LIBXSTREAM_TARGET(mic) static/*IPO*/ class LIBXSTREAM_TARGET(mic) context_type {
public:
#if defined(LIBXSTREAM_STDFEATURES)
  typedef std::atomic<size_t> size_type;
#else
  typedef size_t size_type;
#endif

public:
  context_type()
    : m_lock(libxstream_lock_create())
    , m_device(-2), m_verbosity(-2)
    , m_nthreads(0)
  {
    std::fill_n(m_locks, LIBXSTREAM_MAX_NLOCKS, static_cast<libxstream_lock*>(0));
  }

  ~context_type() {
    for (int i = 0; i < (LIBXSTREAM_MAX_NLOCKS); ++i) {
      libxstream_lock_destroy(m_locks[i]);
    }
    libxstream_lock_destroy(m_lock);
  }

public:
  libxstream_lock*volatile& locks(const volatile void* address) {
    const uintptr_t id = reinterpret_cast<uintptr_t>(address) / (LIBXSTREAM_MAX_SIMD);
    // non-pot: return m_locks[id%(LIBXSTREAM_MAX_NLOCKS)];
    return m_locks[LIBXSTREAM_MOD(id, LIBXSTREAM_MAX_NLOCKS)];
  }

  libxstream_lock* lock() {
    return m_lock;
  }

  size_type& nthreads() {
    return m_nthreads;
  }

  int global_device() const {
    return m_device;
  }
  
  void global_device(int device) {
    libxstream_lock_acquire(m_lock);
    if (-1 > m_device) {
      m_device = device;
    }
    libxstream_lock_release(m_lock);
  }

  // store the active device per host-thread
  int& device() {
    static LIBXSTREAM_TLS int value = -2;
    return value;
  }

  int global_verbosity() const {
    return m_verbosity;
  }
  
  int default_verbosity() {
#if defined(LIBXSTREAM_TRACE) && ((1 < ((2*LIBXSTREAM_TRACE+1)/2) && defined(LIBXSTREAM_DEBUG)) || 1 == ((2*LIBXSTREAM_TRACE+1)/2))
# if defined(__MIC__) // TODO: propagate host's default verbosity
    const int level = -1;
# else
    const char *const verbose_env = getenv("LIBXSTREAM_VERBOSE");
    const char *const verbosity_env = (verbose_env && *verbose_env) ? verbose_env : getenv("LIBXSTREAM_VERBOSITY");
    const int level = (verbosity_env && *verbosity_env) ? atoi(verbosity_env) : 0/*default*/;
# endif
#else
    const int level = 0;
#endif
    return level;
  }

  void global_verbosity(int level) {
    libxstream_lock_acquire(m_lock);
    if (-1 > m_verbosity) {
      m_verbosity = level;
    }
    libxstream_lock_release(m_lock);
  }

  // store the verbosity level per host-thread
  int& verbosity() {
    static LIBXSTREAM_TLS int value = -2;
    return value;
  }

private:
  libxstream_lock*volatile m_locks[LIBXSTREAM_MAX_NLOCKS];
  libxstream_lock* m_lock;
  int m_device, m_verbosity;
  size_type m_nthreads;
} context;


LIBXSTREAM_TARGET(mic) void mem_info(uint64_t& memory_physical, uint64_t& memory_not_allocated)
{
#if defined(_WIN32)
  MEMORYSTATUSEX status;
  status.dwLength = sizeof(status);
  const BOOL ok = GlobalMemoryStatusEx(&status);
  if (TRUE == ok) {
    memory_not_allocated = status.ullAvailPhys;
    memory_physical = status.ullTotalPhys;
  }
  else {
    memory_not_allocated = 0;
    memory_physical = 0;
  }
#else
  const long memory_pages_size = sysconf(_SC_PAGE_SIZE);
  const long memory_pages_phys = sysconf(_SC_PHYS_PAGES);
  const long memory_pages_avail = sysconf(_SC_AVPHYS_PAGES);
  memory_not_allocated = memory_pages_size * memory_pages_avail;
  memory_physical = memory_pages_size * memory_pages_phys;
#endif
}


template<typename DST, typename SRC>
LIBXSTREAM_TARGET(mic) DST bitwise_cast(const SRC& src)
{
  LIBXSTREAM_ASSERT(sizeof(SRC) <= sizeof(DST));
  union {
    SRC src;
    DST dst;
  } result;
  memset(&result, 0, sizeof(result));
  result.src = src;
  return result.dst;
}

} // namespace libxstream_internal


LIBXSTREAM_EXPORT_C LIBXSTREAM_TARGET(mic) void libxstream_use_sink(const void*) {}
LIBXSTREAM_EXPORT_C LIBXSTREAM_TARGET(mic) int libxstream_not_constant(int value) { return value; }


LIBXSTREAM_TARGET(mic) libxstream_lock* libxstream_lock_create()
{
#if defined(LIBXSTREAM_STDFEATURES)
# if defined(LIBXSTREAM_STDMUTEX)
  std::mutex *const typed_lock = new std::mutex;
# else
  std::atomic<int> *const typed_lock = new std::atomic<int>(0);
# endif
#elif defined(_OPENMP)
  omp_lock_t typed_lock;
  omp_init_lock(&typed_lock);
#elif defined(__GNUC__)
  pthread_mutexattr_t attributes;
  pthread_mutexattr_init(&attributes);
  pthread_mutexattr_settype(&attributes, PTHREAD_MUTEX_NORMAL);
  pthread_mutex_t *const typed_lock = new pthread_mutex_t;
  pthread_mutex_init(typed_lock, &attributes);
#else // Windows
  const HANDLE typed_lock = CreateMutex(0/*default*/, FALSE/*unlocked*/, 0/*unnamed*/);
#endif
  return libxstream_internal::bitwise_cast<libxstream_lock*>(typed_lock);
}


LIBXSTREAM_TARGET(mic) libxstream_lock* libxstream_lock_get(const volatile void* address)
{
  libxstream_lock *volatile& result = libxstream_internal::context.locks(address);

  if (0 == result) {
    libxstream_lock *const lock = libxstream_internal::context.lock();
    libxstream_lock_acquire(lock);

    if (0 == result) {
      result = libxstream_lock_create();
    }

    libxstream_lock_release(lock);
  }

  LIBXSTREAM_ASSERT(0 != result);
  return result;
}


LIBXSTREAM_TARGET(mic) void libxstream_lock_destroy(libxstream_lock* lock)
{
#if defined(LIBXSTREAM_STDFEATURES)
# if defined(LIBXSTREAM_STDMUTEX)
  std::mutex *const typed_lock = static_cast<std::mutex*>(lock);
# else
  std::atomic<int> *const typed_lock = static_cast<std::atomic<int>*>(lock);
# endif
  delete typed_lock;
#elif defined(_OPENMP)
  if (0 != lock) {
    omp_lock_t typed_lock = libxstream_internal::bitwise_cast<omp_lock_t>(lock);
    omp_destroy_lock(&typed_lock);
  }
#elif defined(__GNUC__)
  if (0 != lock) {
    pthread_mutex_t *const typed_lock = static_cast<pthread_mutex_t*>(lock);
    pthread_mutex_destroy(typed_lock);
    delete typed_lock;
  }
#else // Windows
  const HANDLE typed_lock = static_cast<HANDLE>(lock);
  CloseHandle(typed_lock);
#endif
}


LIBXSTREAM_TARGET(mic) void libxstream_lock_acquire(libxstream_lock* lock)
{
  LIBXSTREAM_ASSERT(lock);
#if defined(LIBXSTREAM_STDFEATURES)
# if defined(LIBXSTREAM_STDMUTEX)
  std::mutex *const typed_lock = static_cast<std::mutex*>(lock);
  typed_lock->lock();
# else
  std::atomic<int>& typed_lock = *static_cast<std::atomic<int>*>(lock);
  if (1 < ++typed_lock) {
    while (1 < typed_lock) {
      this_thread_sleep();
    }
  }
# endif
#elif defined(_OPENMP)
  omp_lock_t typed_lock = libxstream_internal::bitwise_cast<omp_lock_t>(lock);
  omp_set_lock(&typed_lock);
#elif defined(__GNUC__)
  pthread_mutex_t *const typed_lock = static_cast<pthread_mutex_t*>(lock);
  pthread_mutex_lock(typed_lock);
#else // Windows
  const HANDLE typed_lock = static_cast<HANDLE>(lock);
  WaitForSingleObject(typed_lock, INFINITE);
#endif
}


LIBXSTREAM_TARGET(mic) void libxstream_lock_release(libxstream_lock* lock)
{
  LIBXSTREAM_ASSERT(lock);
#if defined(LIBXSTREAM_STDFEATURES)
# if defined(LIBXSTREAM_STDMUTEX)
  std::mutex *const typed_lock = static_cast<std::mutex*>(lock);
  typed_lock->unlock();
# else
  std::atomic<int>& typed_lock = *static_cast<std::atomic<int>*>(lock);
  --typed_lock;
# endif
#elif defined(_OPENMP)
  omp_lock_t typed_lock = libxstream_internal::bitwise_cast<omp_lock_t>(lock);
  omp_unset_lock(&typed_lock);
#elif defined(__GNUC__)
  pthread_mutex_t *const typed_lock = static_cast<pthread_mutex_t*>(lock);
  pthread_mutex_unlock(typed_lock);
#else // Windows
  const HANDLE typed_lock = static_cast<HANDLE>(lock);
  ReleaseMutex(typed_lock);
#endif
}


LIBXSTREAM_TARGET(mic) bool libxstream_lock_try(libxstream_lock* lock)
{
  LIBXSTREAM_ASSERT(lock);
#if defined(LIBXSTREAM_STDFEATURES)
# if defined(LIBXSTREAM_STDMUTEX)
  std::mutex *const typed_lock = static_cast<std::mutex*>(lock);
  const bool result = typed_lock->try_lock();
# else
  std::atomic<int>& typed_lock = *static_cast<std::atomic<int>*>(lock);
  const bool result = 1 == ++typed_lock;
  if (!result) --typed_lock;
# endif
#elif defined(_OPENMP)
  omp_lock_t typed_lock = libxstream_internal::bitwise_cast<omp_lock_t>(lock);
  const bool result = 0 != omp_test_lock(&typed_lock);
#elif defined(__GNUC__)
  pthread_mutex_t *const typed_lock = static_cast<pthread_mutex_t*>(lock);
  const bool result =  0 == pthread_mutex_trylock(typed_lock);
#else // Windows
  const HANDLE typed_lock = static_cast<HANDLE>(lock);
  const bool result = WAIT_OBJECT_0 == WaitForSingleObject(typed_lock, INFINITE);
#endif
  return result;
}


LIBXSTREAM_TARGET(mic) size_t nthreads_active()
{
  const size_t result = libxstream_internal::context.nthreads();
  LIBXSTREAM_ASSERT(result <= LIBXSTREAM_MAX_NTHREADS);
  return result;
}


LIBXSTREAM_TARGET(mic) int this_thread_id()
{
  static LIBXSTREAM_TLS int id = -1;
  if (0 > id) {
    libxstream_internal::context_type::size_type& nthreads_active = libxstream_internal::context.nthreads();
#if defined(LIBXSTREAM_STDFEATURES)
    id = static_cast<int>(nthreads_active++);
#elif defined(_OPENMP)
    size_t nthreads = 0;
# if (201107 <= _OPENMP)
#   pragma omp atomic capture
# else
#   pragma omp critical
# endif
    nthreads = ++nthreads_active;
    id = static_cast<int>(nthreads - 1);
#else // generic
    libxstream_lock *const lock = libxstream_internal::context.lock();
    libxstream_lock_acquire(lock);
    id = static_cast<int>(nthreads_active++);
    libxstream_lock_release(lock);
#endif
  }
  LIBXSTREAM_ASSERT(id < LIBXSTREAM_MAX_NTHREADS);
  return id;
}


LIBXSTREAM_TARGET(mic) void this_thread_yield()
{
#if defined(LIBXSTREAM_RUNTIME_SLEEP)
# if defined(LIBXSTREAM_STDFEATURES) && defined(LIBXSTREAM_STDFEATURES_THREADX)
  std::this_thread::yield();
# elif defined(__GNUC__) && !defined(__MIC__)
  pthread_yield();
# endif
#else
  YieldProcessor();
#endif
}


LIBXSTREAM_TARGET(mic) void this_thread_sleep(size_t ms)
{
#if defined(LIBXSTREAM_RUNTIME_SLEEP)
  if (0 < ms) {
# if defined(LIBXSTREAM_STDFEATURES) && defined(LIBXSTREAM_STDFEATURES_THREADX)
    typedef std::chrono::milliseconds milliseconds;
    LIBXSTREAM_ASSERT(ms <= static_cast<size_t>(std::numeric_limits<milliseconds::rep>::max() / 1000));
    const milliseconds interval(static_cast<milliseconds::rep>(ms));
    std::this_thread::sleep_for(interval);
# elif defined(_WIN32)
    if (1 < ms) {
      LIBXSTREAM_ASSERT(ms <= std::numeric_limits<DWORD>::max());
      Sleep(static_cast<DWORD>(ms));
    }
    else {
      SwitchToThread();
    }
# else
    const size_t s = ms / 1000;
    ms -= 1000 * s;
    LIBXSTREAM_ASSERT(ms <= static_cast<size_t>(std::numeric_limits<long>::max() / (1000 * 1000)));
    const timespec pause = {
      static_cast<time_t>(s),
      static_cast<long>(ms * 1000 * 1000)
    };
    nanosleep(&pause, 0);
# endif
  }
  else {
    this_thread_yield();
  }
#else
  libxstream_use_sink(&ms);
  YieldProcessor();
#endif
}


LIBXSTREAM_TARGET(mic) void this_thread_wait(size_t& cycle)
{
#if 0 != (LIBXSTREAM_SPIN_CYCLES)
# if 0 < (LIBXSTREAM_SPIN_CYCLES)
  if ((LIBXSTREAM_SPIN_CYCLES) >= cycle)
# else // negative
  LIBXSTREAM_ASSERT(false/*TODO: not implemented!*/);
  const size_t tick = static_cast<size_t>(LIBXSTREAM_RDTSC());
  if (cycle <= tick + (LIBXSTREAM_SPIN_CYCLES))
# endif
  {
# if defined(__INTEL_COMPILER)
#   pragma forceinline recursive
# endif
    this_thread_yield();
# if 0 < (LIBXSTREAM_SPIN_CYCLES)
    ++cycle;
# else
    cycle = 0 != cycle ? cycle : tick;
# endif
  }
  else
#else
  libxstream_use_sink(&cycle);
#endif
  {
#if defined(__INTEL_COMPILER)
#   pragma forceinline recursive
#endif
    this_thread_sleep();
    // force active state after sleep
    //cycle = 0;
  }
}


LIBXSTREAM_EXPORT_C int libxstream_get_ndevices(size_t* ndevices)
{
  LIBXSTREAM_CHECK_CONDITION(ndevices);

#if defined(LIBXSTREAM_OFFLOAD) && (0 != LIBXSTREAM_OFFLOAD) && !defined(__MIC__)
  static const int idevices = std::min(_Offload_number_of_devices(), LIBXSTREAM_MAX_NDEVICES);
  LIBXSTREAM_CHECK_CONDITION(0 <= idevices);
  *ndevices = static_cast<size_t>(idevices);
#else
  *ndevices = 0;
#endif

#if defined(LIBXSTREAM_TRACE) && ((1 < ((2*LIBXSTREAM_TRACE+1)/2) && defined(LIBXSTREAM_DEBUG)) || 1 == ((2*LIBXSTREAM_TRACE+1)/2))
  static LIBXSTREAM_TLS bool print = true;
  if (print) { // once
    LIBXSTREAM_PRINT(2, "get_ndevices: ndevices=%lu", static_cast<unsigned long>(*ndevices));
    print = false;
  }
#endif

  return LIBXSTREAM_ERROR_NONE;
}


LIBXSTREAM_EXPORT_C int libxstream_get_active_device(int* device)
{
  const int result = libxstream_stream_device(0, device);
  LIBXSTREAM_ASSERT(LIBXSTREAM_ERROR_NONE == result);
  return result;
}


LIBXSTREAM_EXPORT_C int libxstream_set_active_device(int device)
{
  const int result = libxstream_stream_create(0, device, 0, 0);
  LIBXSTREAM_ASSERT(LIBXSTREAM_ERROR_NONE == result);
  return result;
}


LIBXSTREAM_EXPORT_C int libxstream_mem_info(int device, size_t* allocatable, size_t* physical)
{
  LIBXSTREAM_CHECK_CONDITION(allocatable || physical);
  uint64_t memory_physical = 0, memory_allocatable = 0;

  LIBXSTREAM_ASYNC_BEGIN
  {
    uint64_t& memory_physical = *ptr<uint64_t,1>();
    uint64_t& memory_allocatable = *ptr<uint64_t,2>();

    LIBXSTREAM_PRINT(2, "mem_info: device=%i allocatable=%lu physical=%lu", LIBXSTREAM_ASYNC_DEVICE,
      static_cast<unsigned long>(memory_allocatable), static_cast<unsigned long>(memory_physical));

#if defined(LIBXSTREAM_OFFLOAD)
    if (0 <= LIBXSTREAM_ASYNC_DEVICE) {
#     pragma offload target(mic:LIBXSTREAM_ASYNC_DEVICE) //out(memory_physical, memory_allocatable)
      libxstream_internal::mem_info(memory_physical, memory_allocatable);
    }
    else
#endif
    {
      libxstream_internal::mem_info(memory_physical, memory_allocatable);
    }
  }
  LIBXSTREAM_ASYNC_END(0, LIBXSTREAM_CALL_DEFAULT | LIBXSTREAM_CALL_DEVICE | LIBXSTREAM_CALL_WAIT, work, device, &memory_physical, &memory_allocatable);

  const int result = work.wait();
  LIBXSTREAM_CHECK_CONDITION(0 < memory_physical && 0 < memory_allocatable);
  if (allocatable) {
    *allocatable = static_cast<size_t>(memory_allocatable);
  }
  if (physical) {
    *physical = static_cast<size_t>(memory_physical);
  }

  LIBXSTREAM_ASSERT(LIBXSTREAM_ERROR_NONE == result);
  return result;
}


LIBXSTREAM_EXPORT_C int libxstream_mem_allocate(int device, void** memory, size_t size, size_t alignment)
{
  LIBXSTREAM_CHECK_CONDITION(0 != memory);
  int result = LIBXSTREAM_ERROR_NONE;

#if defined(LIBXSTREAM_OFFLOAD)
  if (0 <= device) {
    result = libxstream_virt_allocate(memory, size, alignment, &device, sizeof(device));

    if (LIBXSTREAM_ERROR_NONE == result) {
      LIBXSTREAM_ASYNC_BEGIN
      {
        const char* buffer = ptr<const char,1>();
        const size_t size = val<const size_t,2>();

        LIBXSTREAM_PRINT(2, "mem_allocate: device=%i buffer=0x%llx size=%lu", LIBXSTREAM_ASYNC_DEVICE,
          reinterpret_cast<unsigned long long>(buffer), static_cast<unsigned long>(size));

#       pragma offload_transfer target(mic:LIBXSTREAM_ASYNC_DEVICE) nocopy(buffer: length(size) LIBXSTREAM_OFFLOAD_ALLOC)
      }
      LIBXSTREAM_ASYNC_END(0, LIBXSTREAM_CALL_DEFAULT | LIBXSTREAM_CALL_DEVICE, work, device, *memory, size);
      result = work.status();
    }
  }
  else {
#else
  {
    libxstream_use_sink(&device);
#endif
    LIBXSTREAM_PRINT(2, "mem_allocate: device=%i buffer=0x%llx size=%lu", device,
      reinterpret_cast<unsigned long long>(*memory), static_cast<unsigned long>(size));

    result = libxstream_real_allocate(memory, size, alignment);

    if (LIBXSTREAM_ERROR_NONE == result) {
#if defined(LIBXSTREAM_OFFLOAD) && defined(LIBXSTREAM_ALLOC_PINNED)
      LIBXSTREAM_ASYNC_BEGIN
      {
        const char* buffer = ptr<const char,1>();
        const size_t size = val<const size_t,2>();
#       pragma offload_transfer target(mic) host_pin(buffer: length(size))
      }
      LIBXSTREAM_ASYNC_END(0, LIBXSTREAM_CALL_DEFAULT | LIBXSTREAM_CALL_DEVICE, work, device, *memory, size);
      result = work.status();
#endif
    }
  }

  LIBXSTREAM_ASSERT(LIBXSTREAM_ERROR_NONE == result);
  return result;
}


LIBXSTREAM_EXPORT_C int libxstream_mem_deallocate(int device, const void* memory)
{
  int result = LIBXSTREAM_ERROR_NONE;

  if (memory) {
    // synchronize across all devices not just the given device
    libxstream_stream::wait_all(true);

#if defined(LIBXSTREAM_OFFLOAD)
    if (0 <= device) {
# if defined(LIBXSTREAM_CHECK)
      const int memory_device = *static_cast<const int*>(libxstream_virt_data(memory));
      if (device != memory_device) {
        LIBXSTREAM_PRINT(1, "mem_deallocate: device %i does not match allocating device %i!", device, memory_device);
        LIBXSTREAM_CHECK_CONDITION(0 <= memory_device);
        device = memory_device;
      }
# endif
      LIBXSTREAM_ASYNC_BEGIN
      {
        const char *const memory = ptr<const char,1>();
        LIBXSTREAM_PRINT(2, "mem_deallocate: device=%i buffer=0x%llx", LIBXSTREAM_ASYNC_DEVICE, reinterpret_cast<unsigned long long>(memory));
#       pragma offload_transfer target(mic:LIBXSTREAM_ASYNC_DEVICE) nocopy(memory: length(0) LIBXSTREAM_OFFLOAD_FREE)
        LIBXSTREAM_ASYNC_QENTRY.status() = libxstream_virt_deallocate(memory);
      }
      LIBXSTREAM_ASYNC_END(0, LIBXSTREAM_CALL_DEFAULT | LIBXSTREAM_CALL_DEVICE, work, device, memory);
      result = work.status();
    }
    else {
#else
    {
      libxstream_use_sink(&device);
#endif
#if defined(LIBXSTREAM_OFFLOAD) && defined(LIBXSTREAM_ALLOC_PINNED)
      LIBXSTREAM_ASYNC_BEGIN
      {
        const char* memory = ptr<const char,1>();
        LIBXSTREAM_PRINT(2, "mem_deallocate: device=%i buffer=0x%llx", LIBXSTREAM_ASYNC_DEVICE, reinterpret_cast<unsigned long long>(memory));
#       pragma offload_transfer target(mic) host_unpin(memory: length(0))
        LIBXSTREAM_ASYNC_QENTRY.status() = libxstream_real_deallocate(memory);
      }
      LIBXSTREAM_ASYNC_END(0, LIBXSTREAM_CALL_DEFAULT | LIBXSTREAM_CALL_DEVICE, work, device, memory);
      result = work.status();
#else
      LIBXSTREAM_PRINT(2, "mem_deallocate: device=%i buffer=0x%llx", device, reinterpret_cast<unsigned long long>(memory));
      result = libxstream_real_deallocate(memory);
#endif
    }
  }

  LIBXSTREAM_ASSERT(LIBXSTREAM_ERROR_NONE == result);
  return result;
}


LIBXSTREAM_EXPORT_C int libxstream_memset_zero(void* memory, size_t size, libxstream_stream* stream)
{
  LIBXSTREAM_ASSERT(0 != memory);
  LIBXSTREAM_ASYNC_BEGIN
  {
    char* dst = ptr<char,0>();
    const size_t size = val<const size_t,1>();

    LIBXSTREAM_PRINT(2, "memset_zero: stream=0x%llx buffer=0x%llx size=%lu",
      reinterpret_cast<unsigned long long>(LIBXSTREAM_ASYNC_STREAM),
      reinterpret_cast<unsigned long long>(dst), static_cast<unsigned long>(size));

#if defined(LIBXSTREAM_OFFLOAD)
    if (0 <= LIBXSTREAM_ASYNC_DEVICE) {
      if (LIBXSTREAM_ASYNC_READY) {
#       pragma offload LIBXSTREAM_ASYNC_TARGET_SIGNAL in(size) out(dst: LIBXSTREAM_OFFLOAD_REFRESH)
        memset(dst, 0, size);
      }
      else {
#       pragma offload LIBXSTREAM_ASYNC_TARGET_SIGNAL_WAIT in(size) out(dst: LIBXSTREAM_OFFLOAD_REFRESH)
        memset(dst, 0, size);
      }
    }
    else
#endif
    {
      memset(dst, 0, size);
    }
  }
  LIBXSTREAM_ASYNC_END(stream, LIBXSTREAM_CALL_DEFAULT, work, memory, size);

  const int result = work.status();
  LIBXSTREAM_ASSERT(LIBXSTREAM_ERROR_NONE == result);
  return result;
}


LIBXSTREAM_EXPORT_C int libxstream_memcpy_h2d(const void* host_mem, void* dev_mem, size_t size, libxstream_stream* stream)
{
  LIBXSTREAM_CHECK_CONDITION(0 != host_mem && 0 != dev_mem && host_mem != dev_mem);
  LIBXSTREAM_ASYNC_BEGIN
  {
    const char *const src = ptr<const char,0>();
    char *const dst = ptr<char,1>();
    const size_t size = val<const size_t,2>();

    LIBXSTREAM_PRINT(2, "memcpy_h2d: stream=0x%llx 0x%llx->0x%llx size=%lu", reinterpret_cast<unsigned long long>(LIBXSTREAM_ASYNC_STREAM),
      reinterpret_cast<unsigned long long>(src), reinterpret_cast<unsigned long long>(dst), static_cast<unsigned long>(size));

#if defined(LIBXSTREAM_OFFLOAD)
    if (0 <= LIBXSTREAM_ASYNC_DEVICE) {
      if (LIBXSTREAM_ASYNC_READY) {
#       pragma offload_transfer LIBXSTREAM_ASYNC_TARGET_SIGNAL in(src: length(size) into(dst) LIBXSTREAM_OFFLOAD_REUSE)
      }
      else {
#       pragma offload_transfer LIBXSTREAM_ASYNC_TARGET_SIGNAL_WAIT in(src: length(size) into(dst) LIBXSTREAM_OFFLOAD_REUSE)
      }
    }
    else
#endif
    {
#if defined(LIBXSTREAM_ASYNCHOST) && (201307 <= _OPENMP) // V4.0
      if (LIBXSTREAM_ASYNC_READY) {
#       pragma omp task depend(out:capture_region_signal) depend(in:LIBXSTREAM_ASYNC_PENDING)
        std::copy(src, src + size, dst);
      }
      else {
#       pragma omp task depend(out:capture_region_signal)
        std::copy(src, src + size, dst);
        ++capture_region_signal_consumed;
      }
#else
      std::copy(src, src + size, dst);
#endif
    }
  }
  LIBXSTREAM_ASYNC_END(stream, LIBXSTREAM_CALL_DEFAULT, work, host_mem, dev_mem, size);

  const int result = work.status();
  LIBXSTREAM_ASSERT(LIBXSTREAM_ERROR_NONE == result);
  return result;
}


LIBXSTREAM_EXPORT_C int libxstream_memcpy_d2h(const void* dev_mem, void* host_mem, size_t size, libxstream_stream* stream)
{
  LIBXSTREAM_CHECK_CONDITION(0 != dev_mem && 0 != host_mem && dev_mem != host_mem);
  LIBXSTREAM_ASYNC_BEGIN
  {
    const char* src = ptr<const char,0>();
    char *const dst = ptr<char,1>();
    const size_t size = val<const size_t,2>();

    LIBXSTREAM_PRINT(2, "memcpy_d2h: stream=0x%llx 0x%llx->0x%llx size=%lu", reinterpret_cast<unsigned long long>(LIBXSTREAM_ASYNC_STREAM),
      reinterpret_cast<unsigned long long>(src), reinterpret_cast<unsigned long long>(dst), static_cast<unsigned long>(size));

#if defined(LIBXSTREAM_OFFLOAD)
    if (0 <= LIBXSTREAM_ASYNC_DEVICE) {
      if (LIBXSTREAM_ASYNC_READY) {
#       pragma offload_transfer LIBXSTREAM_ASYNC_TARGET_SIGNAL out(src: length(size) into(dst) LIBXSTREAM_OFFLOAD_REUSE)
      }
      else {
#       pragma offload_transfer LIBXSTREAM_ASYNC_TARGET_SIGNAL_WAIT out(src: length(size) into(dst) LIBXSTREAM_OFFLOAD_REUSE)
      }
    }
    else
#endif
    {
      std::copy(src, src + size, dst);
    }
  }
  LIBXSTREAM_ASYNC_END(stream, LIBXSTREAM_CALL_DEFAULT, work, dev_mem, host_mem, size);

  const int result = work.status();
  LIBXSTREAM_ASSERT(LIBXSTREAM_ERROR_NONE == result);
  return result;
}


LIBXSTREAM_EXPORT_C int libxstream_memcpy_d2d(const void* src, void* dst, size_t size, libxstream_stream* stream)
{
  LIBXSTREAM_CHECK_CONDITION(0 != src && 0 != dst);
  int result = LIBXSTREAM_ERROR_NONE;

  if (src != dst) {
    LIBXSTREAM_ASYNC_BEGIN
    {
      const char *const src = ptr<const char,0>();
      char* dst = ptr<char,1>();
      const size_t size = val<const size_t,2>();

      LIBXSTREAM_PRINT(2, "memcpy_d2d: stream=0x%llx 0x%llx->0x%llx size=%lu", reinterpret_cast<unsigned long long>(LIBXSTREAM_ASYNC_STREAM),
        reinterpret_cast<unsigned long long>(src), reinterpret_cast<unsigned long long>(dst), static_cast<unsigned long>(size));

#if defined(LIBXSTREAM_OFFLOAD)
      if (0 <= LIBXSTREAM_ASYNC_DEVICE) {
        // TODO: implement cross-device transfer

        if (LIBXSTREAM_ASYNC_READY) {
#         pragma offload LIBXSTREAM_ASYNC_TARGET_SIGNAL in(size) in(src: LIBXSTREAM_OFFLOAD_REFRESH) out(dst: LIBXSTREAM_OFFLOAD_REFRESH)
          memcpy(dst, src, size);
        }
        else {
#         pragma offload LIBXSTREAM_ASYNC_TARGET_SIGNAL_WAIT in(size) in(src: LIBXSTREAM_OFFLOAD_REFRESH) out(dst: LIBXSTREAM_OFFLOAD_REFRESH)
          memcpy(dst, src, size);
        }
      }
      else
#endif
      {
        memcpy(dst, src, size);
      }
    }
    LIBXSTREAM_ASYNC_END(stream, LIBXSTREAM_CALL_DEFAULT, work, src, dst, size);
    result = work.status();
  }

  LIBXSTREAM_ASSERT(LIBXSTREAM_ERROR_NONE == result);
  return result;
}


LIBXSTREAM_EXPORT_C int libxstream_stream_priority_range(int* least, int* greatest)
{
  LIBXSTREAM_CHECK_CONDITION(0 != least || 0 != greatest);
  const int priority_range_least = libxstream_stream::priority_range_least();
  const int priority_range_greatest = libxstream_stream::priority_range_greatest();
  LIBXSTREAM_PRINT(3, "libxstream_stream_priority_range: least=%i greatest=%i", priority_range_least, priority_range_greatest);

  if (least) {
    *least = priority_range_least;
  }
  if (greatest) {
    *greatest = priority_range_greatest;
  }

  return LIBXSTREAM_ERROR_NONE;
}


LIBXSTREAM_EXPORT_C int libxstream_stream_create(libxstream_stream** stream, int device, int priority, const char* name)
{
  if (stream) {
    libxstream_stream *const s = new libxstream_stream(device, priority, name);
    LIBXSTREAM_ASSERT(s);
    *stream = s;

#if defined(LIBXSTREAM_TRACE) && ((1 < ((2*LIBXSTREAM_TRACE+1)/2) && defined(LIBXSTREAM_DEBUG)) || 1 == ((2*LIBXSTREAM_TRACE+1)/2))
    if (name && *name) {
      LIBXSTREAM_PRINT(2, "stream_create: stream=0x%llx (%s) device=%i priority=%i",
        reinterpret_cast<unsigned long long>(*stream), name, device, priority);
    }
    else {
      LIBXSTREAM_PRINT(2, "stream_create: stream=0x%llx device=%i priority=%i",
        reinterpret_cast<unsigned long long>(*stream), device, priority);
    }
#endif
  }
  else {
    size_t ndevices = LIBXSTREAM_MAX_NDEVICES;
    LIBXSTREAM_CHECK_CONDITION(-1 <= device && ndevices >= static_cast<size_t>(device + 1) && LIBXSTREAM_ERROR_NONE == libxstream_get_ndevices(&ndevices) && ndevices >= static_cast<size_t>(device + 1));

    if (-1 > libxstream_internal::context.global_device()) {
      libxstream_internal::context.global_device(device);
    }

    libxstream_internal::context.device() = device;
    LIBXSTREAM_PRINT(2, "set_active_device: device=%i thread=%i", device, this_thread_id());
  }

  return LIBXSTREAM_ERROR_NONE;
}


LIBXSTREAM_EXPORT_C int libxstream_stream_destroy(const libxstream_stream* stream)
{
#if defined(LIBXSTREAM_TRACE) && ((1 < ((2*LIBXSTREAM_TRACE+1)/2) && defined(LIBXSTREAM_DEBUG)) || 1 == ((2*LIBXSTREAM_TRACE+1)/2))
  if (stream) {
# if defined(LIBXSTREAM_DEBUG)
    const char *const name = stream->name();
    if (name && *name) {
      LIBXSTREAM_PRINT(2, "stream_destroy: stream=0x%llx (%s)", reinterpret_cast<unsigned long long>(stream), name);
    }
    else
# endif
    {
      LIBXSTREAM_PRINT(2, "stream_destroy: stream=0x%llx", reinterpret_cast<unsigned long long>(stream));
    }
  }
#endif
  delete stream;
  return LIBXSTREAM_ERROR_NONE;
}


LIBXSTREAM_EXPORT_C int libxstream_stream_wait(libxstream_stream* stream)
{
  // TODO: print in order
#if defined(LIBXSTREAM_TRACE) && ((1 < ((2*LIBXSTREAM_TRACE+1)/2) && defined(LIBXSTREAM_DEBUG)) || 1 == ((2*LIBXSTREAM_TRACE+1)/2)) && 0
  if (0 != stream) {
# if defined(LIBXSTREAM_DEBUG)
    const char *const name = stream->name();
    if (name && *name) {
      LIBXSTREAM_PRINT(2, "stream_wait: stream=0x%llx (%s)", reinterpret_cast<unsigned long long>(stream), name);
    }
    else
# endif
    {
      LIBXSTREAM_PRINT(2, "stream_wait: stream=0x%llx", reinterpret_cast<unsigned long long>(stream));
    }
  }
  else {
    LIBXSTREAM_PRINT0(2, "stream_wait: wait for all streams");
  }
#endif
  const int result = stream ? stream->wait(true) : libxstream_stream::wait_all(true);
  LIBXSTREAM_ASSERT(LIBXSTREAM_ERROR_NONE == result);
  return result;
}


LIBXSTREAM_EXPORT_C int libxstream_stream_wait_event(libxstream_stream* stream, const libxstream_event* event)
{
  // TODO: print in order
  //LIBXSTREAM_PRINT(2, "stream_wait_event: stream=0x%llx event=0x%llx", reinterpret_cast<unsigned long long>(stream), reinterpret_cast<unsigned long long>(event));
  LIBXSTREAM_CHECK_CONDITION(0 != event);
  const int result = libxstream_event(*event).wait_stream(stream);
  LIBXSTREAM_ASSERT(LIBXSTREAM_ERROR_NONE == result);
  return result;
}


LIBXSTREAM_EXPORT_C int libxstream_stream_device(const libxstream_stream* stream, int* device)
{
  LIBXSTREAM_CHECK_CONDITION(0 != device);
  int result = LIBXSTREAM_ERROR_NONE;

  if (stream) {
    *device = stream->device();
    LIBXSTREAM_PRINT(3, "stream_device: stream=0x%llx device=%i", reinterpret_cast<unsigned long long>(stream), *device);
  }
  else {
    int active_device = libxstream_internal::context.device();

    if (-1 > active_device) {
      active_device = libxstream_internal::context.global_device();

      if (-1 > active_device) {
        size_t ndevices = 0;
        result = libxstream_get_ndevices(&ndevices);
        active_device = static_cast<int>(ndevices - 1);
        libxstream_internal::context.global_device(active_device);
        libxstream_internal::context.device() = active_device;
      }

#if defined(LIBXSTREAM_TRACE) && ((1 < ((2*LIBXSTREAM_TRACE+1)/2) && defined(LIBXSTREAM_DEBUG)) || 1 == ((2*LIBXSTREAM_TRACE+1)/2))
      static LIBXSTREAM_TLS bool print = true;
      if (print) { // once
        LIBXSTREAM_PRINT(2, "get_active_device: device=%i (fallback) thread=%i", active_device, this_thread_id());
        print = false;
      }
#endif
    }

    *device = active_device;
  }

  LIBXSTREAM_ASSERT(LIBXSTREAM_ERROR_NONE == result);
  return result;
}


LIBXSTREAM_EXPORT_C int libxstream_event_create(libxstream_event** event)
{
  LIBXSTREAM_CHECK_CONDITION(event);
  *event = new libxstream_event;
  LIBXSTREAM_PRINT(2, "event_create: event=0x%llx", reinterpret_cast<unsigned long long>(*event));
  return LIBXSTREAM_ERROR_NONE;
}


LIBXSTREAM_EXPORT_C int libxstream_event_destroy(const libxstream_event* event)
{
  LIBXSTREAM_PRINT(0 != event ? 2 : 0, "event_destroy: event=0x%llx", reinterpret_cast<unsigned long long>(event));
  delete event;
  return LIBXSTREAM_ERROR_NONE;
}


LIBXSTREAM_EXPORT_C int libxstream_event_record(libxstream_event* event, libxstream_stream* stream)
{
  LIBXSTREAM_PRINT(2, "event_record: event=0x%llx stream=0x%llx", reinterpret_cast<unsigned long long>(event), reinterpret_cast<unsigned long long>(stream));
  const int result = stream ? event->record(*stream, true) : libxstream_stream::enqueue(*event);
  LIBXSTREAM_ASSERT(LIBXSTREAM_ERROR_NONE == result);
  return result;
}


LIBXSTREAM_EXPORT_C int libxstream_event_query(const libxstream_event* event, libxstream_bool* occurred)
{
  LIBXSTREAM_PRINT(2, "event_query: event=0x%llx", reinterpret_cast<unsigned long long>(event));
  LIBXSTREAM_CHECK_CONDITION(event && occurred);

  bool has_occurred = true;
  const int result = event->query(has_occurred, 0);
  *occurred = has_occurred ? LIBXSTREAM_TRUE : LIBXSTREAM_FALSE;

  LIBXSTREAM_ASSERT(LIBXSTREAM_ERROR_NONE == result);
  return result;
}


LIBXSTREAM_EXPORT_C int libxstream_event_wait(libxstream_event* event)
{
  LIBXSTREAM_PRINT(2, "event_wait: event=0x%llx", reinterpret_cast<unsigned long long>(event));
  LIBXSTREAM_CHECK_CONDITION(event);
  const int result = event->wait(0, true);
  LIBXSTREAM_ASSERT(LIBXSTREAM_ERROR_NONE == result);
  return result;
}


LIBXSTREAM_EXPORT_C int libxstream_fn_signature(libxstream_argument** signature)
{
  static LIBXSTREAM_TLS libxstream_argument arguments[(LIBXSTREAM_MAX_NARGS)+1];
  LIBXSTREAM_CHECK_CALL(libxstream_construct(arguments, LIBXSTREAM_MAX_NARGS));
  *signature = arguments;
  return LIBXSTREAM_ERROR_NONE;
}


LIBXSTREAM_EXPORT_C int libxstream_fn_clear_signature(libxstream_argument* signature)
{
  size_t nargs = 0;
  if (signature) {
    LIBXSTREAM_CHECK_CALL(libxstream_fn_nargs(signature, &nargs));
    LIBXSTREAM_CHECK_CALL(libxstream_construct(signature, nargs));
  }
  LIBXSTREAM_PRINT(2, "fn_clear_signature: signature=0x%llx nargs=%lu", reinterpret_cast<unsigned long long>(signature), static_cast<unsigned long>(nargs));
  return LIBXSTREAM_ERROR_NONE;
}


LIBXSTREAM_EXPORT_C int libxstream_fn_input(libxstream_argument* signature, size_t arg, const void* in, libxstream_type type, size_t dims, const size_t shape[])
{
  LIBXSTREAM_CHECK_CONDITION(0 != signature);
#if defined(LIBXSTREAM_DEBUG)
  size_t nargs = 0;
  LIBXSTREAM_ASSERT(LIBXSTREAM_ERROR_NONE == libxstream_fn_nargs(signature, &nargs) && arg < nargs);
#endif
  const int result = libxstream_construct(signature, arg, libxstream_argument::kind_input, in, type, dims, shape);
  LIBXSTREAM_ASSERT(LIBXSTREAM_ERROR_NONE == result);
  return result;
}


LIBXSTREAM_EXPORT_C int libxstream_fn_output(libxstream_argument* signature, size_t arg, void* out, libxstream_type type, size_t dims, const size_t shape[])
{
  LIBXSTREAM_CHECK_CONDITION(0 != signature);
#if defined(LIBXSTREAM_DEBUG)
  size_t nargs = 0;
  LIBXSTREAM_ASSERT(LIBXSTREAM_ERROR_NONE == libxstream_fn_nargs(signature, &nargs) && arg < nargs);
#endif
  const int result = libxstream_construct(signature, arg, libxstream_argument::kind_output, out, type, dims, shape);
  LIBXSTREAM_ASSERT(LIBXSTREAM_ERROR_NONE == result);
  return result;
}


LIBXSTREAM_EXPORT_C int libxstream_fn_inout(libxstream_argument* signature, size_t arg, void* inout, libxstream_type type, size_t dims, const size_t shape[])
{
  LIBXSTREAM_CHECK_CONDITION(0 != signature);
#if defined(LIBXSTREAM_DEBUG)
  size_t nargs = 0;
  LIBXSTREAM_ASSERT(LIBXSTREAM_ERROR_NONE == libxstream_fn_nargs(signature, &nargs) && arg < nargs);
#endif
  const int result = libxstream_construct(signature, arg, libxstream_argument::kind_inout, inout, type, dims, shape);
  LIBXSTREAM_ASSERT(LIBXSTREAM_ERROR_NONE == result);
  return result;
}


LIBXSTREAM_EXPORT_C int libxstream_fn_nargs(const libxstream_argument* signature, size_t* nargs)
{
  LIBXSTREAM_CHECK_CONDITION(0 != nargs);
  size_t n = 0;
  if (signature) {
    while (libxstream_argument::kind_invalid != signature[n].kind) {
      LIBXSTREAM_ASSERT(n < (LIBXSTREAM_MAX_NARGS));
      ++n;
    }
  }
  *nargs = n;
  return LIBXSTREAM_ERROR_NONE;
}


LIBXSTREAM_EXPORT_C int libxstream_fn_call(libxstream_function function, const libxstream_argument* signature, libxstream_stream* stream, int flags)
{
  LIBXSTREAM_CHECK_CONDITION(0 != function);
  const libxstream_workqueue::entry_type& work = libxstream_offload(function, signature, stream, flags);
  const int result = 0 == (LIBXSTREAM_CALL_WAIT & flags) ? work.status() : work.wait();
  LIBXSTREAM_ASSERT(LIBXSTREAM_ERROR_NONE == result);
  return result;
}


LIBXSTREAM_EXPORT_C LIBXSTREAM_TARGET(mic) int libxstream_get_typesize(libxstream_type type, size_t* typesize)
{
  LIBXSTREAM_CHECK_CONDITION(0 != typesize);
  int result = LIBXSTREAM_ERROR_NONE;

  switch(type) {
    case LIBXSTREAM_TYPE_I8:    *typesize = 1;  break;
    case LIBXSTREAM_TYPE_I16:   *typesize = 2;  break;
    case LIBXSTREAM_TYPE_I32:   *typesize = 4;  break;
    case LIBXSTREAM_TYPE_I64:   *typesize = 8;  break;
    case LIBXSTREAM_TYPE_F32:   *typesize = 4;  break;
    case LIBXSTREAM_TYPE_F64:   *typesize = 8;  break;
    case LIBXSTREAM_TYPE_C32:   *typesize = 8;  break;
    case LIBXSTREAM_TYPE_C64:   *typesize = 16; break;
    case LIBXSTREAM_TYPE_U8:    *typesize = 1;  break;
    case LIBXSTREAM_TYPE_U16:   *typesize = 2;  break;
    case LIBXSTREAM_TYPE_U32:   *typesize = 4;  break;
    case LIBXSTREAM_TYPE_U64:   *typesize = 8;  break;
    case LIBXSTREAM_TYPE_CHAR:  *typesize = 1;  break;
    default: // LIBXSTREAM_TYPE_VOID, etc.
      result = LIBXSTREAM_ERROR_CONDITION;
  }

  //LIBXSTREAM_ASSERT(LIBXSTREAM_ERROR_NONE == result);
  return result;
}


LIBXSTREAM_EXPORT_C LIBXSTREAM_TARGET(mic) int libxstream_get_autotype(size_t typesize, libxstream_type start, libxstream_type* autotype)
{
  LIBXSTREAM_CHECK_CONDITION(0 != autotype);
  size_t size = 0;

  for (int i = static_cast<libxstream_type>(start); i <= LIBXSTREAM_TYPE_VOID; ++i) {
    *autotype = static_cast<libxstream_type>(i);
    if (LIBXSTREAM_ERROR_NONE != libxstream_get_typesize(*autotype, &size) || typesize == size) {
      i = LIBXSTREAM_TYPE_INVALID; // break
    }
  }

  return LIBXSTREAM_ERROR_NONE;
}


LIBXSTREAM_EXPORT_C LIBXSTREAM_TARGET(mic) int libxstream_get_typename(libxstream_type type, const char** name)
{
  LIBXSTREAM_CHECK_CONDITION(0 != name);
  int result = LIBXSTREAM_ERROR_NONE;

  switch(type) {
    case LIBXSTREAM_TYPE_I8:    *name = "i8";   break;
    case LIBXSTREAM_TYPE_U8:    *name = "u8";   break;
    case LIBXSTREAM_TYPE_I16:   *name = "i16";  break;
    case LIBXSTREAM_TYPE_U16:   *name = "u16";  break;
    case LIBXSTREAM_TYPE_I32:   *name = "i32";  break;
    case LIBXSTREAM_TYPE_U32:   *name = "u32";  break;
    case LIBXSTREAM_TYPE_I64:   *name = "i64";  break;
    case LIBXSTREAM_TYPE_U64:   *name = "u64";  break;
    case LIBXSTREAM_TYPE_F32:   *name = "f32";  break;
    case LIBXSTREAM_TYPE_F64:   *name = "f64";  break;
    case LIBXSTREAM_TYPE_C32:   *name = "c32";  break;
    case LIBXSTREAM_TYPE_C64:   *name = "c64";  break;
    case LIBXSTREAM_TYPE_CHAR:  *name = "char"; break;
    case LIBXSTREAM_TYPE_VOID:  *name = "void"; break;
    default:
      result = LIBXSTREAM_ERROR_CONDITION;
  }

  LIBXSTREAM_ASSERT(LIBXSTREAM_ERROR_NONE == result);
  return result;
}


LIBXSTREAM_EXPORT_C LIBXSTREAM_TARGET(mic) int libxstream_get_argument(const void* variable, size_t* arg)
{
  const libxstream_context& context = libxstream_context::instance();
  LIBXSTREAM_CHECK_CONDITION(0 != arg && 0 == (LIBXSTREAM_CALL_EXTERNAL & context.flags));
  const libxstream_argument *const hit = libxstream_find(context, variable);
  int result = LIBXSTREAM_ERROR_NONE;

  if (0 != hit) {
    LIBXSTREAM_ASSERT((LIBXSTREAM_MAX_NARGS) > (hit - context.signature));
    *arg = hit - context.signature;
  }
  else {
    result = LIBXSTREAM_ERROR_CONDITION;
  }

  LIBXSTREAM_ASSERT(LIBXSTREAM_ERROR_NONE == result);
  return result;
}


LIBXSTREAM_EXPORT_C LIBXSTREAM_TARGET(mic) int libxstream_get_arity(const libxstream_argument* signature, size_t* arity)
{
  LIBXSTREAM_CHECK_CONDITION(0 != arity);
  if (0 == signature) {
    const libxstream_context& context = libxstream_context::instance();
    if (0 == (LIBXSTREAM_CALL_EXTERNAL & context.flags)) {
      signature = context.signature;
    }
  }

  size_t n = 0;
  if (signature) {
    while (LIBXSTREAM_TYPE_INVALID != signature[n].type) {
      LIBXSTREAM_ASSERT(n < (LIBXSTREAM_MAX_NARGS));
      LIBXSTREAM_ASSERT(libxstream_argument::kind_invalid != signature[n].kind);
      ++n;
    }
  }
  *arity = n;

  return LIBXSTREAM_ERROR_NONE;
}


LIBXSTREAM_EXPORT_C LIBXSTREAM_TARGET(mic) int libxstream_get_data(const libxstream_argument* signature, size_t arg, const void** data)
{
  LIBXSTREAM_CHECK_CONDITION(0 != data);
#if defined(LIBXSTREAM_DEBUG) && (!defined(LIBXSTREAM_OFFLOAD) || (0 == LIBXSTREAM_OFFLOAD))
  size_t arity = 0;
  LIBXSTREAM_ASSERT(LIBXSTREAM_ERROR_NONE == libxstream_get_arity(signature, &arity) && arg < arity);
#endif
  if (0 == signature) {
    const libxstream_context& context = libxstream_context::instance();
    if (0 == (LIBXSTREAM_CALL_EXTERNAL & context.flags)) {
      signature = context.signature;
    }
  }
  LIBXSTREAM_ASSERT(0 != signature);
  *data = libxstream_get_value(signature[arg]).const_pointer;
  return LIBXSTREAM_ERROR_NONE;
}


LIBXSTREAM_EXPORT_C LIBXSTREAM_TARGET(mic) int libxstream_get_string(const libxstream_argument* signature, size_t arg, const char** value)
{
  LIBXSTREAM_CHECK_CONDITION(0 != value);
#if defined(LIBXSTREAM_DEBUG) && (!defined(LIBXSTREAM_OFFLOAD) || (0 == LIBXSTREAM_OFFLOAD))
  size_t arity = 0;
  LIBXSTREAM_ASSERT(LIBXSTREAM_ERROR_NONE == libxstream_get_arity(signature, &arity) && arg < arity);
#endif
  if (0 == signature) {
    const libxstream_context& context = libxstream_context::instance();
    if (0 == (LIBXSTREAM_CALL_EXTERNAL & context.flags)) {
      signature = context.signature;
    }
  }
  LIBXSTREAM_ASSERT(0 != signature);
  const libxstream_argument& argument = signature[arg];
  const void *const data = libxstream_get_value(argument).const_pointer;
  static LIBXSTREAM_TLS char buffer[128];
  int result = LIBXSTREAM_ERROR_NONE;

  if (0 < argument.dims || 0 == data) {
    if (LIBXSTREAM_TYPE_INVALID > argument.type) {
      LIBXSTREAM_SNPRINTF(buffer, sizeof(buffer), "0x%llx", reinterpret_cast<unsigned long long>(data));
      *value = buffer;
    }
    else {
      result = LIBXSTREAM_ERROR_CONDITION;
    }
  }
  else { // 0 == argument.dims && 0 != data
    libxstream_type type = argument.type;
    if (LIBXSTREAM_TYPE_VOID == type) {
      LIBXSTREAM_CHECK_CALL(libxstream_get_autotype(*argument.shape, LIBXSTREAM_TYPE_CHAR, &type));
    }

    switch(type) {
      case LIBXSTREAM_TYPE_CHAR:  LIBXSTREAM_SNPRINTF(buffer, sizeof(buffer), "%c", *static_cast<const char*>(data)); break;
      case LIBXSTREAM_TYPE_I8:    LIBXSTREAM_SNPRINTF(buffer, sizeof(buffer), "%i", *static_cast<const int8_t*>(data)); break;
      case LIBXSTREAM_TYPE_U8:    LIBXSTREAM_SNPRINTF(buffer, sizeof(buffer), "%u", *static_cast<const uint8_t*>(data)); break;
      case LIBXSTREAM_TYPE_I16:   LIBXSTREAM_SNPRINTF(buffer, sizeof(buffer), "%i", *static_cast<const int16_t*>(data)); break;
      case LIBXSTREAM_TYPE_U16:   LIBXSTREAM_SNPRINTF(buffer, sizeof(buffer), "%u", *static_cast<const uint16_t*>(data)); break;
      case LIBXSTREAM_TYPE_I32:   LIBXSTREAM_SNPRINTF(buffer, sizeof(buffer), "%i", *static_cast<const int32_t*>(data)); break;
      case LIBXSTREAM_TYPE_U32:   LIBXSTREAM_SNPRINTF(buffer, sizeof(buffer), "%u", *static_cast<const uint32_t*>(data)); break;
      case LIBXSTREAM_TYPE_I64:   LIBXSTREAM_SNPRINTF(buffer, sizeof(buffer), "%li", *static_cast<const int64_t*>(data)); break;
      case LIBXSTREAM_TYPE_U64:   LIBXSTREAM_SNPRINTF(buffer, sizeof(buffer), "%lu", *static_cast<const uint64_t*>(data)); break;
      case LIBXSTREAM_TYPE_F32:   LIBXSTREAM_SNPRINTF(buffer, sizeof(buffer), "%f", *static_cast<const float*>(data)); break;
      case LIBXSTREAM_TYPE_F64:   LIBXSTREAM_SNPRINTF(buffer, sizeof(buffer), "%f", *static_cast<const double*>(data)); break;
      case LIBXSTREAM_TYPE_C32: {
        const float *const c = static_cast<const float*>(data);
        LIBXSTREAM_SNPRINTF(buffer, sizeof(buffer), "(%f, %f)", c[0], c[1]);
      } break;
      case LIBXSTREAM_TYPE_C64: {
        const double *const c = static_cast<const double*>(data);
        LIBXSTREAM_SNPRINTF(buffer, sizeof(buffer), "(%f, %f)", c[0], c[1]);
      } break;
      default: {
        result = LIBXSTREAM_ERROR_CONDITION;
      }
    }

    *value = buffer;
  }

  LIBXSTREAM_ASSERT(LIBXSTREAM_ERROR_NONE == result);
  return result;
}


LIBXSTREAM_EXPORT_C LIBXSTREAM_TARGET(mic) int libxstream_get_type(const libxstream_argument* signature, size_t arg, libxstream_type* type)
{
  LIBXSTREAM_CHECK_CONDITION(0 != type);
#if defined(LIBXSTREAM_DEBUG) && (!defined(LIBXSTREAM_OFFLOAD) || (0 == LIBXSTREAM_OFFLOAD))
  size_t arity = 0;
  LIBXSTREAM_ASSERT(LIBXSTREAM_ERROR_NONE == libxstream_get_arity(signature, &arity) && arg < arity);
#endif
  if (0 == signature) {
    const libxstream_context& context = libxstream_context::instance();
    if (0 == (LIBXSTREAM_CALL_EXTERNAL & context.flags)) {
      signature = context.signature;
    }
  }
  LIBXSTREAM_ASSERT(0 != signature);
  *type = signature[arg].type;
  return LIBXSTREAM_ERROR_NONE;
}


LIBXSTREAM_EXPORT_C LIBXSTREAM_TARGET(mic) int libxstream_get_dims(const libxstream_argument* signature, size_t arg, size_t* dims)
{
  LIBXSTREAM_CHECK_CONDITION(0 != dims);
#if defined(LIBXSTREAM_DEBUG) && (!defined(LIBXSTREAM_OFFLOAD) || (0 == LIBXSTREAM_OFFLOAD))
  size_t arity = 0;
  LIBXSTREAM_ASSERT(LIBXSTREAM_ERROR_NONE == libxstream_get_arity(signature, &arity) && arg < arity);
#endif
  if (0 == signature) {
    const libxstream_context& context = libxstream_context::instance();
    if (0 == (LIBXSTREAM_CALL_EXTERNAL & context.flags)) {
      signature = context.signature;
    }
  }
  LIBXSTREAM_ASSERT(0 != signature);
  *dims = signature[arg].dims;
  return LIBXSTREAM_ERROR_NONE;
}


LIBXSTREAM_EXPORT_C LIBXSTREAM_TARGET(mic) int libxstream_get_shape(const libxstream_argument* signature, size_t arg, size_t shape[])
{
  LIBXSTREAM_CHECK_CONDITION(0 != shape);
#if defined(LIBXSTREAM_DEBUG) && (!defined(LIBXSTREAM_OFFLOAD) || (0 == LIBXSTREAM_OFFLOAD))
  size_t arity = 0;
  LIBXSTREAM_ASSERT(LIBXSTREAM_ERROR_NONE == libxstream_get_arity(signature, &arity) && arg < arity);
#endif
  if (0 == signature) {
    const libxstream_context& context = libxstream_context::instance();
    if (0 == (LIBXSTREAM_CALL_EXTERNAL & context.flags)) {
      signature = context.signature;
    }
  }
  LIBXSTREAM_ASSERT(0 != signature);
  const libxstream_argument& argument = signature[arg];
  const size_t dims = argument.dims;

  if (0 < dims) {
    const size_t *const src = argument.shape;
    LIBXSTREAM_PRAGMA_LOOP_COUNT(1, LIBXSTREAM_MAX_NDIMS, 2)
    for (size_t i = 0; i < dims; ++i) shape[i] = src[i];
  }
  else {
    *shape = 0;
  }
  return LIBXSTREAM_ERROR_NONE;
}


LIBXSTREAM_EXPORT_C LIBXSTREAM_TARGET(mic) int libxstream_get_size(const libxstream_argument* signature, size_t arg, size_t* size)
{
  LIBXSTREAM_CHECK_CONDITION(0 != size);
#if defined(LIBXSTREAM_DEBUG) && (!defined(LIBXSTREAM_OFFLOAD) || (0 == LIBXSTREAM_OFFLOAD))
  size_t arity = 0;
  LIBXSTREAM_ASSERT(LIBXSTREAM_ERROR_NONE == libxstream_get_arity(signature, &arity) && arg < arity);
#endif
  if (0 == signature) {
    const libxstream_context& context = libxstream_context::instance();
    if (0 == (LIBXSTREAM_CALL_EXTERNAL & context.flags)) {
      signature = context.signature;
    }
  }
  LIBXSTREAM_ASSERT(0 != signature);
  const libxstream_argument& argument = signature[arg];
  *size = libxstream_linear_size(argument.dims, argument.shape, 1);
  return LIBXSTREAM_ERROR_NONE;
}


LIBXSTREAM_EXPORT_C LIBXSTREAM_TARGET(mic) int libxstream_get_elemsize(const libxstream_argument* signature, size_t arg, size_t* size)
{
  LIBXSTREAM_CHECK_CONDITION(0 != size);
#if defined(LIBXSTREAM_DEBUG) && (!defined(LIBXSTREAM_OFFLOAD) || (0 == LIBXSTREAM_OFFLOAD))
  size_t arity = 0;
  LIBXSTREAM_ASSERT(LIBXSTREAM_ERROR_NONE == libxstream_get_arity(signature, &arity) && arg < arity);
#endif
  if (0 == signature) {
    const libxstream_context& context = libxstream_context::instance();
    if (0 == (LIBXSTREAM_CALL_EXTERNAL & context.flags)) {
      signature = context.signature;
    }
  }
  LIBXSTREAM_ASSERT(0 != signature);
  const libxstream_argument& argument = signature[arg];
  size_t typesize = 0 != argument.dims ? 1 : *argument.shape;
  if (LIBXSTREAM_TYPE_VOID != argument.type) {
    LIBXSTREAM_CHECK_CALL(libxstream_get_typesize(argument.type, &typesize));
  }
  *size = typesize;
  return LIBXSTREAM_ERROR_NONE;
}


LIBXSTREAM_EXPORT_C LIBXSTREAM_TARGET(mic) int libxstream_get_datasize(const libxstream_argument* signature, size_t arg, size_t* size)
{
  LIBXSTREAM_CHECK_CONDITION(0 != size);
#if defined(LIBXSTREAM_DEBUG) && (!defined(LIBXSTREAM_OFFLOAD) || (0 == LIBXSTREAM_OFFLOAD))
  size_t arity = 0;
  LIBXSTREAM_ASSERT(LIBXSTREAM_ERROR_NONE == libxstream_get_arity(signature, &arity) && arg < arity);
#endif
  if (0 == signature) {
    const libxstream_context& context = libxstream_context::instance();
    if (0 == (LIBXSTREAM_CALL_EXTERNAL & context.flags)) {
      signature = context.signature;
    }
  }
  LIBXSTREAM_ASSERT(0 != signature);
  const libxstream_argument& argument = signature[arg];
  size_t typesize = 0 != argument.dims ? 1 : *argument.shape;
  if (LIBXSTREAM_TYPE_VOID != argument.type) {
    LIBXSTREAM_CHECK_CALL(libxstream_get_typesize(argument.type, &typesize));
  }
  *size = libxstream_linear_size(argument.dims, argument.shape, typesize);
  return LIBXSTREAM_ERROR_NONE;
}


LIBXSTREAM_EXPORT_C LIBXSTREAM_TARGET(mic) int libxstream_get_verbosity(int* level)
{
  LIBXSTREAM_CHECK_CONDITION(0 != level);
  int result = LIBXSTREAM_ERROR_NONE, verbosity = libxstream_internal::context.verbosity();

  if (-1 > verbosity) {
    verbosity = libxstream_internal::context.global_verbosity();
    libxstream_internal::context.verbosity() = verbosity;

    if (-1 > verbosity) {
      verbosity = libxstream_internal::context.default_verbosity();
      libxstream_internal::context.global_verbosity(verbosity);
      libxstream_internal::context.verbosity() = verbosity;
    }

    //LIBXSTREAM_PRINT(2, "get_verbosity: level=%i (default) thread=%i", verbosity, this_thread_id());
  }

  *level = verbosity;

  LIBXSTREAM_ASSERT(LIBXSTREAM_ERROR_NONE == result);
  return result;
}


LIBXSTREAM_EXPORT_C LIBXSTREAM_TARGET(mic) int libxstream_set_verbosity(int level)
{
  LIBXSTREAM_CHECK_CONDITION(-1 <= level);

  if (-1 > libxstream_internal::context.global_verbosity()) {
    libxstream_internal::context.global_verbosity(level);
  }

  libxstream_internal::context.verbosity() = level;
  LIBXSTREAM_PRINT(2, "set_verbosity: level=%i thread=%i", level, this_thread_id());

  return LIBXSTREAM_ERROR_NONE;
}


LIBXSTREAM_EXPORT_C LIBXSTREAM_TARGET(mic) int libxstream_print(int verbosity, const char* message, ...)
{
  int level = 0, result = libxstream_get_verbosity(&level);

  if (LIBXSTREAM_ERROR_NONE == result && ((level >= verbosity && 0 != verbosity) || 0 > level)) {
    va_list args;
    va_start(args, message);
    LIBXSTREAM_FLOCK(stderr);
    vfprintf(stderr, message, args);
    LIBXSTREAM_FUNLOCK(stderr);
    va_end(args);
  }

  LIBXSTREAM_ASSERT(LIBXSTREAM_ERROR_NONE == result);
  return result;
}

#endif // defined(LIBXSTREAM_EXPORTED) || defined(__LIBXSTREAM) || defined(__LIBXSTREAM)
