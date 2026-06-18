#pragma once

#include "cgwnd.hpp"
#include "ctextboard.hpp"
#include "utils/layout.hpp"
#include "utils/msvc9_stl.hpp"
#include "utils/offsets.hpp"

#include <cstddef>
#include <cstdint>

class cif_wnd;

struct cif_wnd_vtable {
  VFN_CDECL(get_res, void*);
  VFN_THISCALL(on_timer, void, cif_wnd* self, float delta);
  CGWND_VTABLE_COMMON(cif_wnd, on_timer_id)
};

// CIFWnd — CGWnd + CTextBoard @ +0x84, CResIDManager @ +0x1C4. Derived IF controls @ +0x394.
class cif_wnd : public cgwnd {
public:
  ctextboard_vtable* m_textboard_vftable;
  PAD_TO(ext_client::offsets::cif_wnd::fields::textboard_vftable + sizeof(void*),
         ext_client::offsets::cif_wnd::fields::ui_res_map);
  PAD_BYTES(m_ui_res_map, ext_client::msvc9::ui_res_map_size);
  PAD_TO(ext_client::offsets::cif_wnd::fields::ui_res_map + ext_client::msvc9::ui_res_map_size,
         ext_client::offsets::cif_wnd::size);

  DECLARE_SDK_VTABLE(cif_wnd_vtable, wnd_vftable)
  DECLARE_SDK_WND_CAST(cif_wnd)

  auto ui_res_map() -> void* {
    return reinterpret_cast<std::uint8_t*>(this) + ext_client::offsets::cif_wnd::fields::ui_res_map;
  }

  auto ui_res_map() const -> const void* {
    return const_cast<cif_wnd*>(this)->ui_res_map();
  }

  auto textboard() -> ctextboard* {
    return reinterpret_cast<ctextboard*>(reinterpret_cast<std::uint8_t*>(this) +
                                         ext_client::offsets::cif_wnd::fields::textboard_vftable);
  }
};

static_assert(sizeof(cif_wnd) == ext_client::offsets::cif_wnd::size, "cif_wnd size mismatch");
static_assert(offsetof(cif_wnd, m_textboard_vftable) == ext_client::offsets::cif_wnd::fields::textboard_vftable);
static_assert(offsetof(cif_wnd, m_ui_res_map) == ext_client::offsets::cif_wnd::fields::ui_res_map);
