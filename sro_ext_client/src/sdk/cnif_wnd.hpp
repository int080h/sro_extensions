#pragma once

#include "cgwnd.hpp"
#include "cnif_textboard.hpp"
#include "utils/msvc9_stl.hpp"
#include "utils/offsets.hpp"

#include <cstdint>
#include <windows.h>
#include <d3d9.h>

struct D3DXVECTOR2 {
  float x;
  float y;
};

class cnif_wnd;

struct cnif_wnd_vtable {
  VFN_CDECL(get_res, void*);
  VFN_THISCALL(on_timer, void, cnif_wnd* self, float delta);
  CGWND_VTABLE_COMMON(cnif_wnd, on_timer_id)
};

// CNIFWnd multiple inheritance layout.
// In MSVC 32-bit:
// - cgwnd starts at offset 0x00, size 0x84
// - cnif_textboard starts at offset 0x84, size 0x104
// - cnif_wnd fields start at offset 0x188
class cnif_wnd : public cgwnd, public cnif_textboard {
public:
  bool m_bMineAlphaScaleUse;                                  // +0x188
  std::uint8_t padding_189[3];                                // +0x189
  std::uint32_t m_dwIsAniFadeIn;                              // +0x18C
  std::uint32_t m_dwStyleOptionBit;                           // +0x190
  RECT m_rcTextTexturePos;                                    // +0x194
  std::n_wstring m_wstrInnerText;                             // +0x1A4
  std::uint32_t m_dwEnableMouseActions;                       // +0x1C0
  std::uint32_t m_dwTextureBGColor;                           // +0x1C4
  std::uint32_t m_dwTextureFGColor;                           // +0x1C8
  float m_fRenderColorRight;                                  // +0x1CC
  float m_fRenderColorDown;                                   // +0x1D0
  std::uint32_t m_dwTooltipValignTop;                         // +0x1D4
  std::n_wstring m_wstrTooltipText;                           // +0x1D8
  bool m_blWndDragMode;                                       // +0x1F4
  std::uint8_t padding_1f5[3];                                // +0x1F5
  cnif_wnd* m_pParentOwner;                                   // +0x1F8
  std::n_string m_strParentsId;                               // +0x1FC
  std::uint32_t m_cdwParentsId;                               // +0x218
  D3DXVECTOR2 m_vTextureUV[4];                                // +0x21C
  D3DVECTOR m_vTexturedRenderCords[8];                        // +0x23C
  D3DVECTOR m_vUnkCords_1;                                    // +0x29C
  D3DVECTOR m_vUnkCords_2;                                    // +0x2A8
  D3DVECTOR m_vTextureColorRenderCords[8];                    // +0x2B4
  D3DVECTOR m_vUnkCords_3;                                    // +0x314
  char pad_0320[12];                                          // +0x320
  std::n_vector<cnif_wnd*> m_vecEdgesTextureHelper;            // +0x32C
  bool m_bEdgesHelperVisibleState;                            // +0x33C
  std::uint8_t padding_33d[3];                                // +0x33D
  std::uint32_t m_dwWndType;                                  // +0x340
  std::uint32_t field_0344;                                   // +0x344
  std::uint32_t field_0348;                                   // +0x348

  auto wnd_vftable() -> cnif_wnd_vtable* {
    return reinterpret_cast<cnif_wnd_vtable*>(cgwnd::vftable);
  }
  auto wnd_vftable() const -> const cnif_wnd_vtable* {
    return reinterpret_cast<const cnif_wnd_vtable*>(cgwnd::vftable);
  }
  auto board_vftable() -> cnif_textboard_vtable* {
    return reinterpret_cast<cnif_textboard_vtable*>(cnif_textboard::vftable);
  }
  auto board_vftable() const -> const cnif_textboard_vtable* {
    return reinterpret_cast<const cnif_textboard_vtable*>(cnif_textboard::vftable);
  }

  DECLARE_SDK_WND_CAST(cnif_wnd)
};

