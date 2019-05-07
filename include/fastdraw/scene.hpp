///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 Felipe Magno de Almeida.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// See http://www.boost.org/libs/foreach for documentation
//

#ifndef FASTDRAWCXX_SCENE_HH
#define FASTDRAWCXX_SCENE_HH

#include <fastdraw/object/object_variant.hpp>

#include <vector>
#include <iostream>
#include <functional>
#include <cassert>

namespace fastdraw {

template <typename Coord, typename Point, typename Color, template <typename> class Container = std::vector>
struct scene
{
  typedef Coord coord_type;
  typedef Point point_type;
  typedef Color color_type;
  typedef fastdraw::object::object_variant<Coord, Point, Color> object_type;

  typedef Container<object_type> object_container_type;
  typedef typename object_container_type::const_iterator iterator;
  typedef typename object_container_type::const_iterator const_iterator;

  struct scene_object
  {
    object_type object;
  };

  object_container_type objects;
};

template <typename Coord, typename Point, typename Color, template <typename> class Container = std::vector>
struct scene_difference
{
  typedef scene<Coord, Point, Color, Container> scene_type;
  typedef typename scene_type::object_type object_type;

  struct replace
  {
    std::size_t index;
    object_type object;
  };
  struct insert
  {
    std::size_t index;
    object_type object;
  };
  struct remove
  {
    std::size_t index;
  };

  void clear()
  {
    operations.clear();
  }

  typedef std::variant<remove, replace, insert> operations_type;
  Container<operations_type> operations;
};

template <typename Coord, typename Point, typename Color, template <typename> class Container
          , typename...Objects>
void push_back (fastdraw::scene<Coord, Point, Color, Container> const& scene
                , fastdraw::scene_difference<Coord, Point, Color, Container>& diff
                , Objects...draw_objects)
{
  typedef fastdraw::scene_difference<Coord, Point, Color, Container> scene_diff_type;
  typedef typename scene_diff_type::insert insert;
  std::size_t index = scene.objects.size() + diff.operations.size();
  (((std::cout << "push draw\n"), diff.operations.push_back(insert{index++, std::forward<Objects>(draw_objects)})), ...);
}

template <typename Coord, typename Point, typename Color, template <typename> class Container
          , typename Object>
void replace (fastdraw::scene<Coord, Point, Color, Container> const& scene
              , fastdraw::scene_difference<Coord, Point, Color, Container>& diff
              , std::size_t index
              , Object draw_object)
{
  typedef fastdraw::scene_difference<Coord, Point, Color, Container> scene_diff_type;
  typedef typename scene_diff_type::replace replace;
  diff.operations.push_back(replace{index, std::forward<Object>(draw_object)});
}

template <typename Coord, typename Point, typename Color, template <typename> class Container>
struct merge_scene_difference_visitor
{
  using coord_type = Coord;
  using point_type = Point;
  using color_type = Color;
  using scene_type = fastdraw::scene<Coord, Point, Color, Container>;
  using scene_difference = fastdraw::scene_difference<Coord, Point, Color, Container>;

  using insert = typename scene_difference::insert;
  using remove = typename scene_difference::remove;
  using replace = typename scene_difference::replace;

  scene_type* scene;

  void operator()(insert const& op) const
  {
    std::cout << "adding to scene" << std::endl;
    assert (op.index <= scene->objects.size());
    if (op.index == scene->objects.size())
    {
      scene->objects.push_back (std::move(op.object));
    }
    else
    {
      scene->objects.insert (std::next(scene->objects.begin(), op.index), std::move(op.object));
    }
  }

  void operator()(remove const& op) const
  {
    std::cout << "remove" << std::endl;
    scene->objects.erase (std::next(scene->objects.begin(), op.index));
  }

  void operator()(replace const& op) const
  {
    // std::cout << "merge replace" << std::endl;
    scene->objects[op.index] = std::move(op.object);
  }
};
  
template <typename Coord, typename Point, typename Color, template <typename> class Container>
void merge_scene_difference (fastdraw::scene<Coord, Point, Color, Container>& scene

                             , fastdraw::scene_difference<Coord, Point, Color, Container> const& diff)
{
  for (auto&& op : diff.operations)
  {
    std::visit(merge_scene_difference_visitor<Coord, Point, Color, Container>{&scene}, op);
  }
}
  
}

#endif
