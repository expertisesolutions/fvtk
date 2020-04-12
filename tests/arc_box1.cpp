//////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 Felipe Magno de Almeida.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// See http://www.boost.org/libs/foreach for documentation
//

#include <ftk/ui/backend/uv/uv_loop.hpp>
#include <ftk/ui/toplevel_window.hpp>
#include <ftk/ui/backend/vulkan/text.hpp>

#include <ftk/ui/backend/vulkan_backend.hpp>
#include <ftk/ui/backend/x11_base_backend.hpp>
#include <ftk/ui/backend/uv/uv_loop.hpp>
#include <ftk/ui/backend/uv/timer.hpp>
#include <ftk/ui/backend/vulkan/draw.hpp>
#include <ftk/ui/backend/vulkan_backend.ipp>
#include <ftk/ui/region_union.hpp>
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

  typedef ftk::ui::backend::vulkan_backend<ftk::ui::backend::uv::uv_loop, ftk::ui::backend::xlib_surface_backend<ftk::ui::backend::uv::uv_loop>> backend_type;
  backend_type backend({&loop});

  auto vulkan_window = backend.create_window(1280, 1000, res_path);
  
  typedef pc::inplace_executor_t executor_type;
  ftk::ui::backend::vulkan::submission_pool<executor_type>
    vulkan_submission_pool (vulkan_window.voutput.device
                            , &vulkan_window.queues
                            , pc::inplace_executor
                            , 1 /* thread count */);

  auto empty_image = ftk::ui::backend::vulkan::load_empty_image_view
    (vulkan_window.voutput.device, vulkan_window.voutput.physical_device
     , vulkan_submission_pool).get();
  
  ftk::ui::toplevel_window<backend_type&> w(vulkan_window, res_path, empty_image.image_view);

  std::cout << "w width " << w.window.voutput.swapChainExtent.width << std::endl;
  
  // w.append_arc_quadractic ({10, 10}, {200, 200}, {500, 500});

  // w.append_arc_quadractic ({10, 10}, {200, 200}, {500, 1000});
  // w.append_arc_quadractic ({2*(200 - 10), 2*(200-10)}, {2*(500-200), 2*(1000-200)}, {500, 1000});
  // w.append_arc_cubic ({0, 100}, {600, 300}, {700, 600}, {1200, 1500});
  // w.append_arc_cubic ({0, 0}, {1280, 0}, {0, 1000}, {1280, 1000});
  // w.append_arc_cubic ({0, 0}, {1280 , 900}, {1, 900}, {1280, 1});

  // w.append_arc_quadractic ({600, 200}, {100, 300}, {500, 900}); // tangent
  // w.append_arc_quadractic ({-200+1280, 600}, {-300+1280, 100}, {-900+1280, 500}); // normal?
  // w.append_arc_cubic ({10, 10}, {300, 300}, {600, 600}, {1000, 1000});

  std::pair<int, int> arcs[][4] =
    {
     {{10, 10}, {200,200}, {500, 1000}, {0, 0}}
     , {{380, 380}, {600,1600}, {500, 1000}, {0, 0}}
     , {{0, 100}, {600, 300}, {700, 600}, {1200, 1500}}
     , {{0, 0}, {1280, 0}, {0, 1000}, {1280, 1000}}
     , {{0, 0}, {1280 , 900}, {1, 900}, {1280, 1}}
    };

  std::pair<int, int> modifiers[] =
    {
     {0, 0}
     , {100, 100}
     , {20, 100}
     , {1000, 200}
     , {200, 1000}
     , {90, 300}
     , {300, 600}
    };

  auto first_mod = &modifiers[0]
    , last_mod = first_mod + sizeof(modifiers)/sizeof(modifiers[0]);
  for (; first_mod != last_mod; ++first_mod)
  {
    for (int x_mod = 0; x_mod < 500; x_mod += 150)
    //int x_mod = 0;
    {
      int y_mod = 0;
      // for (int y_mod = 0; y_mod != 500; y_mod += 10)
      // {
        auto arc_first = &arcs[0]
          , arc_last = &arcs[0] + sizeof(arcs)/sizeof(arcs[0]);
        for (;arc_first != arc_last; ++arc_first)
        {
          if ((*arc_first)[3].first == 0
              && (*arc_first)[3].second == 0)
            w.append_arc_quadractic ({(*arc_first)[0].first + x_mod + first_mod->first, (*arc_first)[0].second + y_mod + first_mod->second}
                                , {(*arc_first)[1].first + x_mod + first_mod->first, (*arc_first)[1].second + y_mod + first_mod->second}
                                , {(*arc_first)[2].first + x_mod + first_mod->first, (*arc_first)[2].second + y_mod + first_mod->second});
          else
            w.append_arc_cubic ({(*arc_first)[0].first + x_mod + first_mod->first, (*arc_first)[0].second + y_mod + first_mod->second}
                                , {(*arc_first)[1].first + x_mod + first_mod->first, (*arc_first)[1].second + y_mod + first_mod->second}
                                , {(*arc_first)[2].first + x_mod + first_mod->first, (*arc_first)[2].second + y_mod + first_mod->second}
                                , {(*arc_first)[3].first + x_mod + first_mod->first, (*arc_first)[3].second + y_mod + first_mod->second});
        }
        //}
    }
  }

  try
  {
    int i = 0;
    while (true)
    {
      auto now = std::chrono::high_resolution_clock::now();
      std::cout << "drawing " << i++ << std::endl;
      ftk::ui::backend::vulkan::draw_and_present (w, true);
      auto diff = std::chrono::high_resolution_clock::now() - now;
      std::cout << "Time between drawing and presentation "
                << std::chrono::duration_cast<std::chrono::milliseconds>(diff).count()
                << "ms" << std::endl;
    }
  }
  catch(...)
  {
    std::cerr << "exiting with an exception " << std::endl;
    return -1;
  }
  
  ftk::ui::backend::uv::timer_wait
    (backend.loop
     , 205000
     , [&] (uv_timer_t* timer)
       {
         uv_stop (&loop);
       });  
  
  uv_run(&loop, UV_RUN_DEFAULT);

  uv_loop_close(&loop);
}
