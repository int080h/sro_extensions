#pragma once

#include "sdk/game/cref_obj_item.hpp"
#include "sdk/game/cso_item.hpp"
#include "sdk/game/c_skill_manager.hpp"
#include "sdk/ui/cif_slot_with_help.hpp"

#include <d3d9.h>
#include <imgui.h>
#include <windows.h>
#include <cstdint>
#include <string>

namespace ext_client::render {

  // Texture loading and caching via native game engine (sub_4E59C0)
  auto get_texture(const std::string& ddj_path) -> IDirect3DTexture9*;
  auto get_sox_edge_texture() -> IDirect3DTexture9*;
  auto get_nasrun_edge_texture() -> IDirect3DTexture9*;
  auto clear_texture_cache() -> void;

  // Animation frame and spritesheet UV calculations (sub_760AC0 / sub_7415A0)
  // SoX: 256x128 texture, 32 frames (8x4 grid, 32x32 cells, 40ms/frame)
  // Nasrun: 512x32 texture, 16 frames (16x1 grid, 32x32 cells, 40ms/frame)
  inline auto calculate_sox_frame_at(std::uint32_t time_ms, std::uint32_t seed = 0) -> std::uint32_t {
    return ((time_ms / 40) + seed) % 32;
  }

  inline auto calculate_sox_frame(std::uint32_t seed = 0) -> std::uint32_t {
    return calculate_sox_frame_at(::GetTickCount(), seed);
  }

  inline auto calculate_nasrun_frame_at(std::uint32_t time_ms, std::uint32_t seed = 0) -> std::uint32_t {
    return ((time_ms / 40) + seed) % 16;
  }

  inline auto calculate_nasrun_frame(std::uint32_t seed = 0) -> std::uint32_t {
    return calculate_nasrun_frame_at(::GetTickCount(), seed);
  }

  inline auto get_sox_uvs(std::uint32_t frame_index, ImVec2& uv0, ImVec2& uv1) -> void {
    frame_index %= 32;
    const std::uint32_t col = frame_index % 8;
    const std::uint32_t row = frame_index / 8;

    // 256x128 texture, 8 cols (each 32px / 256px = 0.125f), 4 rows (each 32px / 128px = 0.25f)
    uv0.x = static_cast<float>(col) * 0.125f;
    uv0.y = static_cast<float>(row) * 0.25f;
    uv1.x = uv0.x + 0.125f;
    uv1.y = uv0.y + 0.25f;
  }

  inline auto get_nasrun_uvs(std::uint32_t frame_index, ImVec2& uv0, ImVec2& uv1) -> void {
    frame_index %= 16;

    // 512x32 texture, 16 cols (each 32px / 512px = 0.0625f), 1 row (32px / 32px = 1.0f)
    uv0.x = static_cast<float>(frame_index) * 0.0625f;
    uv0.y = 0.0f;
    uv1.x = uv0.x + 0.0625f;
    uv1.y = 1.0f;
  }

  // Render an item icon with optional animated SoX / Nasrun border overlay
  auto render_item_icon(IDirect3DTexture9* icon_tex,
                        bool is_sox,
                        bool is_devil,
                        const ImVec2& size = ImVec2(32.0f, 32.0f),
                        std::uint32_t seed = 0) -> void;

  // Render icon directly from cref_obj_item
  auto render_item_icon(const cref_obj_item* ref,
                        const ImVec2& size = ImVec2(32.0f, 32.0f),
                        bool show_sox = true,
                        std::uint32_t seed = 0) -> void;

  // Render interactive item slot with icon, animated border, opt level badge, and stack count
  auto render_item_slot(const char* str_id,
                        const cso_item* item,
                        const ImVec2& size = ImVec2(32.0f, 32.0f),
                        bool interactive = true) -> bool;

  auto render_item_slot_from_data(const char* str_id,
                                  const sdk::game::item_tooltip_data& data,
                                  const ImVec2& size = ImVec2(32.0f, 32.0f),
                                  bool interactive = true) -> bool;

  // Render an interactive skill slot with icon, level badge, cooldown overlay, and hover tooltip
  auto render_skill_slot(const char* str_id,
                         const ::c_skill_manager::s_skill_details& skill,
                         const ImVec2& size = ImVec2(36.0f, 36.0f),
                         bool interactive = true) -> bool;

  // Render rich Silkroad skill tooltip matching native UI layout
  auto render_skill_tooltip(const ::c_skill_manager::s_skill_details& skill) -> void;

  // Render empty skill slot matching native Silkroad recessed metallic frame
  auto render_empty_skill_slot(const ImVec2& size = ImVec2(36.0f, 36.0f)) -> void;

} // namespace ext_client::render
