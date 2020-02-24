///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 Felipe Magno de Almeida.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// See http://www.boost.org/libs/foreach for documentation
//

#ifndef FASTDRAW_IMAGE_LOADERS_JPEG_HPP
#define FASTDRAW_IMAGE_LOADERS_JPEG_HPP

#include <fastdraw/format.hpp>

#include <filesystem>
extern "C" {
#include <jpeglib.h>
#include <jerror.h>
}

#include <csetjmp>

namespace fastdraw { namespace image_loader {

struct jpeg
{
  mutable jpeg_decompress_struct cinfo;

  struct file_deleter
  {
    void operator()(FILE* fp) const { if (fp) { std::cout << "~fp " << fp << std::endl; fclose(fp); } }
  };
  
  mutable std::unique_ptr<FILE, file_deleter> fp;
  
  jpeg (std::filesystem::path path)
  {
    //std::cout << "jpeg this " << this << " cinfo " << cinfo << std::endl;
    struct jpeg_error_mgr jerr;
    jmp_buf setjmp_buffer;
    cinfo.err = jpeg_std_error(&jerr);

    if (setjmp (setjmp_buffer))
    {
      std::cout << "some error!" << std::endl;
      jpeg_destroy_decompress(&cinfo);
      if (!!fp)
        fp.reset();
      throw -1;
    }

    jpeg_create_decompress(&cinfo);

    fp.reset (std::fopen (path.c_str(), "rb"));
    if (!fp)
      throw -1;
    std::cout << "fp " << fp.get() << std::endl;
    jpeg_stdio_src(&cinfo, fp.get());

    jpeg_read_header(&cinfo, TRUE);

    jpeg_start_decompress(&cinfo);

// #ifdef JCS_EXTENSIONS
//     cinfo.out_color_space = JCS_EXT_RGBA;
// #endif
  }

  ~jpeg()
  {
    if (!!fp)
    {
      // jpeg_finish_decompress (&cinfo);
      // jpeg_destroy_decompress (&cinfo);
    }
  }

  jpeg(jpeg&& other)
    : cinfo (other.cinfo), fp (std::move(other.fp))
  {
  }

  int32_t width () const
  {
    return cinfo.output_width;
  }

  int32_t height () const
  {
    return cinfo.output_height;
  }

  fastdraw::buffer_format format () const
  {
// #ifndef JCS_EXTENSIONS
    if (cinfo.num_components == 3)
      return buffer_format::r8g8b8;
    else
      return buffer_format::r8g8b8a8;
// #else 
//     return buffer_format::r8g8b8a8;
// #endif
  }

  std::uint32_t stride () const
  {
    return width () * cinfo.num_components;
  }

  void write_to (char* buffer, std::size_t size) const
  {
    typedef unsigned int uint;
    while (cinfo.output_scanline != static_cast<uint>(height()))
    {
      ::jpeg_read_scanlines (&cinfo, (JSAMPARRAY)&buffer, 1);
      buffer += stride();
    }
  }
};

struct jpeg_loader
{
  jpeg load (std::filesystem::path path) const
  {
    return {path};
  }
};
    
} }

#endif
