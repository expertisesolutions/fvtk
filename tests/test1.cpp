///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2018 Felipe Magno de Almeida.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// See http://www.boost.org/libs/foreach for documentation
//

#include <iostream>
#include <fastdraw/renderer.hpp>

#include <variant>

struct line
{
  
};

template <typename Output>
void draw(line const& l, Output& out)
{
  std::cout << "draw line" << std::endl;
}

struct circle
{
  
};

template <typename Output>
void draw(circle const& l, Output& out)
{
  std::cout << "draw circle" << std::endl;
}

typedef std::variant<line, circle> objects;

template <typename Output>
void draw (objects& obj, Output& out)
{
  std::visit([&out] (auto&& v) { draw(v, out); }, obj);
}

double output;

int main()
{
  
  
  fastdraw::renderer<int, objects> renderer;
  renderer.render_objects.push_back(line{});

  render (renderer, output);

  
  
  return -1;
}

