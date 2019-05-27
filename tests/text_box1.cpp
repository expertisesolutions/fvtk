///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 Felipe Magno de Almeida.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// See http://www.boost.org/libs/foreach for documentation
//

#include <ftk/ui/toplevel_window.hpp>
#include <ftk/ui/text_box.hpp>
#include <uv.h>

#include <ftk/ui/backend/vulkan.hpp>
#include <ftk/ui/backend/x11_base.hpp>
#include <ftk/ui/backend/uv.hpp>

#include <ftk/ui/backend/vulkan_draw.hpp>
#include <ftk/ui/backend/vulkan.ipp>

int main()
{
  uv_loop_t loop;
  uv_loop_init (&loop);

  typedef ftk::ui::backend::vulkan<ftk::ui::backend::uv> backend_type;
  backend_type backend({&loop});

  ftk::ui::toplevel_window<backend_type> w(backend);

  std::cout << "w width " << w.window.voutput.swapChainExtent.width << std::endl;
  
  ftk::ui::text_box tb;

  on_draw (backend, w);
  
  uv_run(&loop, UV_RUN_DEFAULT);
  //while(1);

  uv_loop_close(&loop);
}
