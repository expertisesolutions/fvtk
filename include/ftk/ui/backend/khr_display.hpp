///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 Felipe Magno de Almeida.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// See http://www.boost.org/libs/foreach for documentation
//

#ifndef FTK_FTK_UI_BACKEND_KHR_DISPLAY_HPP
#define FTK_FTK_UI_BACKEND_KHR_DISPLAY_HPP

namespace ftk { namespace ui { namespace backend {

struct khr_display
{
  template <typename Loop>
  khr_display (Loop const&) {}
  
  struct window
  {
  };

  window create_window(int width, int height) const
  {
    return {};
  }
};

      
} } }
      
#endif
