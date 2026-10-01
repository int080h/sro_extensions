#pragma once

#include "sdk/types/halign_type.hpp"
#include "sdk/types/valign_type.hpp"

#include "utils/msvc9_stl.hpp"

#include <cstdint>

#include <d3d9.h>



class cng_font_texture {

public:

  using text_char_list = std::n_list<std::pair<wchar_t, void*> >;



  auto get_pFontData() -> void*;
  auto get_dwBGColor() -> std::uint32_t;
  auto get_dwFGColor() -> std::uint32_t;
  auto get_listTextChar() -> text_char_list;
  auto set_pFontData(void* val) -> void;
  auto set_dwBGColor(std::uint32_t val) -> void;
  auto set_dwFGColor(std::uint32_t val) -> void;
  auto set_listTextChar(text_char_list val) -> void;

};



class cnif_text_board {

public:

  auto is_cAniFadeAlphaStart() -> bool;
  auto set_cAniFadeAlphaStart(bool val) -> void;
  auto is_btCurrentTextureAlpha() -> bool;
  auto set_btCurrentTextureAlpha(bool val) -> void;
  auto is_cAniFadeAlphaMax() -> bool;
  auto set_cAniFadeAlphaMax(bool val) -> void;
  auto is_bRenderTexture() -> bool;
  auto set_bRenderTexture(bool val) -> void;
  auto get_nTextureHAlignType() -> halign_type;
  auto get_nTextureVAlignType() -> valign_type;
  auto get_NFontTexture() -> cng_font_texture;
  auto get_wstrFontTexture() -> std::n_wstring;
  auto get_dwUnknwonColor() -> std::uint32_t;
  auto get_dwBG_FontColor() -> std::uint32_t;
  auto get_pFontTextData() -> void*;
  auto get_dwTextureColor() -> std::uint32_t;
  auto get_padding_af() -> std::uint8_t;
  auto get_fAniFadeTime() -> float;
  auto get_fAniFadeCurrentTime() -> float;
  auto get_pBgTexture3D() -> IDirect3DBaseTexture9*;
  auto get_pHoverTexture3D() -> IDirect3DBaseTexture9*;
  auto get_strFocusTexturePath() -> std::n_string;
  auto get_pRenderTexture() -> IDirect3DBaseTexture9*;
  auto get_strBGroundTexturePath() -> std::n_string;
  auto get_dwReleaseTexture() -> std::uint32_t;
  auto set_nTextureHAlignType(halign_type val) -> void;
  auto set_nTextureVAlignType(valign_type val) -> void;
  auto set_NFontTexture(cng_font_texture val) -> void;
  auto set_wstrFontTexture(std::n_wstring val) -> void;
  auto set_dwUnknwonColor(std::uint32_t val) -> void;
  auto set_dwBG_FontColor(std::uint32_t val) -> void;
  auto set_pFontTextData(void* val) -> void;
  auto set_dwTextureColor(std::uint32_t val) -> void;
  auto set_padding_af(std::uint8_t val) -> void;
  auto set_fAniFadeTime(float val) -> void;
  auto set_fAniFadeCurrentTime(float val) -> void;
  auto set_pBgTexture3D(IDirect3DBaseTexture9* val) -> void;
  auto set_pHoverTexture3D(IDirect3DBaseTexture9* val) -> void;
  auto set_strFocusTexturePath(std::n_string val) -> void;
  auto set_pRenderTexture(IDirect3DBaseTexture9* val) -> void;
  auto set_strBGroundTexturePath(std::n_string val) -> void;
  auto set_dwReleaseTexture(std::uint32_t val) -> void;

};

