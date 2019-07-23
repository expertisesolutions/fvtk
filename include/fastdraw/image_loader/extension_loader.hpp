///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 Felipe Magno de Almeida.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// See http://www.boost.org/libs/foreach for documentation
//

#ifndef FASTDRAW_IMAGE_LOADERS_EXTENSION_LOADER_HPP
#define FASTDRAW_IMAGE_LOADERS_EXTENSION_LOADER_HPP

#include <fastdraw/format.hpp>
#include <fastdraw/image_loader/jpeg.hpp>
#include <fastdraw/image_loader/png.hpp>

#include <filesystem>
#include <variant>

namespace fastdraw { namespace image_loader {

int32_t width (std::variant<jpeg, png> const& loader)
{
  return std::visit ([] (auto&& loader)
                     {
                       return loader.width();
                     }, loader);
}

int32_t height (std::variant<jpeg, png> const& loader)
{
  return std::visit ([] (auto&& loader)
                     {
                       return loader.height();
                     }, loader);
}

fastdraw::buffer_format format (std::variant<jpeg, png> const& loader)
{
  return std::visit ([] (auto&& loader)
                     {
                       return loader.format();
                     }, loader);
}

std::uint32_t stride (std::variant<jpeg, png> const& loader)
{
  return std::visit ([] (auto&& loader)
                     {
                       return loader.stride();
                     }, loader);
}

void write_to (std::variant<jpeg, png>& loader, char* buffer, std::size_t size)
{
  std::visit ([&] (auto&& loader)
              {
                return loader.write_to(buffer, size);
              }, loader);
}
    
struct extension_loader
{
  std::variant<jpeg, png> load (std::filesystem::path path) const
  {
    std::cout << "loading extension " << path.extension() << std::endl;
    if (path.extension() == ".png")
    {
      std::cout << "loading with png" << std::endl;
      return png {path};
    }
    else
      return jpeg {path};
  }
};

} }

#endif
