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

namespace ftk { namespace ui {

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

  // Backend* backend_;
  mutable typename Backend::window window;
};
    
} }

#endif
