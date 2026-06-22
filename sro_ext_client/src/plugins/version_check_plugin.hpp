#pragma once

#include <Windows.h>
#include <d3d9.h>
#include <gdiplus.h>
#include <imgui.h>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "core/core_event_manager.hpp"
#include "core/core_config.hpp"
#include "utils/msvc9_stl.hpp"
#include "sdk/cps_version_check.hpp"
#include "sdk/cif_static.hpp"

namespace ext_client::plugins::version_check {

  // =========================================================================
  // 1. com_ptr Utility Class
  // =========================================================================
  template<typename T>
  class com_ptr {
    T* m_ptr = nullptr;

  public:
    com_ptr() = default;
    explicit com_ptr(T* ptr)
      : m_ptr(ptr) {}
    ~com_ptr() { reset(); }

    com_ptr(const com_ptr&) = delete;
    com_ptr& operator=(const com_ptr&) = delete;

    com_ptr(com_ptr&& other) noexcept
      : m_ptr(other.m_ptr) {
      other.m_ptr = nullptr;
    }
    com_ptr& operator=(com_ptr&& other) noexcept {
      if (this != &other) {
        reset();
        m_ptr = other.m_ptr;
        other.m_ptr = nullptr;
      }
      return *this;
    }

    auto get() const -> T* { return m_ptr; }
    auto operator->() const -> T* { return m_ptr; }
    auto operator&() -> T** { return &m_ptr; }
    operator T*() const { return m_ptr; }

    auto reset(T* ptr = nullptr) -> void {
      if (m_ptr) {
        m_ptr->Release();
      }
      m_ptr = ptr;
    }

    auto release() -> T* {
      T* temp = m_ptr;
      m_ptr = nullptr;
      return temp;
    }
  };

  // =========================================================================
  // 2. Constants & Enums
  // =========================================================================
  inline constexpr std::uint32_t k_cif_static_loading_banner_vftable = ext_client::offsets::cif_static::vtable::loading_banner;
  inline constexpr std::uint32_t k_loading_banner_descriptor = 0x01179A58;
  inline constexpr int k_max_banner_frames = 32;

  enum class intro_render_stage : int {
    d3d_setup = 1,
    wire_stages = 2,
  };

  // =========================================================================
  // 3. Structures
  // =========================================================================
  struct loading_banner_state {
    char path[256]{};
    std::uint32_t widget_vftable = 0;
    std::uint32_t image_vftable = 0;
    void* texture = nullptr;
    bool path_read = false;
  };

  struct intro_renderer_state {
    void* vftable;
  };

  // =========================================================================
  // 4. Globals (Extern)
  // =========================================================================
  extern std::uint32_t g_last_banner_switch_time;
  extern int g_current_banner_index;
  extern cif_static* g_banner_widget;

  extern cif_static* g_banner_frames[k_max_banner_frames];
  extern int g_banner_frame_count;
  extern Gdiplus::Bitmap* g_overlay_bitmaps[k_max_banner_frames];
  extern int g_overlay_bitmap_count;

  extern ULONG_PTR g_version_check_gdiplus_token;

  extern LONG g_orig_style;
  extern LONG g_orig_ex_style;
  extern bool g_style_saved;
  extern RECT g_orig_rect;

  extern int g_target_width;
  extern int g_target_height;
  extern bool g_target_saved;

  extern bool g_intro_render_pipeline_ready;

  extern IDirect3DDevice9* g_active_d3d_device;
  extern std::vector<loading_banner_state> g_loading_banners;
  extern int g_loading_banner_count;
  extern intro_renderer_state* g_intro_renderer;

  extern std::uint32_t g_active_d3d_device_reset_conn;

  // =========================================================================
  // 5. Helper Function Declarations
  // =========================================================================
  auto control_panel() -> ext_client::core::config::core_config::version_check_t&;
  auto is_version_check_active_process() -> bool;
  auto find_loading_banner_widget(cps_version_check* self) -> cif_static*;
  auto resolve_loading_hwnd() -> HWND;
  auto ensure_minimize_button(HWND hwnd) -> void;
  auto query_d3d_texture(void* candidate) -> IDirect3DTexture9*;
  auto find_d3d_texture_in_resource(void* resource) -> IDirect3DTexture9*;
  auto load_ddj_as_gdi_bitmap(const char* path) -> Gdiplus::Bitmap*;
  auto load_banner_overlay_images() -> void;
  auto free_banner_overlays() -> void;
  auto load_all_banners_config() -> void;
  auto free_all_banners_config() -> void;
  auto update_loading_banner_texture(cps_version_check* self) -> void;
  auto draw_overlay_on_device(IDirect3DDevice9* device) -> void;
  auto apply_loading_window_styles(HWND hwnd) -> void;
  auto restore_loading_window_styles(HWND hwnd) -> void;
  auto start_overlay_for_window(HWND hwnd) -> void;
  auto restore_window_style() -> void;
  auto update_banner_cycle(cps_version_check* self) -> void;
  auto banner_frame_at(int idx) -> cif_static*;
  auto setup_login_render_pipeline() -> void;
  auto shutdown_gdiplus() -> void;

  // =========================================================================
  // 6. Named Event Handlers
  // =========================================================================
  auto handle_version_check_create(void* raw_ctx) -> void;
  auto handle_version_check_update(void* raw_ctx) -> void;
  auto handle_set_child_process(void* raw_ctx) -> void;
  auto handle_load_intro_camera(void* raw_ctx) -> void;
  auto handle_shutdown(void* raw_ctx) -> void;
  auto handle_menu(void* raw_ctx) -> void;
  auto handle_tick(void* raw_ctx) -> void;

} // namespace ext_client::plugins::version_check
