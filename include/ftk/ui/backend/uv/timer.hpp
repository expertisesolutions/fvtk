///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 Felipe Magno de Almeida.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// See http://www.boost.org/libs/foreach for documentation
//

#ifndef FTK_UV_BACKEND_UV_TIMER_HPP
#define FTK_UV_BACKEND_UV_TIMER_HPP

#include <ftk/ui/backend/uv/uv_loop.hpp>

#include <utility>

namespace ftk { namespace ui { namespace backend { namespace uv {

template <typename F>
void timer_wait (uv_loop& loop, uint64_t timeout, uint64_t repeat, F function)
{
  uv_timer_t* handle = new uv_timer_t;
  ::uv_timer_init (loop.loop, handle);

  handle->data = new F(std::move(function));
  
  uv_timer_cb cb = [] (uv_timer_t* handle)
                  {
		    //std::cout << "event " << event << std::endl;
                    (*static_cast<F*>(handle->data))(handle);
                  };
  uv_timer_start (handle, cb, timeout, repeat);
}

template <typename F>
void timer_wait (uv_loop& loop, uint64_t timeout, F function)
{
  uv_timer_t* handle = new uv_timer_t;
  ::uv_timer_init (loop.loop, handle);

  handle->data = new F(std::move(function));
  
  uv_timer_cb cb = [] (uv_timer_t* handle)
                  {
		    //std::cout << "event " << event << std::endl;
                    (*static_cast<F*>(handle->data))(handle);
                  };
  uv_timer_start (handle, cb, timeout, 0);
}
      
} } } }

#endif
