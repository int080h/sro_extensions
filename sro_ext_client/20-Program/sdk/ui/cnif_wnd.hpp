#pragma once

#include "sdk/ui/cgwnd.hpp"

#include "sdk/ui/cnif_text_board.hpp"

#include "utils/msvc9_stl.hpp"
#include "utils/vectorf.hpp"



#include <cstdint>

#include <windows.h>

#include <d3d9.h>



class cnif_wnd;



// CNIFWnd — cgwnd @ 0x00, cnif_text_board @ 0x84, derived fields @ 0x188.

class cnif_wnd : public cgwnd, public cnif_text_board {

public:

  auto is_bMineAlphaScaleUse() -> bool;
  auto set_bMineAlphaScaleUse(bool val) -> void;
  auto is_blWndDragMode() -> bool;
  auto set_blWndDragMode(bool val) -> void;
  auto is_bEdgesHelperVisibleState() -> bool;
  auto set_bEdgesHelperVisibleState(bool val) -> void;
  auto get_dwIsAniFadeIn() -> std::uint32_t;
  auto get_dwStyleOptionBit() -> std::uint32_t;
  auto get_rcTextTexturePos() -> RECT;
  auto get_wstrInnerText() -> std::n_wstring;
  auto get_dwEnableMouseActions() -> std::uint32_t;
  auto get_dwTextureBGColor() -> std::uint32_t;
  auto get_dwTextureFGColor() -> std::uint32_t;
  auto get_fRenderColorRight() -> float;
  auto get_fRenderColorDown() -> float;
  auto get_dwTooltipValignTop() -> std::uint32_t;
  auto get_wstrTooltipText() -> std::n_wstring;
  auto get_pParentOwner() -> cnif_wnd*;
  auto get_strParentsId() -> std::n_string;
  auto get_cdwParentsId() -> std::uint32_t;
  auto get_vUnkCords_1() -> vector3f;
  auto get_vUnkCords_2() -> vector3f;
  auto get_vUnkCords_3() -> vector3f;
  auto get_vecEdgesTextureHelper() -> std::n_vector<cnif_wnd*>;
  auto get_dwWndType() -> std::uint32_t;
  auto get_field_0344() -> std::uint32_t;
  auto get_field_0348() -> std::uint32_t;
  auto set_dwIsAniFadeIn(std::uint32_t val) -> void;
  auto set_dwStyleOptionBit(std::uint32_t val) -> void;
  auto set_rcTextTexturePos(RECT val) -> void;
  auto set_wstrInnerText(std::n_wstring val) -> void;
  auto set_dwEnableMouseActions(std::uint32_t val) -> void;
  auto set_dwTextureBGColor(std::uint32_t val) -> void;
  auto set_dwTextureFGColor(std::uint32_t val) -> void;
  auto set_fRenderColorRight(float val) -> void;
  auto set_fRenderColorDown(float val) -> void;
  auto set_dwTooltipValignTop(std::uint32_t val) -> void;
  auto set_wstrTooltipText(std::n_wstring val) -> void;
  auto set_pParentOwner(cnif_wnd* val) -> void;
  auto set_strParentsId(std::n_string val) -> void;
  auto set_cdwParentsId(std::uint32_t val) -> void;
  auto set_vUnkCords_1(vector3f val) -> void;
  auto set_vUnkCords_2(vector3f val) -> void;
  auto set_vUnkCords_3(vector3f val) -> void;
  auto set_vecEdgesTextureHelper(std::n_vector<cnif_wnd*> val) -> void;
  auto set_dwWndType(std::uint32_t val) -> void;
  auto set_field_0344(std::uint32_t val) -> void;
  auto set_field_0348(std::uint32_t val) -> void;

};

