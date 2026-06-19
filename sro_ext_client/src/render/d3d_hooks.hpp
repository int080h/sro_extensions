#pragma once

#include <Windows.h>
#include <d3d9.h>

#include <functional>

class cgfx_video3d;

namespace ext_client::render {

  struct device_callbacks {
    std::function<void(IDirect3DDevice9*)> on_device_created;
    std::function<void(IDirect3DDevice9*)> on_end_scene;
    std::function<void()> on_pre_reset;
    std::function<void()> on_post_reset;
  };

  auto resolve_hwnd(::cgfx_video3d* app) -> HWND;
  auto apply_presentation_params(D3DPRESENT_PARAMETERS* params) -> void;
  auto apply_behavior_flags(IDirect3D9* d3d, UINT adapter, D3DDEVTYPE device_type, DWORD& behavior_flags) -> void;

  class d3d_hooks {
  public:
    d3d_hooks() = default;
    ~d3d_hooks();

    d3d_hooks(const d3d_hooks&) = delete;
    d3d_hooks& operator=(const d3d_hooks&) = delete;
    d3d_hooks(d3d_hooks&&) = delete;
    d3d_hooks& operator=(d3d_hooks&&) = delete;

    auto install(const device_callbacks& callbacks) -> bool;
    auto uninstall() -> void;
    auto is_installed() const -> bool;

  private:
    bool m_installed = false;
  };

} // namespace ext_client::render
