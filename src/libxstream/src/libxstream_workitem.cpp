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
#include "libxstream_workitem.hpp"
#include "libxstream_event.hpp"

#include <libxstream_begin.h>
#include <algorithm>
#include <cstdio>
#if defined(LIBXSTREAM_STDFEATURES)
# include <thread>
# include <atomic>
#else
# if defined(__GNUC__)
#   include <pthread.h>
# else
#   include <Windows.h>
# endif
#endif
#include <libxstream_end.h>


namespace libxstream_workitem_internal {

class scheduler_type {
public:
  typedef libxstream_workqueue::entry_type entry_type;

public:
  scheduler_type()
    : m_global_queue()
    , m_stream(0)
#if defined(LIBXSTREAM_STDFEATURES)
    , m_thread() // do not start here
#else
    , m_thread(0)
#endif
    , m_terminated(false)
  {}

  ~scheduler_type() {
    if (running()) {
      m_terminated = true;

      // terminates the background thread
      entry_type& entry = m_global_queue.allocate_entry();
      delete entry.dangling();
      entry = entry_type(&m_global_queue);
      entry.wait();

#if defined(LIBXSTREAM_STDFEATURES)
      m_thread.detach();
#else
# if defined(__GNUC__)
      pthread_detach(m_thread);
# else
      CloseHandle(m_thread);
# endif
#endif
    }
  }

public:
  bool running() const {
#if defined(LIBXSTREAM_STDFEATURES)
    return m_thread.joinable() && !m_terminated;
#else
    return 0 != m_thread && !m_terminated;
#endif
  }

  void start() {
    if (!m_terminated && !running()) {
      libxstream_lock *const lock = libxstream_lock_get(this);
      libxstream_lock_acquire(lock);

      if (!running()) {
#if defined(LIBXSTREAM_STDFEATURES)
        std::thread(run, this).swap(m_thread);
#else
# if defined(__GNUC__)
        pthread_create(&m_thread, 0, run, this);
# else
        m_thread = CreateThread(0, 0, run, this, 0, 0);
# endif
#endif
      }

      libxstream_lock_release(lock);
    }
  }

  entry_type* front() {
    entry_type* result = m_global_queue.front();

    if (0 == result || 0 == result->item()) { // no item in global queue
      libxstream_stream *const stream = libxstream_stream::schedule(m_stream);
      result = stream ? stream->work() : 0;
      m_stream = stream;
    }

    return result;
  }

  entry_type& push(libxstream_workitem& workitem) {
    entry_type& entry = m_global_queue.allocate_entry_mt();
    entry.push(workitem);
    return entry;
  }

private:
#if defined(LIBXSTREAM_STDFEATURES) || defined(__GNUC__)
  static void* run(void* scheduler)
#else
  static DWORD WINAPI run(_In_ LPVOID scheduler)
#endif
  {
    scheduler_type& s = *static_cast<scheduler_type*>(scheduler);
    bool continue_run = true;

#if defined(LIBXSTREAM_ASYNCHOST) && (201307 <= _OPENMP)
#   pragma omp parallel
#   pragma omp master
#endif
    for (; continue_run;) {
      scheduler_type::entry_type* entry = s.front();
      size_t cycle = 0;

      while (0 == entry || 0 == entry->item()) {
        this_thread_wait(cycle);
        entry = s.front();
      }

      if (entry->valid()) {
        entry->execute();
#if defined(LIBXSTREAM_ASYNCHOST) && (201307 <= _OPENMP)
#       pragma omp taskwait
#endif
      }
      else {
        continue_run = false;
      }

      entry->pop();
    }

#if defined(LIBXSTREAM_STDFEATURES) || defined(__GNUC__)
    return scheduler;
#else
    return EXIT_SUCCESS;
#endif
  }

private:
  libxstream_workqueue m_global_queue;
  libxstream_stream* m_stream;
#if defined(LIBXSTREAM_STDFEATURES)
  std::thread m_thread;
#elif defined(__GNUC__)
  pthread_t m_thread;
#else
  HANDLE m_thread;
#endif
  bool m_terminated;
};
static/*IPO*/ scheduler_type scheduler;

} // namespace libxstream_workitem_internal


libxstream_workitem::libxstream_workitem(libxstream_stream* stream, int flags, size_t argc, const arg_type argv[], const char* name)
  : m_function(0)
  , m_stream(stream)
  , m_event(0)
  , m_pending(0)
  , m_flags(flags)
  , m_thread(this_thread_id())
#if defined(LIBXSTREAM_DEBUG)
  , m_name(name)
#endif
{
#if !defined(LIBXSTREAM_DEBUG)
  libxstream_use_sink(name);
#endif
  if (2 == argc && (argv[0].signature() || argv[1].signature())) {
    const libxstream_argument* signature = 0;
    if (argv[1].signature()) {
      m_function = *reinterpret_cast<const libxstream_function*>(argv + 0);
      signature = static_cast<const libxstream_argument*>(libxstream_get_value(argv[1]).const_pointer);
    }
    else {
      LIBXSTREAM_ASSERT(argv[0].signature());
      m_function = *reinterpret_cast<const libxstream_function*>(argv + 1);
      signature = static_cast<const libxstream_argument*>(libxstream_get_value(argv[0]).const_pointer);
    }

    size_t arity = 0;
    if (signature) {
      LIBXSTREAM_CHECK_CALL_ASSERT(libxstream_get_arity(signature, &arity));
      LIBXSTREAM_PRAGMA_LOOP_COUNT(0, LIBXSTREAM_MAX_NARGS, LIBXSTREAM_MAX_NARGS/2)
      for (size_t i = 0; i < arity; ++i) m_signature[i] = signature[i];
    }
    LIBXSTREAM_CHECK_CALL_ASSERT(libxstream_construct(m_signature, arity, libxstream_argument::kind_invalid, 0, LIBXSTREAM_TYPE_INVALID, 0, 0));
  }
  else {
    LIBXSTREAM_ASSERT(argc <= (LIBXSTREAM_MAX_NARGS));
    for (size_t i = 0; i < argc; ++i) m_signature[i] = argv[i];
    LIBXSTREAM_CHECK_CALL_ASSERT(libxstream_construct(m_signature, argc, libxstream_argument::kind_invalid, 0, LIBXSTREAM_TYPE_INVALID, 0, 0));
#if defined(LIBXSTREAM_DEBUG)
    size_t arity = 0;
    LIBXSTREAM_ASSERT(LIBXSTREAM_ERROR_NONE == libxstream_get_arity(m_signature, &arity) && arity == argc);
#endif
  }

  libxstream_workitem_internal::scheduler.start();
}


libxstream_workitem::~libxstream_workitem()
{
  delete m_event;
}


libxstream_workitem* libxstream_workitem::clone() const
{
  LIBXSTREAM_ASSERT(0 == (LIBXSTREAM_CALL_WAIT & m_flags));
  libxstream_workitem *const instance = virtual_clone();
  LIBXSTREAM_ASSERT(0 != instance);
  return instance;
}


void libxstream_workitem::operator()(libxstream_workqueue::entry_type& entry)
{
  LIBXSTREAM_ASSERT(this == entry.item());
  virtual_run(entry);
}


libxstream_workqueue::entry_type& libxstream_enqueue(libxstream_workitem* workitem)
{
  libxstream_workqueue::entry_type *const result = workitem
    ? &libxstream_workitem_internal::scheduler.push(*workitem)
    : libxstream_workitem_internal::scheduler.front();
  LIBXSTREAM_ASSERT(0 != result);
  return *result;
}

#endif // defined(LIBXSTREAM_EXPORTED) || defined(__LIBXSTREAM)
