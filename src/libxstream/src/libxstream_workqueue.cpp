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
#include "libxstream_workqueue.hpp"
#include "libxstream_workitem.hpp"

#include <libxstream_begin.h>
#include <algorithm>
#include <cstdio>
#if defined(LIBXSTREAM_STDFEATURES)
# include <atomic>
#endif
#include <libxstream_end.h>


void libxstream_workqueue::entry_type::push(libxstream_workitem& workitem)
{
  delete m_dangling;
  const bool async = 0 == (LIBXSTREAM_CALL_WAIT & workitem.flags());
  const bool witem = 0 == (LIBXSTREAM_CALL_EVENT & workitem.flags());
  libxstream_workitem *const item = async ? workitem.clone() : &workitem;
  m_status = witem ? LIBXSTREAM_ERROR_NONE : LIBXSTREAM_ERROR_CONDITION;
  m_dangling = async ? item : 0;
  m_item = item;
}


int libxstream_workqueue::entry_type::wait(bool any, bool any_status) const
{
  int result = LIBXSTREAM_ERROR_CONDITION;

  if (any_status) {
    if (any || !valid()) {
#if defined(LIBXSTREAM_SLEEP_CLIENT)
      size_t cycle = 0;
      while (0 != m_item) this_thread_wait(cycle);
#else
      while (0 != m_item) this_thread_yield();
#endif
    }
    else if (0 != m_item && m_item->thread() != this_thread_id()) {
#if defined(LIBXSTREAM_SLEEP_CLIENT)
      size_t cycle = 0;
#endif
      do {
#if defined(LIBXSTREAM_SLEEP_CLIENT)
        this_thread_wait(cycle);
#else
        this_thread_yield();
#endif
      }
      while (0 != m_item);
    }

    result = m_status;
  }
  else {
    if (any || !valid()) {
#if defined(LIBXSTREAM_SLEEP_CLIENT)
      size_t cycle = 0;
      while (0 != m_item || LIBXSTREAM_ERROR_NONE != m_status) this_thread_wait(cycle);
#else
      while (0 != m_item || LIBXSTREAM_ERROR_NONE != m_status) this_thread_yield();
#endif
    }
    else if (0 != m_item && m_item->thread() != this_thread_id()) {
#if defined(LIBXSTREAM_SLEEP_CLIENT)
      size_t cycle = 0;
#endif
      do {
#if defined(LIBXSTREAM_SLEEP_CLIENT)
        this_thread_wait(cycle);
#else
        this_thread_yield();
#endif
      }
      while (0 != m_item || LIBXSTREAM_ERROR_NONE != m_status);
    }

    result = m_status;
  }

  //LIBXSTREAM_ASSERT(LIBXSTREAM_ERROR_NONE == result);
  return result;
}


void libxstream_workqueue::entry_type::execute()
{
  LIBXSTREAM_ASSERT(0 != m_item && (m_item == m_dangling || 0 == m_dangling));
  (*m_item)(*this);
}


void libxstream_workqueue::entry_type::pop()
{
  if (!valid() || 0 == (LIBXSTREAM_CALL_LOOP & m_item->flags())) {
    m_item = 0;
    LIBXSTREAM_ASSERT(0 != m_queue);
    m_queue->pop();
  }
}


libxstream_workqueue::libxstream_workqueue()
  : m_index(0)
#if defined(LIBXSTREAM_STDFEATURES)
  , m_size(new std::atomic<size_t>(0))
#else
  , m_size(new size_t(0))
#endif
{
  std::fill_n(m_buffer, LIBXSTREAM_MAX_QSIZE, entry_type(this, 0));
}


libxstream_workqueue::~libxstream_workqueue()
{
#if defined(LIBXSTREAM_DEBUG)
  size_t pending = 0;
  for (size_t i = 0; i < LIBXSTREAM_MAX_QSIZE; ++i) {
    pending += (0 == m_buffer[i].item()) ? 0 : 1;
    delete m_buffer[i].dangling();
  }
  if (0 < pending) {
    LIBXSTREAM_PRINT(1, "%lu work item%s pending!", static_cast<unsigned long>(pending), 1 < pending ? "s are" : " is");
  }
#endif
#if defined(LIBXSTREAM_STDFEATURES)
  delete static_cast<std::atomic<size_t>*>(m_size);
#else
  delete static_cast<size_t*>(m_size);
#endif
}


size_t libxstream_workqueue::size() const
{
#if defined(LIBXSTREAM_STDFEATURES)
  const size_t atomic_size = *static_cast<const std::atomic<size_t>*>(m_size);
#else
  const size_t atomic_size = *static_cast<const size_t*>(m_size);
#endif
  return atomic_size - m_index;
}


libxstream_workqueue::entry_type& libxstream_workqueue::allocate_entry_mt()
{
  entry_type* result = 0;
#if defined(LIBXSTREAM_STDFEATURES)
  result = m_buffer + LIBXSTREAM_MOD((*static_cast<std::atomic<size_t>*>(m_size))++, LIBXSTREAM_MAX_QSIZE);
#elif defined(_OPENMP)
  size_t size1 = 0;
  size_t& size = *static_cast<size_t*>(m_size);
# if (201107 <= _OPENMP)
# pragma omp atomic capture
# else
# pragma omp critical
# endif
  size1 = ++size;
  result = m_buffer + LIBXSTREAM_MOD(size1 - 1, LIBXSTREAM_MAX_QSIZE);
#else // generic
  size_t& size = *static_cast<size_t*>(m_size);
  libxstream_lock *const lock = libxstream_lock_get(this);
  libxstream_lock_acquire(lock);
  result = m_buffer + LIBXSTREAM_MOD(size++, LIBXSTREAM_MAX_QSIZE);
  libxstream_lock_release(lock);
#endif
  LIBXSTREAM_ASSERT(0 != result && result->queue() == this);

  if (0 != result->item()) {
    LIBXSTREAM_PRINT0(1, "queuing work is stalled!");
    do { // stall if capacity is exceeded
      this_thread_sleep();
    }
    while (0 != result->item());
  }

  return *result;
}


libxstream_workqueue::entry_type& libxstream_workqueue::allocate_entry()
{
  entry_type* result = 0;
#if defined(LIBXSTREAM_STDFEATURES)
  result = m_buffer + LIBXSTREAM_MOD(static_cast<std::atomic<size_t>*>(m_size)->fetch_add(1, std::memory_order_relaxed), LIBXSTREAM_MAX_QSIZE);
#else
  size_t& size = *static_cast<size_t*>(m_size);
  result = m_buffer + LIBXSTREAM_MOD(size++, LIBXSTREAM_MAX_QSIZE);
#endif
  LIBXSTREAM_ASSERT(0 != result && result->queue() == this);

  if (0 != result->item()) {
    LIBXSTREAM_PRINT0(1, "queuing work is stalled!");
    do { // stall if capacity is exceeded
      this_thread_sleep();
    }
    while (0 != result->item());
  }

  return *result;
}

#endif // defined(LIBXSTREAM_EXPORTED) || defined(__LIBXSTREAM)
