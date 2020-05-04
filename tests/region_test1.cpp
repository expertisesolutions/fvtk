//////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019-2020 Felipe Magno de Almeida.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// See http://www.boost.org/libs/foreach for documentation
//

#include <ftk/ui/region_union.hpp>
#include <chrono>

int main(int argc, char* argv[])
{
  ftk::ui::region_union<int> union_;

  // union_.add_region({10, 10, 970, 970});
  // union_.add_region({0, 0, 990, 990});
  // union_.add_region({0, 0, 600, 990});
  // union_.add_region({600, 0, 50, 990});
  // union_.add_region({600, 0, 200, 990});
  // union_.add_region({500, 900, 200, 200});

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
      //int y_mod = 0;
      for (int y_mod = 0; y_mod != 500; y_mod += 10)
      {
        auto arc_first = &arcs[0]
          , arc_last = &arcs[0] + sizeof(arcs)/sizeof(arcs[0]);
        for (;arc_first != arc_last; ++arc_first)
        {
          if ((*arc_first)[3].first == 0
              && (*arc_first)[3].second == 0)
          {
            union_.add_region ({(*arc_first)[0].first + x_mod + first_mod->first, (*arc_first)[0].second + y_mod + first_mod->second
                                , (*arc_first)[2].first + x_mod + first_mod->first, (*arc_first)[2].second + y_mod + first_mod->second});
          }
          else
          {
            union_.add_region ({(*arc_first)[0].first + x_mod + first_mod->first, (*arc_first)[0].second + y_mod + first_mod->second
                                , (*arc_first)[3].first + x_mod + first_mod->first, (*arc_first)[3].second + y_mod + first_mod->second});
          }
        }
      }
    }
  }

  auto now = std::chrono::high_resolution_clock::now();
  auto r = union_.non_overlapping_regions();
  auto diff = std::chrono::high_resolution_clock::now() - now;
  std::cout << "Time calculation "
            << std::chrono::duration_cast<std::chrono::milliseconds>(diff).count()
            << "ms for " << union_.rectangles.size() << " old rectangles and new " << r.size() << std::endl;

  return -1;
}
