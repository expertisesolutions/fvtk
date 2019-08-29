///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 Felipe Magno de Almeida.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// See http://www.boost.org/libs/foreach for documentation
//

#ifndef FTK_FTK_UI_BACKEND_LIBUV_HPP
#define FTK_FTK_UI_BACKEND_LIBUV_HPP

#include <uv.h>

#include <utility>

namespace ftk { namespace ui { namespace backend { namespace uv {

struct uv_loop
{
  ::uv_loop_t* loop;

  enum events
  {
    readable = UV_READABLE, writable = UV_WRITABLE
  };

  struct fd
  {
    ::uv_poll_t* handle;
    int fd_;
  };

  template <typename F>
  fd create_fd (int fd, int events, F f) const
  {
    uv_poll_t* handle = new uv_poll_t;
    ::uv_poll_init (loop, handle, fd);
    handle->data = new F(std::move(f));

    uv_poll_cb cb = [] (uv_poll_t* handle, int status, int event)
                    {
                      if (status == 0)
                        (*static_cast<F*>(handle->data))();
                    };
    
    ::uv_poll_start (handle, events, cb);
    return { handle, fd };
  }
};

} } } }

#endif
