///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 Felipe Magno de Almeida.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// See http://www.boost.org/libs/foreach for documentation
//

#ifndef FTK_FTK_UI_BACKEND_VULKAN_FWD_HPP
#define FTK_FTK_UI_BACKEND_VULKAN_FWD_HPP

#include <ftk/ui/backend/x11_base_fwd.hpp>
#include <ftk/ui/backend/uv/uv_loop_fwd.hpp>

namespace ftk { namespace ui { namespace backend {

template <typename Loop = uv::uv_loop, typename WindowingBase = x11_base<Loop> >
struct vulkan;
    
} } }

#endif
