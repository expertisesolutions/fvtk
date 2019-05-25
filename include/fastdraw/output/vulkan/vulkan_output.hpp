///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 Felipe Magno de Almeida.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// See http://www.boost.org/libs/foreach for documentation
//

#ifndef FASTDRAW_OUTPUT_VULKAN_OUTPUT_HPP
#define FASTDRAW_OUTPUT_VULKAN_OUTPUT_HPP

#include <fastdraw/output/x11/x11_base.hpp>
#include <fastdraw/output/vulkan/vulkan_output_info.hpp>
#include <fastdraw/output/vulkan/vulkan_draw_info.hpp>
#include <fastdraw/output/vulkan/add_triangle.hpp>
#include <fastdraw/output/vulkan/add_box.hpp>
#include <fastdraw/output/vulkan/add_text.hpp>
#include <fastdraw/scene.hpp>

#include <vector>
#include <iostream>

namespace fastdraw { namespace output { namespace vulkan {

template <typename Coord, typename Point, typename Color, typename WindowingBase = x11_base>
struct vulkan_output : vulkan_output_info<WindowingBase>
{
  typedef fastdraw::object::object_variant<Coord, Point, Color> object_type;

  struct scene_object_output
  {
    //object_type object;
    std::size_t index;
    std::vector<vulkan_draw_info> draw_infos;
    
  };
  
  std::vector<scene_object_output> object_outputs;
};

template <typename Coord, typename Point, typename Color, typename WindowingBase>
struct create_output_specific_object_visitor
{
  typedef vulkan_draw_info result_type;

  std::size_t index;
  vulkan_output<Coord, Point, Color, WindowingBase>* output;
  
  vulkan_draw_info operator()(object::fill_triangle<Point, Color> const& triangle)
  {
    auto info = vulkan::create_output_specific_object (*output, triangle);
    output->object_outputs.push_back({index, {info}});
    return info;
  }
  vulkan_draw_info operator()(object::fill_text<Point, std::string, Color> const& text)
  {
    auto info = vulkan::create_output_specific_object (*output, text);
    output->object_outputs.push_back({index, {info}});
    return info;
  }
  vulkan_draw_info operator()(object::fill_box<Point, Color> const& box)
  {
    auto info = vulkan::create_output_specific_object (*output, box);
    output->object_outputs.push_back({index, {info}});
    return info;
  }
};

template <typename Coord, typename Point, typename Color, typename WindowingBase>
struct replace_visitor
{
  typedef vulkan_draw_info result_type;

  std::size_t index;
  vulkan_output<Coord, Point, Color, WindowingBase>* output;
  
  vulkan_draw_info operator()(object::fill_triangle<Point, Color> const& triangle)
  {
    assert(output->object_outputs.size() > index);
    assert(!output->object_outputs[index].draw_infos.empty());
    return vulkan::replace_push_constants (output->object_outputs[index].draw_infos[0], triangle);
  }
  vulkan_draw_info operator()(object::fill_text<Point, std::string, Color> const& text)
  {
    // assert(output->object_outputs.size() > index);
    // assert(!output->object_outputs[index].draw_infos.empty());
    // return vulkan::replace_push_constants (output->object_outputs[index].draw_infos[0], triangle);
    throw -1.0;
  }
  vulkan_draw_info operator()(object::fill_box<Point, Color> const& box)
  {
    //return vulkan::create_output_specific_object (*output, box);
    throw -1;
    return {};
  }
};
    
template <typename Coord, typename Point, typename Color, template <typename> class Container, typename WindowingBase>
vulkan::vulkan_output<Coord, Point, Color, WindowingBase>
output (fastdraw::scene<Coord, Point, Color, Container>& scene
        , fastdraw::scene_difference<Coord, Point, Color, Container>& scene_diff
        , vulkan::vulkan_output<Coord, Point, Color, WindowingBase>& output)
{
  typedef vulkan::vulkan_output<Coord, Point, Color, WindowingBase> output_type;
  typedef fastdraw::scene_difference<Coord, Point, Color, Container> scene_diff_type;
  using insert = typename scene_diff_type::insert;
  using replace = typename scene_diff_type::replace;
  using remove = typename scene_diff_type::remove;
  output_type diff_output;
      
  for (auto&& operation : scene_diff.operations)
  {
    //std::cout << "operation" << std::endl;
    if (insert* op = std::get_if<insert>(&operation))
    {
      // should we deal with what? z-index? what else?
      std::cout << "pushed added output object" << std::endl;
      
      diff_output.object_outputs.push_back
        ({op->index
          , {std::visit(create_output_specific_object_visitor<Coord, Point, Color, WindowingBase>{op->index, &output}, op->object.object)}});

    }
    else if (replace* op = std::get_if<replace>(&operation))
    {
      diff_output.object_outputs.push_back
        ({op->index
          , {std::visit(replace_visitor<Coord, Point, Color, WindowingBase>{op->index, &output}, op->object.object)}});
    }
    else if (remove* op = std::get_if<remove>(&operation))
    {
      std::cout << "remove op" << std::endl;
    }
    else
    {
      std::cout << "what?" << std::endl;
    }
  }
  return diff_output;
}

} } }
  

#endif
