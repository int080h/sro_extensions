#pragma once

#include "cgwnd.hpp"

#include <cstddef>
#include <cstdint>

class cif_wnd;

struct cif_wnd_vtable {
  VFN_CDECL(get_res, void*);
  VFN_THISCALL(on_timer, void, cif_wnd* self, float delta);
  CGWND_VTABLE_COMMON(cif_wnd, on_timer_id)
};

// CIFWnd: base in-game frame (CIFWnd::ctor @ 0x730F50). Derived IF controls begin at +0x394.
class cif_wnd : public cgwnd {
public:
  PAD_TO(ext_client::offsets::cgwnd::size, ext_client::offsets::cif_wnd::size);

  DECLARE_SDK_VTABLE(cif_wnd_vtable, wnd_vftable)
  DECLARE_SDK_WND_CAST(cif_wnd)
};

static_assert(sizeof(cif_wnd) == ext_client::offsets::cif_wnd::size, "cif_wnd size mismatch");
