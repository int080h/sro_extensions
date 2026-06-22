#pragma once

#include "utils/msvc9_stl.hpp"
#include "utils/offsets.hpp"

#include <cstdint>
#include <d3d9.h>

enum ENUM_HALIGN_TYPE : int {
  HALIGN_LEFT = 0,
  HALIGN_CENTER = 1,
  HALIGN_RIGHT = 2,
};

enum ENUM_VALIGN_TYPE : int {
  VALIGN_TOP = 0,
  VALIGN_CENTER = 1,
  VALIGN_DOWN = 2,
};

struct cng_font_texture_vtable {
  VFN_THISCALL(scalar_deleting_dtor, void*, void* self, char should_free);
};

struct cng_font_texture {
  cng_font_texture_vtable* vftable;                  // +0x00
  void* m_pFontData;                                 // +0x04
  std::uint8_t pad_0008[4];                          // +0x08
  std::uint32_t m_dwBGColor;                         // +0x0C
  std::uint32_t m_dwFGColor;                         // +0x10
  std::uint8_t pad_0014[0x30];                       // +0x14
  std::n_list<std::pair<wchar_t, void*>> m_listTextChar; // +0x44
  struct {
    short width;
    short height;
  } m_BGroundDimensions;                             // +0x50
  std::uint8_t pad_0054[0x18];                       // +0x54
};

struct cnif_textboard_vtable {
  VFN_CDECL(get_res, void*);
  VFN_THISCALL(on_timer, void, void* self, float delta);
  VFN_THISCALL(scalar_deleting_dtor, void*, void* self, char should_free);
  VFN_THISCALL(on_post_ctor, void, void* self);
  VFN_THISCALL(equals, int, void* self, void* other);
  VFN_THISCALL(null_05, void, void* self);
  VFN_CDECL(create_instance, void*);
  VFN_THISCALL(on_create, int, void* self, int mode);
  VFN_THISCALL(on_destroy, int, void* self, int mode);
  VFN_THISCALL(on_init, int, void* self, int mode);
  VFN_THISCALL(on_attach, int, void* self);
  VFN_THISCALL(on_detach, int, void* self);
  VFN_THISCALL(on_cmd, int, void* self, int cmd);
  VFN_THISCALL(null_14, int, void* self);
  VFN_THISCALL(null_15, int, void* self);
  VFN_THISCALL(null_16, int, void* self);
  VFN_THISCALL(on_update, int, void* self);
};

class cnif_textboard {
public:
  cnif_textboard_vtable* vftable;                     // +0x00
  ENUM_HALIGN_TYPE m_nTextureHAlignType;              // +0x04
  ENUM_VALIGN_TYPE m_nTextureVAlignType;              // +0x08
  cng_font_texture m_NFontTexture;                    // +0x0C
  std::n_wstring m_wstrFontTexture;                   // +0x78
  std::uint32_t m_dwUnknwonColor;                     // +0x94
  std::uint32_t m_dwBG_FontColor;                     // +0x98
  void* m_pFontTextData;                              // +0x9C
  std::uint32_t m_dwTextureColor;                     // +0xA0
  std::uint8_t pad_00a4[8];                           // +0xA4
  std::uint8_t m_cAniFadeAlphaStart;                  // +0xAC
  std::uint8_t m_btCurrentTextureAlpha;               // +0xAD
  std::uint8_t m_cAniFadeAlphaMax;                    // +0xAE
  std::uint8_t padding_af;                            // +0xAF
  float m_fAniFadeTime;                               // +0xB0
  float m_fAniFadeCurrentTime;                        // +0xB4
  IDirect3DBaseTexture9* m_pBgTexture3D;              // +0xB8
  IDirect3DBaseTexture9* m_pHoverTexture3D;           // +0xBC
  std::n_string m_strFocusTexturePath;                // +0xC0
  IDirect3DBaseTexture9* m_pRenderTexture;            // +0xDC
  std::n_string m_strBGroundTexturePath;              // +0xE0
  std::uint32_t m_dwReleaseTexture;                   // +0xFC
  bool m_bRenderTexture;                              // +0x100
  std::uint8_t pad_101[3];                            // +0x101

  DECLARE_SDK_VTABLE(cnif_textboard_vtable, board_vftable)
};

