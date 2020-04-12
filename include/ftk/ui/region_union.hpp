///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019-2020 Felipe Magno de Almeida.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// See http://www.boost.org/libs/foreach for documentation
//

#ifndef FTK_FTK_UI_REGION_UNION_HPP
#define FTK_FTK_UI_REGION_UNION_HPP

#include <set>
#include <algorithm>

#include <iostream>

namespace ftk { namespace ui {

// namespace detail {

// template <typename T>
// struct region_union_node
// {
//   region_union_node* parent;
//   region_union_node* children[2];
//   region r;
// };
  
// }

namespace region_detail {
    
enum event_type
{
 left_edge, right_edge
};

inline std::ostream& operator<<(std::ostream& os, event_type t)
{
  return os << (t == left_edge ? "left_edge" : "right_edge");
}

}
    
template <typename T>
struct region_union
{
  using event_type = region_detail::event_type;
  struct event
  {
    event_type type;
    int x, x0, w;
    int y, h;

    friend bool operator<(event const& lhs, event const& rhs)
    {
      return
        lhs.x == rhs.x
        ?
        ( lhs.y == rhs.y
          ?
          ( lhs.h == rhs.h
            ?
            ( lhs.x0 == rhs.x0
              ?
              ( lhs.type == rhs.type
                ? lhs.w < rhs.w
                : lhs.type < rhs.type
              )
              : lhs.x0 < rhs.x0
            )
            : lhs.h < rhs.h
          )
          : lhs.y < rhs.y
        )
        : lhs.x < rhs.x;
    }

    friend std::ostream& operator<<(std::ostream& os, event const& e)
    {
      return os << "[\"event\" type: " << e.type << " x: " << e.x << " 'x0': " << e.x0 << " w: " << e.w << " y: " << e.y << " h: " << e.h << "]";
    }
  };
  
  std::multiset<event> events;
  
  struct region
  {
    T x, y, w, h;

    friend std::ostream& operator<<(std::ostream& os, region const& r)
    {
      return os << "[x: " << r.x << " y: " << r.y << " w: " << r.w << " h: " << r.h << "]";
    }
    friend bool operator<(region const& left, region const& right)
    {
      return left.x == right.x
        ? left.y == right.y
        ? left.w == right.w
        ? left.h < right.h
        : left.w < right.w
        : left.y < right.y
        : left.x < right.x
        ;
    }
  };

  void add_region (region r)
  {
    //std::cout << "adding region " << r << std::endl;
    events.insert (event{event_type::left_edge, r.x, r.x, r.w, r.y, r.h});
    events.insert (event{event_type::right_edge, r.x + r.w, r.x, r.w, r.y, r.h});
  }

  void remove_region (region r)
  {
    events.erase (event{event_type::left_edge, r.x, r.x, r.w, r.y, r.h});
    events.erase (event{event_type::right_edge, r.x + r.w, r.x, r.w, r.y, r.h});
  }

  struct active
  {
    event_type type;
    int x, w, y, y0, h;

    friend bool operator<(active const& lhs, active const& rhs)
    {
      return lhs.y == rhs.y
        ?
        ( lhs.x == rhs.x
          ?
          ( lhs.y0 == rhs.y0
            ?
            ( lhs.type == rhs.type
              ?
              ( lhs.w == rhs.w
                ? lhs.h < rhs.h
                : lhs.w < rhs.w
              )
              : lhs.type < rhs.type
            )
            : lhs.y0 < rhs.y0
          )
          : lhs.x < rhs.x
        )
        : lhs.y < rhs.y;
    }
    friend bool operator==(active const& left, active const& right)
    {
      // return left.x == right.x && left.type == right.type
      //   && left.y == right.y && left.y0 == right.y0 && left.w == right.w
      //   && left.h == right.h;
      return !(left < right) && !(right < left);
    }
    friend std::ostream& operator<<(std::ostream& os, active const& a)
    {
      return os << "[type: " << a.type << " x: " << a.x << " w: " << a.w << " y: " << a.y << " 'y0': " << a.y0 << " h: " << a.h << "]";
    }
  };
  
  std::vector<region> non_overlapping_regions () const
  {
     std::vector<region> r;
     std::vector<region> realized_areas;
     std::vector<active> x_actives;
     std::vector<active> y_actives;
     //std::cout << "number of events " << events.size() << std::endl;
     for (auto&& e : events)
     {
       y_actives.clear();

       //std::cout << "testing event " << e << std::endl;
       if (e.type == event_type::left_edge)
       {
         // we can check for intersections on this y-line sweep
         auto left = active {event_type::left_edge, e.x, e.w, e.y, e.y, e.h};
         auto right = active {event_type::right_edge, e.x, e.w, e.y + e.h, e.y, e.h};

         auto left_insertion = std::upper_bound (x_actives.begin(), x_actives.end(), left);

         //std::cout << "elements before " << std::distance(x_actives.begin(), left_insertion) << " " << left << std::endl;


         //std::cout << "add to x_actives" << std::endl;

         x_actives.insert (left_insertion, left);

         auto right_insertion = std::upper_bound (x_actives.begin(), x_actives.end(), right);

         x_actives.insert (right_insertion, right);
       }
       else
       {
         // remove active
         active const a{event_type::left_edge, e.x0, e.w, e.y, e.y, e.h};
         x_actives.erase (std::lower_bound(x_actives.begin(), x_actives.end(), a));

         std::cout << "found event for right_edge " << e << std::endl;
         auto top = active {event_type::left_edge, e.x, e.w, e.y, e.y, e.h};
         auto bottom = active {event_type::right_edge, e.x, e.w, e.y + e.h, e.y, e.h};
         auto top_insertion = std::lower_bound (x_actives.begin(), x_actives.end(), top);
         auto bottom_insertion = std::lower_bound (x_actives.begin(), x_actives.end(), bottom);

         for (auto current = x_actives.begin()
                ;current != top_insertion; ++current)
         {
           if (current->type == event_type::left_edge)
           {
             //std::cout << "adding" << std::endl;
             y_actives.push_back (*current);
           }
           else
           {
             active const a{event_type::left_edge, current->x, current->w, current->y0, current->y0, current->h};
             //std::cout << "removing " << a << std::endl;
             y_actives.erase (std::remove (y_actives.begin(), y_actives.end(), a), y_actives.end());
           }
         }
         for (auto current = std::next(top_insertion)
                ;current != bottom_insertion; ++current)
         {
           if (current->type == event_type::left_edge)
           {
             //std::cout << "intersection starts after on y " << *current << std::endl;
             y_actives.push_back (*current);
           }
           // else
           // {
           //   active const a{event_type::left_edge, current->x, current->w, current->y0, current->y0, current->h};
           //   std::cout << "intersection finishes before on y " << a << std::endl;
           //   //y_actives.erase (std::remove (y_actives.begin(), y_actives.end(), a), y_actives.end());
           // }
         }

         // y_actives has all current intersections (?)
         // N intersections
         std::set<region> regions;
         for (auto&& i : y_actives)
         {
           int const x = std::max (i.x, e.x0)
             , y = std::max (i.y, e.y)
             , w = e.x - x
             , h = std::min (i.y + i.h, e.y + e.h) - y;

           //std::cout << "intersection is " << region{x, y, w, h} << std::endl;

           /* o que fazer na intersecção */

           regions.insert({x, y, w, h});
         }

       }


       //std::cout << "y_actives size " << y_actives.size() << std::endl;
     }

     return {};
  }
  
};
    
} }

#endif
