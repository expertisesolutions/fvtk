///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 Felipe Magno de Almeida.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// See http://www.boost.org/libs/foreach for documentation
//

#ifndef FTK_FTK_UI_TOPLEVEL_WINDOW_HPP
#define FTK_FTK_UI_TOPLEVEL_WINDOW_HPP

#include <functional>

#include <iostream>

#include <mutex>

namespace ftk { namespace ui {

struct toplevel_window_image
{
  VkImageView image_view;
  std::int32_t x, y, width, height;
};

struct toplevel_window_comand_buffer_cache
{
  VkCommandBuffer command_buffer;
};
    
template <typename Backend>
struct toplevel_window
{
  toplevel_window (Backend& backend)
  //: backend_(&backend)
    : window (backend.create_window(1280, 1000))
  {
    //backend_->exposure_signal.connect (std::bind(&toplevel_window<Backend>::exposure, this));
    
  }

  toplevel_window (toplevel_window&& other) = delete; // for now

  // void exposure ()
  // {
  //   std::cout << "exposure event for toplevel window" << std::endl;

  //   // cached scene
    
  // }

  // std::size_t  add_on_top (void* buffer, std::int32_t x, std::int32_t y, std::int32_t width, std::int32_t height, std::uint32_t stride)
  // {
  //   shm_buffers.push_back ({buffer, x, y, width, height, stride});
  // }

  //std::mutex render_mutex;
  
  // should be a fastdraw something
  std::vector<toplevel_window_image> images;
  std::vector<toplevel_window_comand_buffer_cache> command_buffers;
  
  // Backend* backend_;
  mutable typename Backend::window window;

  std::mutex image_mutex; // for images vector and command_buffers cache
};
    
} }

#endif
