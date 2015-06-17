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
#ifndef LIBXSTREAM_EVENT_HPP
#define LIBXSTREAM_EVENT_HPP

#include "libxstream_workqueue.hpp"

#if defined(LIBXSTREAM_EXPORTED) || defined(__LIBXSTREAM)


struct/*!class*/ libxstream_event {
public:
  libxstream_event();
  libxstream_event(const libxstream_event& other);

  ~libxstream_event();

public:
  libxstream_event& operator=(const libxstream_event& rhs) {
    libxstream_event tmp(rhs);
    swap(tmp);
    return *this;
  }

  void swap(libxstream_event& other);

  // Enqueue this event into the given stream; reset to start over.
  int record(libxstream_stream& stream, bool reset);

  // Query whether the event already happened or not.
  int query(bool& occurred, const libxstream_stream* exclude = 0) const;

  // Wait for the event to happen; optionally exclude events related to a given stream.
  int wait(const libxstream_stream* exclude, bool any);

  // Wait for the event to happen using a barrier i.e., waiting within the given stream.
  int wait_stream(libxstream_stream* stream);

private:
  typedef libxstream_workqueue::entry_type* slot_type;
  slot_type* m_slots;
  void* m_expected;
};

#endif // defined(LIBXSTREAM_EXPORTED) || defined(__LIBXSTREAM)
#endif // LIBXSTREAM_EVENT_HPP
