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

#include <filesystem>

int main(int argc, char* argv[])
{
  uv_loop_t loop;
  uv_loop_init (&loop);

  if (argc < 2)
  {
    std::cout << "Pass directory for resource path" << std::endl;
    return 1;
  }

  std::filesystem::path res_path (argv[1]);

  std::cout << "resource path " << res_path << std::endl;

  typedef ftk::ui::backend::vulkan<ftk::ui::backend::uv, ftk::ui::backend::xlib_surface<ftk::ui::backend::uv>> backend_type;
  backend_type backend({&loop});

  auto vulkan_window = backend.create_window(1280, 1000, res_path);
  
  typedef pc::inplace_executor_t executor_type;
  ftk::ui::backend::vulkan_submission_pool<executor_type>
    vulkan_submission_pool (vulkan_window.voutput.device
                            , &vulkan_window.queues
                            , pc::inplace_executor
                            , 1 /* thread count */);

  auto empty_image = ftk::ui::backend::load_empty_image_view
    (vulkan_window.voutput.device, vulkan_window.voutput.physical_device
     , vulkan_submission_pool).get();
  
  ftk::ui::toplevel_window<backend_type&> w(vulkan_window, res_path, empty_image.image_view);

  std::cout << "w width " << w.window.voutput.swapChainExtent.width << std::endl;
  
  w.append_component ({10, 10, 200, 200, ftk::ui::rectangle_component{{255, 0, 0, 255}}});
  draw (w);
  
  uv_run(&loop, UV_RUN_DEFAULT);

  uv_loop_close(&loop);
}