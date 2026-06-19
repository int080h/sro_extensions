#pragma once

#include "utils/offsets.hpp"

// CGWndBase slice between cobj_child (+0x20) and CGWnd::control_id (+0x2C).
class cgwnd_base {
public:
  PAD(ext_client::offsets::cgwnd_base::fields::region_end - ext_client::offsets::cgwnd_base::fields::region_begin);
};
