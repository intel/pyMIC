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
#ifndef LIBXSTREAM_WORKQUEUE_HPP
#define LIBXSTREAM_WORKQUEUE_HPP

#include "libxstream.hpp"

#if defined(LIBXSTREAM_EXPORTED) || defined(__LIBXSTREAM)


class libxstream_workitem;


class libxstream_workqueue {
public:
  class entry_type {
  public:
    entry_type(libxstream_workqueue* queue = 0, libxstream_workitem* item = reinterpret_cast<libxstream_workitem*>(-1))
      : m_status((0 != queue && 0 != item) ? LIBXSTREAM_ERROR_NONE : LIBXSTREAM_ERROR_CONDITION), m_queue(queue), m_dangling(0), m_item(item)
    {}
  public:
    bool valid() const { return reinterpret_cast<libxstream_workitem*>(-1) != m_item; }
    const libxstream_workqueue* queue() const { return m_queue; }
    const libxstream_workitem* dangling() const { return m_dangling; }
    const libxstream_workitem* item() const { return m_item; }
    libxstream_workitem* item() { return m_item; }
    int status() const { return m_status; }
    int& status() { return m_status; }
    void push(libxstream_workitem& workitem);
    /**
     * Wait until the workitem has been executed i.e., regardless of the thread owning the stream (any thread).
     * Otherwise the wait period is omitted if the current thread is still the same since enqueuing the item.
     */
    int wait(bool any = true, bool any_status = true) const;
    void execute();
    void pop();
  private:
    mutable int m_status;
    libxstream_workqueue* m_queue;
    const libxstream_workitem* m_dangling;
    libxstream_workitem* m_item; // last
  };

public:
  libxstream_workqueue();
  ~libxstream_workqueue();

public:
  size_t size() const;

  entry_type& allocate_entry_mt();
  entry_type& allocate_entry();

  const entry_type* front() const {
#if defined(LIBXSTREAM_DEBUG)
    return 0 < size() ? (m_buffer + LIBXSTREAM_MOD(m_index, LIBXSTREAM_MAX_QSIZE)) : 0;
#else
    return m_buffer + LIBXSTREAM_MOD(m_index, LIBXSTREAM_MAX_QSIZE);
#endif
  }

  entry_type* front() {
#if defined(LIBXSTREAM_DEBUG)
    return 0 < size() ? (m_buffer + LIBXSTREAM_MOD(m_index, LIBXSTREAM_MAX_QSIZE)) : 0;
#else
    return m_buffer + LIBXSTREAM_MOD(m_index, LIBXSTREAM_MAX_QSIZE);
#endif
  }

  const entry_type* back() const {
    const size_t s = size();
#if defined(LIBXSTREAM_DEBUG)
    return 0 < s ? (m_buffer + LIBXSTREAM_MOD(s - 1, LIBXSTREAM_MAX_QSIZE)) : 0;
#else
    return m_buffer + LIBXSTREAM_MOD(s - 1, LIBXSTREAM_MAX_QSIZE);
#endif
  }

  entry_type* back() {
    const size_t s = size();
#if defined(LIBXSTREAM_DEBUG)
    return 0 < s ? (m_buffer + LIBXSTREAM_MOD(s - 1, LIBXSTREAM_MAX_QSIZE)) : 0;
#else
    return m_buffer + LIBXSTREAM_MOD(s - 1, LIBXSTREAM_MAX_QSIZE);
#endif
  }

  void pop() { ++m_index; }

private:
  entry_type m_buffer[LIBXSTREAM_MAX_QSIZE];
  size_t m_index;
  void* m_size;
};

#endif // defined(LIBXSTREAM_EXPORTED) || defined(__LIBXSTREAM)
#endif // LIBXSTREAM_WORKQUEUE_HPP
