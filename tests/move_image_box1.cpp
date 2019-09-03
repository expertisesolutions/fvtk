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

#include <ftk/ui/backend/vulkan_backend.hpp>
#include <ftk/ui/backend/x11_base_backend.hpp>
#include <ftk/ui/backend/uv/uv_loop.hpp>
#include <ftk/ui/backend/uv/timer.hpp>
#include <ftk/ui/backend/vulkan/draw.hpp>
#include <ftk/ui/backend/vulkan_backend.ipp>
#include <ftk/ui/backend/vulkan/image.hpp>

#include <fastdraw/image_loader/extension_loader.hpp>

#include <filesystem>

#include <portable_concurrency/thread_pool>

int main(int argc, char* argv[])
{
  uv_loop_t loop;
  uv_loop_init (&loop);

  if (argc < 3)
  {
    std::cout << "Pass directories for resource paths" << std::endl;
    return 1;
  }

  std::filesystem::path res_path (argv[1]);
  std::filesystem::path test_res_path (argv[2]);

  std::cout << "resource path " << res_path << std::endl;

  typedef ftk::ui::backend::vulkan_backend<ftk::ui::backend::uv::uv_loop, ftk::ui::backend::xlib_surface_backend<ftk::ui::backend::uv::uv_loop>> backend_type;
  backend_type backend({&loop});

  auto vulkan_window = backend.create_window(1280, 1000, res_path);
  
  portable_concurrency::static_thread_pool thread_pool {8};
  typedef pc::static_thread_pool::executor_type executor_type;
  ftk::ui::backend::vulkan::submission_pool<executor_type>
    vulkan_submission_pool (vulkan_window.voutput.device
                            , &vulkan_window.queues
                            , thread_pool.executor()
                            , 4 /* thread count */);

  auto empty_image = ftk::ui::backend::vulkan::load_empty_image_view
    (vulkan_window.voutput.device, vulkan_window.voutput.physical_device
     , vulkan_submission_pool).get();
  
  ftk::ui::toplevel_window<backend_type&> w(vulkan_window, res_path, empty_image.image_view);

  std::cout << "w width " << w.window.voutput.swapChainExtent.width << std::endl;

  fastdraw::image_loader::extension_loader fs_loader;
  ftk::ui::backend::vulkan::image_loader<executor_type> vulkan_loader
    {vulkan_window.voutput.device, vulkan_window.voutput.physical_device
     , &vulkan_submission_pool};

  auto image_path = test_res_path / "background.jpg";
  std::cout << "loading " << image_path.c_str() << std::endl;
  auto image = vulkan_loader.load (image_path, fs_loader);

  w.append_rectangle (0, 0, 600, 500, {0, 0, 255, 255});
  w.append_rectangle (600, 500, 600, 500, {0, 0, 255, 255});
  auto image_iterator = w.append_image (10, 10, 200, 200, image.get().image_view);
  ftk::ui::backend::vulkan::draw (w);

  std::size_t x = 10, y = 10;
  
  ftk::ui::backend::uv::timer_wait
    (backend.loop
     , 15
     , 15
     , [&] (uv_timer_t* timer)
       {
         w.move_component (image_iterator, x += 10, y += 10);
         ftk::ui::backend::vulkan::draw (w);
       });  

  ftk::ui::backend::uv::timer_wait
    (backend.loop
     , 1000
     , [&] (uv_timer_t* timer)
       {
         uv_stop (&loop);
       });  

  uv_run(&loop, UV_RUN_DEFAULT);

  uv_loop_close(&loop);
}
