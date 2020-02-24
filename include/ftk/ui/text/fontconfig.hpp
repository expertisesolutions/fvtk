///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2020 Felipe Magno de Almeida.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// See http://www.boost.org/libs/foreach for documentation
//

#ifndef FTK_FTK_TEXT_FONTCONFIG_HPP
#define FTK_FTK_TEXT_FONTCONFIG_HPP

#include <fontconfig/fontconfig.h>

namespace ftk { namespace ui {

struct fontconfig
{
  fontconfig ()
  {
    FcInit ();

    fc_config = FcInitLoadConfigAndFonts();
  }

  ~fontconfig ()
  {
    FcFini ();
  }

  std::string find_font ()
  {
    FcResult res;
    auto pattern = FcPatternBuild (nullptr, nullptr);
    FcPatternAddString (pattern, FC_FAMILY, static_cast<FcChar8 const*>(static_cast<const void*>("Monospace")));

    FcConfigSubstitute(fc_config, pattern, FcMatchPattern);
    FcDefaultSubstitute(pattern);

    /* do matching */
    auto set = FcFontSort(fc_config, pattern, FcTrue, NULL, &res);
    if (!set)
    {
      //FIXME add ERR log capability
      //ERR("No fontconfig font matches '%s'. It was the last resource, no font found!", fdesc->name);
      FcPatternDestroy(pattern);
      //p_nm = NULL;
    }
    else
    {
      for (int i = 0; i < set->nfont; i++)
      {
        FcValue filename;

        if (FcPatternGet(set->fonts[i], FC_FILE, 0, &filename) == FcResultMatch)
          {
             // if (font)
             //   evas_common_font_add((RGBA_Font *)font, (char *)filename.u.s, size, wanted_rend, bitmap_scalable);
             // else
             //   font = (Evas_Font_Set *)evas_common_font_load((char *)filename.u.s, size, wanted_rend, bitmap_scalable);
            std::cout << "filename font " << static_cast<const char*>(static_cast<const void*>(filename.u.s)) << std::endl;
            return static_cast<const char*>(static_cast<const void*>(filename.u.s));
          }
      }
    }
    return {};
  }

  fontconfig(fontconfig&& other) = default;
  fontconfig& operator=(fontconfig&& other) = default;
  fontconfig(fontconfig const& other) = delete;
  fontconfig& operator=(fontconfig const& other) = delete;

  FcConfig* fc_config;
};    

} }

#endif
