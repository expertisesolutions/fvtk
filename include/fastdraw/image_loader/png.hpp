///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 Felipe Magno de Almeida.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// See http://www.boost.org/libs/foreach for documentation
//

#ifndef FASTDRAW_IMAGE_LOADERS_PNG_HPP
#define FASTDRAW_IMAGE_LOADERS_PNG_HPP

#include <fastdraw/format.hpp>

#include <filesystem>

#include <png.h>

namespace fastdraw { namespace image_loader {

struct png
{
  png_structp png_ptr;
  png_infop info_ptr;

  struct file_deleter
  {
    void operator()(FILE* fp) const { if (fp) fclose(fp); }
  };
  
  std::unique_ptr<FILE, file_deleter> fp;
  
  png (std::filesystem::path path)
  {
    std::cout << "created png this " << this << std::endl;
    fp.reset (std::fopen (path.c_str(), "rb"));
    if (!fp)
      throw -1;

    png_byte header[8];

    fread (header, 1, 8, fp.get());
    if ( ::png_sig_cmp (header, 0, 8))
      throw -1;

    png_ptr = ::png_create_read_struct (PNG_LIBPNG_VER_STRING, this, nullptr, nullptr);
    if (!png_ptr)
      throw -1;

    info_ptr = ::png_create_info_struct (png_ptr);
    if (!info_ptr)
      throw -1;

    if (setjmp ( png_jmpbuf (png_ptr)))
      throw -1;

    ::png_init_io (png_ptr, fp.get());
    ::png_set_sig_bytes (png_ptr, 8);

    ::png_read_info (png_ptr, info_ptr);

    png_uint_32 width, height;
    int bit_depth, color_type;
    png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth,
                 &color_type, NULL, NULL, NULL);    

    if (color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_expand(png_ptr);
    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
        png_set_expand(png_ptr);
    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
        png_set_expand(png_ptr);

    if (bit_depth == 16)
      png_set_strip_16(png_ptr);
    if (color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
      png_set_gray_to_rgb(png_ptr);

    png_read_update_info(png_ptr, info_ptr);
  }

  ~png()
  {
    if (png_ptr)
    {
      std::cout << "destryoing " << this << std::endl;
      png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    }
  }

  png(png&& other)
    : png_ptr (std::move(other.png_ptr)), info_ptr (std::move(other.info_ptr))
    , fp (std::move(other.fp))
  {
    other.png_ptr = nullptr;
    other.info_ptr = nullptr;
  }

  int32_t width () const
  {
    return png_get_image_width (png_ptr, info_ptr);
  }

  int32_t height () const
  {
    return png_get_image_height (png_ptr, info_ptr);
  }

  fastdraw::buffer_format format () const
  {
    //auto bit_depth = static_cast<unsigned int>(png_get_bit_depth (png_ptr, info_ptr));
    auto color_type = png_get_color_type (png_ptr, info_ptr);
    switch (color_type)
    {
    case PNG_COLOR_TYPE_RGB:
      return buffer_format::r8g8b8;
    case PNG_COLOR_TYPE_RGBA:
      return buffer_format::r8g8b8a8;
    default:
      return buffer_format::unknown;
    };
  }

  std::uint32_t stride () const
  {
    return png_get_rowbytes (png_ptr, info_ptr);
  }

  void write_to (char* buffer, std::size_t size) const
  {
    std::cout << "reading this " << this << std::endl;
    png_byte** row_pointers = new png_byte*[height()];
    for (int32_t i = 0; i != height(); ++i)
    {
      if (png_ptr)
      {
        row_pointers[i] = static_cast<png_byte*>(static_cast<void*>(buffer)) + i * stride();
      }
      else
        row_pointers[i] = (i ? row_pointers[i - 1] : static_cast<png_byte*>(static_cast<void*>(buffer)));
    }
    ::png_read_image (png_ptr, row_pointers);
  }
};

struct png_loader
{
  png load (std::filesystem::path path) const
  {
    return {path};
  }
};
    
} }

#endif
