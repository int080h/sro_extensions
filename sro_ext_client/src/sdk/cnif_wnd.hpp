#pragma once

#include "cgwnd.hpp"
#include "cnif_textboard.hpp"

#include <cstddef>
#include <cstdint>

class cnif_wnd;

struct cnif_wnd_vtable {
  VFN_CDECL(get_res, void*);
  VFN_THISCALL(on_timer, void, cnif_wnd* self, float delta);
  CGWND_VTABLE_COMMON(cnif_wnd, on_timer_id)
};

class cnif_wnd : public cgwnd {
public:
  cnif_textboard_vtable* m_textboard_vftable;
  PAD_TO(ext_client::offsets::cnif_wnd::fields::textboard_vftable + sizeof(void*), ext_client::offsets::cnif_wnd::size);

  DECLARE_SDK_VTABLE(cnif_wnd_vtable, wnd_vftable)
  DECLARE_SDK_WND_CAST(cnif_wnd)
};

static_assert(sizeof(cnif_wnd) == ext_client::offsets::cnif_wnd::size, "cnif_wnd size mismatch");
static_assert(offsetof(cnif_wnd, m_textboard_vftable) == ext_client::offsets::cnif_wnd::fields::textboard_vftable);
