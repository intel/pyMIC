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
#include "libxstream_event.hpp"
#include "libxstream_workitem.hpp"

#include <libxstream_begin.h>
#if defined(LIBXSTREAM_STDFEATURES)
# include <atomic>
#endif
#include <libxstream_end.h>

#if defined(LIBXSTREAM_OFFLOAD) && (0 != LIBXSTREAM_OFFLOAD)
# include <offload.h>
#endif


libxstream_event::libxstream_event()
  : m_slots(0)
#if defined(LIBXSTREAM_STDFEATURES)
  , m_expected(new std::atomic<size_t>(0))
#else
  , m_expected(new size_t(0))
#endif
{}


libxstream_event::libxstream_event(const libxstream_event& other)
  : m_slots(other.m_slots ? new slot_type[(LIBXSTREAM_MAX_NDEVICES)*(LIBXSTREAM_MAX_NSTREAMS)] : 0)
#if defined(LIBXSTREAM_STDFEATURES)
  , m_expected(other.m_expected ? new std::atomic<size_t>(static_cast<size_t>(*static_cast<const std::atomic<size_t>*>(other.m_expected))) : 0)
#else
  , m_expected(other.m_expected ? new size_t(*static_cast<const size_t*>(other.m_expected)) : 0)
#endif
{
  if (m_slots) {
    std::copy(&other.m_slots[0], &other.m_slots[0] + (LIBXSTREAM_MAX_NDEVICES)*(LIBXSTREAM_MAX_NSTREAMS), &m_slots[0]);
  }
}


libxstream_event::~libxstream_event()
{
  delete[] m_slots;
#if defined(LIBXSTREAM_STDFEATURES)
  delete static_cast<std::atomic<size_t>*>(m_expected);
#else
  delete static_cast<size_t*>(m_expected);
#endif
}


void libxstream_event::swap(libxstream_event& other)
{
  std::swap(m_slots, other.m_slots);
  std::swap(m_expected, other.m_expected);
}


int libxstream_event::record(libxstream_stream& stream, bool reset)
{
#if defined(LIBXSTREAM_STDFEATURES)
  std::atomic<size_t>& expected = *static_cast<std::atomic<size_t>*>(m_expected);
#else
  size_t& expected = *static_cast<size_t*>(m_expected);
#endif
  LIBXSTREAM_CHECK_CONDITION((LIBXSTREAM_MAX_NDEVICES)*(LIBXSTREAM_MAX_NSTREAMS) >= expected);

  if (reset) {
#if defined(LIBXSTREAM_STDFEATURES)
    expected = 0;
#elif defined(_OPENMP)
# if (201107 <= _OPENMP)
#   pragma omp atomic write
# else
#   pragma omp critical
#   endif
    expected = 0;
#else // generic
    libxstream_lock *const lock = libxstream_lock_get(this);
    libxstream_lock_acquire(lock);
    expected = 0;
    libxstream_lock_release(lock);
#endif
  }

  LIBXSTREAM_ASYNC_BEGIN
  {
    int& status = LIBXSTREAM_ASYNC_QENTRY.status();
#if defined(LIBXSTREAM_OFFLOAD)
    if (0 <= LIBXSTREAM_ASYNC_DEVICE) {
      if (LIBXSTREAM_ASYNC_READY) {
#       pragma offload LIBXSTREAM_ASYNC_TARGET_SIGNAL //out(status)
        {
          status = LIBXSTREAM_ERROR_NONE;
        }
      }
      else {
#       pragma offload LIBXSTREAM_ASYNC_TARGET_SIGNAL_WAIT //out(status)
        {
          status = LIBXSTREAM_ERROR_NONE;
        }
      }
    }
    else
#endif
    {
      status = LIBXSTREAM_ERROR_NONE;
    }
  }
  LIBXSTREAM_ASYNC_END(stream, LIBXSTREAM_CALL_DEFAULT | LIBXSTREAM_CALL_EVENT, work);

  if (0 == m_slots) {
    libxstream_lock *const lock = libxstream_lock_get(this);
    libxstream_lock_acquire(lock);

    if (0 == m_slots) {
      m_slots = new slot_type[(LIBXSTREAM_MAX_NDEVICES)*(LIBXSTREAM_MAX_NSTREAMS)];
      std::fill_n(m_slots, (LIBXSTREAM_MAX_NDEVICES)*(LIBXSTREAM_MAX_NSTREAMS), slot_type(0));
    }

    libxstream_lock_release(lock);
  }
  LIBXSTREAM_ASSERT(0 != m_slots);

#if defined(LIBXSTREAM_STDFEATURES)
  const size_t slot = ++expected;
#elif defined(_OPENMP)
  size_t slot = 0;
# if (201107 <= _OPENMP)
# pragma omp atomic capture
# else
# pragma omp critical
# endif
  slot = ++expected;
#else // generic
  size_t slot = 0;
  libxstream_lock *const lock = libxstream_lock_get(this);
  libxstream_lock_acquire(lock);
  slot = ++expected;
  libxstream_lock_release(lock);
#endif
  m_slots[slot-1] = &LIBXSTREAM_ASYNC_INTERNAL(work);

  return LIBXSTREAM_ERROR_NONE;
}


int libxstream_event::query(bool& occurred, const libxstream_stream* exclude) const
{
#if defined(LIBXSTREAM_STDFEATURES)
  const size_t expected = *static_cast<const std::atomic<size_t>*>(m_expected);
#else
  const size_t expected = *static_cast<const size_t*>(m_expected);
#endif
  LIBXSTREAM_ASSERT((LIBXSTREAM_MAX_NDEVICES)*(LIBXSTREAM_MAX_NSTREAMS) >= expected);
  LIBXSTREAM_ASSERT(0 == expected || 0 != m_slots);
  occurred = true; // everythig "occurred" if nothing is expected
  int result = LIBXSTREAM_ERROR_NONE;

  if (0 != m_slots) {
    for (size_t i = 0; i < expected; ++i) {
      const slot_type slot = m_slots[i];
      const libxstream_workitem *const item = slot ? slot->item() : 0;
      if ((0 != item && exclude != item->stream()) || LIBXSTREAM_ERROR_NONE != slot->status()) {
        occurred = false;
        i = expected; // break
      }
    }
  }

  LIBXSTREAM_ASSERT(LIBXSTREAM_ERROR_NONE == result);
  return result;
}


int libxstream_event::wait(const libxstream_stream* exclude, bool any)
{
#if defined(LIBXSTREAM_STDFEATURES)
  const size_t expected = *static_cast<std::atomic<size_t>*>(m_expected);
#else
  const size_t expected = *static_cast<size_t*>(m_expected);
#endif
  LIBXSTREAM_ASSERT((LIBXSTREAM_MAX_NDEVICES)*(LIBXSTREAM_MAX_NSTREAMS) >= expected);
  LIBXSTREAM_ASSERT(0 == expected || 0 != m_slots);
  int result = LIBXSTREAM_ERROR_NONE;

  if (0 != m_slots && 0 < expected) {
    for (size_t i = 0; i < expected; ++i) {
      const slot_type slot = m_slots[i];
      const libxstream_workitem *const item = slot ? slot->item() : 0;
      if (0 != item && exclude != item->stream()) {
        result = slot->wait(any, false);
        LIBXSTREAM_CHECK_ERROR(result);
        m_slots[i] = 0;
      }
    }

#if defined(LIBXSTREAM_STDFEATURES)
    std::atomic<size_t>& expected_dst = *static_cast<std::atomic<size_t>*>(m_expected);
#else
    size_t& expected_dst = *static_cast<size_t*>(m_expected);
#endif
    LIBXSTREAM_ASSERT(expected == expected_dst);

#if defined(LIBXSTREAM_STDFEATURES)
    expected_dst = 0;
#elif defined(_OPENMP)
# if (201107 <= _OPENMP)
#   pragma omp atomic write
# else
#   pragma omp critical
#   endif
    expected_dst = 0;
#else // generic
    libxstream_lock *const lock = libxstream_lock_get(this);
    libxstream_lock_acquire(lock);
    expected_dst = 0;
    libxstream_lock_release(lock);
#endif
  }

  LIBXSTREAM_ASSERT(LIBXSTREAM_ERROR_NONE == result);
  return result;
}


int libxstream_event::wait_stream(libxstream_stream* stream)
{
  LIBXSTREAM_ASYNC_BEGIN
  {
    const libxstream_event& event = *ptr<const libxstream_event,0>();
    int result = LIBXSTREAM_ERROR_NONE;
    bool occurred = true;

    if (0 == this->event()) {
      this->event(&event);
    }

    result = event.query(occurred, LIBXSTREAM_ASYNC_STREAM);

    if (occurred) {
      flags(flags() & ~LIBXSTREAM_CALL_LOOP); // clear
    }

    LIBXSTREAM_ASYNC_QENTRY.status() = result;
  }
  LIBXSTREAM_ASYNC_END(stream, LIBXSTREAM_CALL_DEFAULT | LIBXSTREAM_CALL_LOOP, work, new libxstream_event(*this));

  const int result = work.status();
  LIBXSTREAM_ASSERT(LIBXSTREAM_ERROR_NONE == result);
  return result;
}

#endif // defined(LIBXSTREAM_EXPORTED) || defined(__LIBXSTREAM)
